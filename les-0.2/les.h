/**
 * les.h -- Load estimation server. Communicates with a PTP device over the
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

#ifndef _LES_H_
#define _LES_H_

#include "protocol.h"
#include "ptpdevice.h"
#include "window.h"
#include "conffile.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <math.h>

/**
 * Default configuration options
 */
#define DEF_OUTFILE		"delays.log"
#define DEF_TTYSPEED	B115200
#define DEF_TTYDEV		"/dev/ttyUSB0"
#define DEF_FITFUNC		"100000.0+200000.0*X"
#define DEF_W			0.85
#define DEF_WINSIZE		1
#define DEF_DLOW		135000
#define DEF_PROTOCOL	_PROTO_UDP_ /* from netfunc.h */
#define DEF_PORT		7575
#define DEF_LOCKFILE	"les.lock"
#define DEF_DAEMON		0
#define DEF_SKIPSYNC	0

pthread_mutex_t mtx_delay_info;
pthread_mutex_t mtx_running;
int stop = 0;

struct load_info_t delay_stats;
struct window_t *window;

/**
 * Parameters for the load estimation algorithm
 */
struct les_params_t {
	/* fit function */
	char fitfunc[256];
	/* smoothing factor */
	double w;
	/* window size */
	int winsize;
	/* low load delay threshold */
	double Dlow;
	/* ignore sync delay samples */
	int skipsync;
	/* tty device name */
	char devname[128];
	/* tty device speed */
	int devspeed;
	/* output file */
	char logfile[256];
};

/**
 * Update running load estimate based on the new delay sample received.
 */
double estimate_load(struct les_params_t *params, struct window_t *window, long long sample);

/**
 * Log a delay sample and the current load estimate with a local timestamp
 */
void log_delay_sample(FILE *fp, long long sample, double load);


/**
 * Return current load information.
 */
struct load_info_t *get_load_info();

/**
 * Thread which communicates with the PTP device and maintains delay statistics.
 * Delay samples are also logged to a file. Configuration options are passed
 * in the params argument, which is supposed to be a struct log_params_t*.
 */
void tfunc_delay_monitor(void *params);

/**
 * Termination signal handler.
 */
void term_handler(int signal);

/**
 * Run as a daemon.
 */
void daemonize(char *lockfile);

/**
 * Log message to syslog or stderr.
 */
void log_message(int type, char *message, int tosyslog);

/**
 * Output configuration settings.
 */
void print_config(struct les_params_t* lp, struct cnx_info_t *cnx, int daemon, char *lockfile);

/**
 * Parses the expression of load as a function of delay and returns the result.
 * Function is supposed to be the string read from the configuration file, e.g.,
 * e^-(1.08X) +5*e^(2*X). The name of the variable is *strictly* X and will be
 * replaced by the value of the delay argument.
 */
double map_to_load(char *func, double val, int *err);
#endif

