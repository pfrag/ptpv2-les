/**
 * les.c -- Load estimation server. Communicates with a PTP device over the
 * serial interface and, based on the delay measurements read, calculates the 
 * network load level (low, medium, high). Clients can retrieve load estimates
 * by sending an LREQ message.
 *
 * Copyright (C) 2012-2013 Pantelis A. Frangoudis <pfrag@aueb.gr>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "les.h"

/**
 * Load estimation algorithm.
 */
double estimate_load(struct les_params_t *params, struct window_t *window, long long sample) {
	double retval;
	int err;

	pthread_mutex_lock(&mtx_delay_info);

	/* some delay statistics */
	delay_stats.nsamples++;
	delay_stats.sample_sum += sample;
	delay_stats.avg = ((double)delay_stats.sample_sum) / (double)delay_stats.nsamples;
	delay_stats.weighted_avg = (1 - params->w)*(double)sample + params->w*delay_stats.weighted_avg;
	if (sample > delay_stats.max) delay_stats.max = sample;
	if (sample < delay_stats.min || delay_stats.min == 0) delay_stats.min = sample;

	/* load to which current sample maps */
	double l;

	/* Add sample to window and slide it */
	window_slide(&window, sample, params->winsize);

	/* Calculate window average */
	double avg = window_average(window);

	/* If delay is below a threshold, consider current load as 0 */
	if (avg < params->Dlow) {
		l = 0.0;
	}
	else {
		/* Otherwise, use curve. If curve value > 1.0, consider curr load as 1*/
		l = fmin(map_to_load(params->fitfunc, avg, &err), 1.0);
	}

	/* Update load estimate */
	delay_stats.load_type = params->w*delay_stats.load_type + (1 - params->w)*l;
	retval = delay_stats.load_type;

	pthread_mutex_unlock(&mtx_delay_info);

	return retval;
}

/**
 * Log a delay sample and the current load estimate with a local timestamp
 */
void log_delay_sample(FILE *fp, long long sample, double load) {
	char s[80];
	char str_sec[32];
	char timestamp[64];
	struct tm ptm;
	struct timeval tv;

	if (!fp) return;

	memset(&tv, 0, sizeof(tv));
	memset(&ptm, 0, sizeof(ptm));
	memset(timestamp, 0, 64);
	memset(str_sec, 0, 32);
	memset(s, 0, 80);

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ptm);
	strftime(str_sec, 64, "%Y-%m-%d,%H:%M:%S", &ptm);
	sprintf (timestamp, "%s.%03ld", str_sec, tv.tv_usec/1000);

	sprintf(s, "%s\t%Ld\t%f\n", timestamp, sample, load);
	fwrite((void*)s, 1, strlen(s), fp);
	fflush(fp);
}

/**
 * Return current load information.
 */
struct load_info_t *get_load_info() {
	struct load_info_t *retval;
	retval = (struct load_info_t*)malloc(sizeof(struct load_info_t));
	memset(retval, 0, sizeof(struct load_info_t));

	pthread_mutex_lock(&mtx_delay_info);
	memcpy(retval, &delay_stats, sizeof(struct load_info_t));	
	pthread_mutex_unlock(&mtx_delay_info);

	/* todo: negative load for STATUS_DEV_UNAVAIL or xtra status fld in proto */

	return retval;
}

/**
 * Thread which communicates with the PTP device and maintains delay statistics.
 * Delay samples are also logged to a file. Configuration options are passed
 * in the params argument, which is supposed to be a struct les_params_t*.
 */
void tfunc_delay_monitor(void *params) {
	/* Log delay measurements here */
	FILE *logfp;

	/* init/config serial communication */
	int fd = dev_init_comm( ((struct les_params_t*)params)->devname, ((struct les_params_t*)params)->devspeed);
	if (fd < 0)  {
		return;
	}

	/* open log file */
	logfp = fopen(((struct les_params_t*)params)->logfile, "w");
	
	//sync with data from serial port
	dev_data_sync(fd);

	int i = 0;
	int len = 0;
	long long sample;
	char line[99]; //line data + \n\n
	char lastline[99]; //previous line read
	double load = 0;

	memset(lastline, 0, 99);
	do {
		int stopval = 0;
		pthread_mutex_lock(&mtx_running);
		stopval = stop;
		pthread_mutex_unlock(&mtx_running);
		if (stopval) break;

		memset(line, 0, 99);
		len = dev_read_line(fd, line, 98, 1200);
		if (strlen(line) != 96) {
			//resync
			dev_data_sync(fd);
		}
		else {
			if (((struct les_params_t*)params)->skipsync && is_sync(lastline, line)) {
				//we ignore SYNC delay samples
				memcpy(lastline, line, 99);
				continue;
			}
			sample = extract_sample_delay(line);
			if (sample > 0) {
				memcpy(lastline, line, 99);
				load = estimate_load((struct les_params_t*)params, window, sample);
				log_delay_sample(logfp, sample, load);
			}
		}
	} while(1);

	fclose(logfp);
	dev_close(fd);
}

void term_handler(int signal) {
	int exiting = 0;
	pthread_mutex_lock(&mtx_running);
	if (!stop) {
		stop = 1;
		fprintf(stderr, "\nClosing files & devices. Press ctrl-c again to exit\n");
	}
	else {
		exiting = 1;
	}
	pthread_mutex_unlock(&mtx_running);
	if (exiting) {
		exit(0);
	}
}

/**
 * Log a message to syslog (should be open) or to stderr
 */
void log_message(int type, char *message, int tosyslog) {
	if (tosyslog) {
		syslog(type, "%s", message);
	}
	else {
		fprintf(stderr, "%s\n", message);
	}
}

/**
 * Run as a daemon.
 */
void daemonize(char *lockfile) {
	pid_t pid;
	pid_t sid;
	int n;

	int lock_fd;

	char str[10];
	if (getppid() == 1) {
		return;
	}

	pid = fork();
	if (pid < 0) {
		exit(1); /* failure */
	}

	if (pid > 0) {
		exit(0); /* success */
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	umask(0); /* set newly created file permissions */

	sid = setsid(); /* obtain a new process group */
	if (sid < 0) {
		exit(1);
	}

	lock_fd = open(lockfile, O_RDWR|O_CREAT, 0640);
	if (lock_fd < 0) {
		log_message(LOG_ERR, "Error: les: Could not create lock file", 1);
		exit(1);
	}

	if (lockf(lock_fd, F_TLOCK, 0) < 0) {
		log_message(LOG_ERR, "Error: les: Cannot lock pid file. les already running?", 1);
		exit(1);
	}

	sprintf(str, "%d\n", getpid());
	n = write(lock_fd, str, strlen(str));

	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTERM, term_handler);
	signal(SIGKILL, term_handler);
	signal(SIGINT, term_handler);
	signal(SIGHUP, term_handler);
}

int main(int argc, char **argv) {
	int ret;
	int mtype;
	char *message;
	int clen;
	int i;
	int perr;
	struct load_info_t *linfo;

	/***********************************************************/
	/* Configuration */
	/***********************************************************/
	int is_daemon;
	char lockfile[80];
	struct les_params_t params;
	struct cnx_info_t info;
	char *checkptr;

	memset(&params, 0, sizeof(struct les_params_t));
	memset(&info, 0, sizeof(struct cnx_info_t));

	char **confvalues;
	char *confoptions[] = {
		"ttydev", 
		"ttyspeed",
		"fitfunc",
		"w",
		"winsize",
		"dlow",
		"protocol",
		"port",
		"daemon",
		"lockfile",
		"outfile",
		"skipsync",
		NULL
	};

	if (argc != 2) {
		fprintf(stderr, "Usage: les conffile\n");
		exit(1);
	}

	/* read configuration */
	confvalues = (char **)malloc(12 * sizeof(char*));
	for (i = 0; i < 12; i++) {
		confvalues[i] = (char*)malloc(80);
		memset(confvalues[i], 0, 80);
	}
	ret = parse_conffile(argv[1], (char**)confoptions, confvalues, 12);
	if (ret == -1) {
		fprintf(stderr, "Error: Could not open configuration file\n");
		exit(1);
	}

	if (ret == -2) {
		fprintf(stderr, "Error: Invalid configuration option\n");
		exit(1);
	}

	if (ret == -3) {
		fprintf(stderr, "Error: Duplicate configuration option\n");
		exit(1);
	}

	/* tty device */
	if (!*confvalues[0]) {
		strncpy(params.devname, DEF_TTYDEV, 80);
	}
	else {
		strncpy(params.devname, confvalues[0], 80);
	}

	/* device speed */
	if (!*confvalues[1]) {
		if (!strncasecmp(confvalues[1], "115200", 6)) {
			params.devspeed = B115200;
		}
		else if (!strncasecmp(confvalues[1], "9600", 6)) {
			params.devspeed = B9600;
		}
		else if (!strncasecmp(confvalues[1], "38400", 6)) {
			params.devspeed = B38400;
		}
		else {
			/* default */
			params.devspeed = DEF_TTYSPEED;
		}
	}
	else {
		params.devspeed = DEF_TTYSPEED;
	}

	/* fit function */
	if (!*confvalues[2]) {
		strcpy(params.fitfunc, DEF_FITFUNC);
	}
	else {
		strcpy(params.fitfunc, confvalues[2]);
		/* Test if the given func is syntactically correct. If not, silently go back to default */
		map_to_load(params.fitfunc, 1.0, &perr);
		if (perr) {
			memset(params.fitfunc, 0, 128);
			strcpy(params.fitfunc, DEF_FITFUNC);
		}
	}

	/* smoothing factor */
	params.w = strtod(confvalues[3], &checkptr);
	if (*checkptr != '\0') {
		params.w = DEF_W;
	}

	/* window size */
	params.winsize = strtol(confvalues[4], &checkptr, 10);
	if (*checkptr != '\0') {
		params.winsize = DEF_WINSIZE;
	}

	/* low load threshold (in nanosec) */
	params.Dlow = strtod(confvalues[5], &checkptr);
	if (*checkptr != '\0') {
		params.Dlow = DEF_DLOW;
	}

	/* protocol (see netfunc.h) */
	if (!strncmp(confvalues[6], "TCP", 3)) {
		info.proto = _PROTO_TCP_;
	}
	else if (!strncmp(confvalues[6], "UDP", 3)) {
		info.proto = _PROTO_UDP_;
	}
	else {
		info.proto = DEF_PROTOCOL;
	}
	
	/* port */
	info.port = strtol(confvalues[7], &checkptr, 10);
	if (*checkptr != '\0') {
		info.port = DEF_PORT;
	}

	/* daemonize? */
	is_daemon = strtol(confvalues[8], &checkptr, 10);
	if (*checkptr != '\0' || (is_daemon != 0 && is_daemon != 1) ) {
		/* maybe value is given as yes/y/no/n */
		if (!strncasecmp(confvalues[8], "y", 1)) {
			is_daemon = 1;
		}
		else 
		if (!strncasecmp(confvalues[8], "n", 1)) {
			is_daemon = 0;
		}
		else {
			is_daemon = DEF_DAEMON;
		}
	}
	
	/* lockfile */
	memset(lockfile, 0, 80);
	if (!*confvalues[9]) {
		strncpy(lockfile, DEF_LOCKFILE, 80);
	}
	else {
		strncpy(lockfile, confvalues[9], 80);
	}

	/* output file */
	if (!*confvalues[10]) {
		strncpy(params.logfile, DEF_OUTFILE, 80);
	}
	else {
		strncpy(params.logfile, confvalues[10], 80);
	}

	/* ignore sync messages in load estimation */
	params.skipsync = strtol(confvalues[11], &checkptr, 10);
	if (*checkptr != '\0' || (params.skipsync != 0 && params.skipsync != 1) ) {
		/* maybe value is given as yes/y/no/n */
		if (!strncasecmp(confvalues[11], "y", 1)) {
			params.skipsync = 1;
		}
		else 
		if (!strncasecmp(confvalues[11], "n", 1)) {
			params.skipsync = 0;
		}
		else {
			params.skipsync = DEF_SKIPSYNC;
		}
	}

	/* show configuration */
	print_config(&params, &info, is_daemon, lockfile);

	for (i = 0; i < 12; i++) {
		free(confvalues[i]);
	}
	free(confvalues);

	/* endof configuration options */
	/******************************************************/

	if (is_daemon) {
		/* start as a daemon */
		openlog ("les", LOG_PID, LOG_LOCAL5);
		daemonize(lockfile);
	}
	else {
		/* set signals */
		signal(SIGTERM, term_handler);
		signal(SIGKILL, term_handler);
		signal(SIGINT, term_handler);
		signal(SIGHUP, term_handler);
	}

	/* mutices */
	pthread_mutex_init(&mtx_running, NULL);
	pthread_mutex_init(&mtx_delay_info, NULL);

	/* sample window */
	window = NULL;

	/* threads */
	pthread_t delay_thread;
	pthread_create(&delay_thread, NULL, (void*)&tfunc_delay_monitor, (void*)&params);

	/* networking */
	memcpy(info.host, "0.0.0.0\0", 8); /* any addr */
	if (init_server(&info) < 0) {
		fprintf(stderr, "Init failed");
		exit(1);
	}

	do {
		int stopval = 0;
		pthread_mutex_lock(&mtx_running);
		stopval = stop;
		pthread_mutex_unlock(&mtx_running);
		if (stopval) break;

		struct cnx_info_t clicnx;

		/* either apply an iterative TCP server, or work on top of UDP */
		if (info.proto == _PROTO_TCP_) {
			/* accept */
			struct cnx_info_t* cinfo = accept_client_connection(&info);
			memcpy(&clicnx, cinfo, sizeof(struct cnx_info_t));
			free(cinfo);
		}
		else {
			/* Operation over UDP, use the original "info" struct */
			memcpy(&clicnx, &info, sizeof(struct cnx_info_t));
		}

		/* read a protocol message */
		/* TODO: check timeout! */
		message = recv_protocol_message(&clicnx, &mtype, 0, FROM_CLIENT);

		/* check message type and respond */
		if (mtype == MTYPE_LREQ) {
			if (message) free(message);
			linfo = get_load_info();
			message = generate_load_response(linfo);
			free(linfo);

			/* send load response */
			xmit_protocol_message(&clicnx, (void *)message, TO_CLIENT);
			if (message) free(message);
		}
	} while (1);

	return 0;
}

/**
 * Output configuration settings.
 */
void print_config(struct les_params_t* lp, struct cnx_info_t *cnx, int daemon, char *lockfile) {
	fprintf(stderr, "Application configuration:\n");	
	fprintf(stderr, "--------------------------\n");
	if (cnx->proto == _PROTO_TCP_) {
		fprintf(stderr, "Protocol: TCP\n");
	}
	else if (cnx->proto == _PROTO_UDP_) {
		fprintf(stderr, "Protocol: UDP\n");
	}
	else {
		fprintf(stderr, "Protocol: Unknown\n");
	}
	fprintf(stderr, "Port: %d\nDaemon: %d\nLockfile: %s\nOutput file: %s\n", cnx->port, daemon, lockfile, lp->logfile);

	fprintf(stderr, "\nDevice configuration:\n");	
	fprintf(stderr, "--------------------------\n");
	int speed;
	switch (lp->devspeed) {
		case B115200:
			speed = 115200;
			break;
		case B9600:
			speed = 9600;
		case B38400:
			speed = 38400;
		default:
			speed = -1;
	}
	fprintf(stderr, "Name: %s\nSpeed: %d\n", lp->devname, speed);

	fprintf(stderr, "\nLoad estimation algorithm:\n");
	fprintf(stderr, "Fit function: %s\nSmoothing factor (w): %lf\nSample window size: %d\nLow load threshold: %lf\nSkip SYNC: %d\n", lp->fitfunc, lp->w, lp->winsize, lp->Dlow, lp->skipsync);
	fprintf(stderr, "--------------------------\n\n");
}

