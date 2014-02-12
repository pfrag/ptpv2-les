/**
 * myperf.c -- A simple UDP traffic generator capable of generating traffic 
 * with multi-modal packet size distributions. 
 *
 * Example usage:
 * + Server (rx): myperf -s -p 12345
 * + Client (tx): myperf -c XXX.XXX.XXX.XXX -p 12345 -l 100000000 -t 60 -d 0.4:40/0.4:1470/0.2:U40-1470
 *
 * The above command generates traffic with average bitrate of 100 Mbps for 
 * 60 seconds to a server with an IP address XXX.XXX.XXX.XXX listening at 
 * port 12345. The -d flag specifies the packet size distribution. In this 
 * example, packet size distribution is bimodal: Each packet size is either
 * 40 bytes (with probability 0.4), either 1470 bytes (with probability 0.4) or
 * its sizes is uniformly drawn from the (40,1470) range with probability 0.2.
 * A user can specify arbitrarily many modes. For each mode, the syntax is as 
 * follows: 
 * + Fixed packet size mode: <mixing proportion>:<pkt size>
 * + Uniformly distributed packet size mode: <mixing proportion>:U<minimum size>-<maximum size>. 
 * and modes are separated by "/".
 *
 * To generate CBR traffic with fixed packet sizes, add the -d 1.0:<pkt size> flag.
 * NOTE: mixing proportions should add up to 1.0.
 * 
 *
 * Copyright (C) 2013 Pantelis A. Frangoudis <pfrag@aueb.gr>
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

#include "myperf.h"

#define SZ_HDR_PLUS_ETH	42

struct option long_options[] = {
	{"server",			no_argument,		0,		's'},
	{"client",			required_argument,	0,		'c'},
	{"load",			required_argument,	0,		'l'},
	{"port",			required_argument,	0,		'p'},
	{"time",			required_argument,	0,		't'},
	{"distribution",	required_argument,	0,		'd'},
	{"help",			no_argument,		0,		'h'},
	{0,					0,					0,		0}
};

/**
 * Keep track of per-host pkt stats
 */
struct exp_stats_t *statistics;

/**
 * Go to sleep (uses nanosleep)
 */
void mysleep(unsigned long usec) {
    struct timespec requested, remaining;

    requested.tv_sec  = 0;
    requested.tv_nsec = usec * 1000L;

    nanosleep(&requested, &remaining);
}

/**
 * Output statistics (pkt/byte counts, tx bitrate, pkt loss)
 */
void show_client_stats(struct exp_stats_t *stats, int duration) {
	double bitrate = (double)stats->bytes_tx * 8.0 / (double)duration / 1000.0;
	int lost = stats->pkt_tx - stats->pkt_rx;
	double loss = (double)(stats->pkt_tx - stats->pkt_rx)/(double)stats->pkt_tx;
	printf("\nStatistics:\n======================\n");
	printf("Sent %lld bytes (%lld pkts) in %d s.\nServer received %lld bytes (%lld pkts).\nBitrate: %f Kbps\nLoad: %f\nPkt Loss: %f\n", stats->bytes_tx, stats->pkt_tx, duration, stats->bytes_rx, stats->pkt_rx, bitrate, (stats->bytes_tx*8 + stats->pkt_tx*SZ_HDR_PLUS_ETH*8)/(double)duration/1000.0, loss);
	printf("======================\n");
}

/**
 * Show usage.
 */
void usage() {
	printf("Usage: myperf -s/-c [ipaddr] -p [port] -l [load in Mbps]\n");
	printf("\t\t--server/-s\t\t\tRun in server (receiver) mode\n");
	printf("\t\t--client/-c [ipaddr]\t\tRun in client (sender) mode\n");
	printf("\t\t--load/-l [bitrate in bps]\tAverage network load\n");
	printf("\t\t--distribution/-d [string]\tPacket size distribution specification\n");
	printf("\t\t--port/-p [port number]\t\tPort\n");
	printf("\t\t--time/-t [tx duration]\t\tTransmission duration\n");
	printf("\t\t--help/-h\t\t\tDisplay this message\n");
}

int main(int argc, char **argv) {
	int c = 0;
	int smode = 0;
	int cmode = 0;
	char *checkptr;
	char str_rcv_addr[80];
	int duration = -1;
	double load = -1;
	int port = 0;
	int ret = 0;
	char pkt_distro[128];
	char *message = NULL;
	int mtype = 0;
	int pktsize = 0;

	while((c = getopt_long(argc, argv, "c:p:l:d:t:sh", long_options, NULL)) != -1) {
		switch (c) {
            case 'c':
                /* client mode */
				cmode = 1;
				memset(str_rcv_addr, 0, 80);
				sprintf(str_rcv_addr, "%s", optarg);
				break;

                break;
            case 's':
                /* server mode */
				smode = 1;
                break;
            case 'p':
                /* port */
				port = strtol(optarg, &checkptr, 10);
				if ((*checkptr != '\0' && (port == LONG_MAX || port == LONG_MIN)) || port == 0 ) {
					fprintf(stderr, "Invalid port number\n");
					exit(1);
				}
                break;
			case 'l':
				/* load (bps) */
				load = strtod(optarg, &checkptr);
				if ((*checkptr != '\0' && (load == HUGE_VALF || load == HUGE_VALL)) || load == 0 ) {
					fprintf(stderr, "Invalid load value\n");
					exit(1);
				}
				break;
			case 'd':
				/* pkt size distribution */
				strcpy(pkt_distro, optarg);				
				break;
			case 't':
				/* tx duration */
				duration = strtol(optarg, &checkptr, 10);
				if ((*checkptr != '\0' && (duration == LONG_MAX || duration == LONG_MIN)) || duration == 0) {
					fprintf(stderr, "Invalid tx duration\n");
					exit(1);
				}
				break;
			case 'h':
				usage();
				return 0;
		}
	}

	struct pkt_mode_t *pktmodes = mode_parse(pkt_distro);
	if (cmode && !pktmodes) {
		fprintf(stderr, "Invalid packet distribution specification\n");
		return 1;
	}

	/* Show configuration */
	printf("Configuration:\n======================\n");
	if (cmode) {
		printf("Mode:\t\tClient [tx to %s]\n", str_rcv_addr);
		printf("Port:\t\t%d\n", port);
		printf("Load:\t\t%f Kbps\n", load/1000.0);
		printf("Duration:\t%d s\n", duration);
		mode_output(pktmodes);
	}
	else {
		printf("Mode:\t\tServer\n");
		printf("Port:\t\t%d\n", port);
		printf("======================\n");
	}

	/* modes are mutually exclusive */
	if ( (cmode == smode) && smode ) {
		fprintf(stderr, "Please select a mode (-c/-s)\n\n");
		usage();
		return 1;
	}

	/* initialize cnx structure */
	struct cnx_info_t cnx;
	memset(&cnx, 0, sizeof(struct cnx_info_t));
	cnx.proto = _PROTO_UDP_;
	if (smode) {
		strcpy(cnx.host, "0.0.0.0");
	}
	else {
		strcpy(cnx.host, str_rcv_addr);
	}
	cnx.port = port;

	/* seed me */
	srandom(time(NULL));

	if (smode) {
		/* server mode */
		char *hostip;

		/* init statistics */
		struct exp_stats_t *stats = NULL;		

		/* init */
		if (init_server(&cnx) < 0) {
			fprintf(stderr, "Failed to initialize server\n");
			return 1;
		}

		while (1) {
			/* handle packets: foreach pkt, update stats */
			message = recv_protocol_message(&cnx, &mtype, 0, FROM_CLIENT);
			hostip = get_peer_addr(&cnx, FROM_CLIENT);

			struct exp_stats_t *clistats = NULL;
			switch (mtype) {
				case MTYPE_STRT:
					/* reset statistics */
					stats_reset_counters(&stats, hostip, port);
					break;
				case MTYPE_DATA:
					/* get message length and update pkt stats */
					pktsize = parse_data(message);
					if (pktsize > 0) {
						stats_update_counters(&stats, hostip, port, pktsize, 1);
					}
					break;
				case MTYPE_STOP: case MTYPE_STAT:
					/* end-of-tx, respond with a STAT message */
					clistats = stats_lookup_host(clistats, hostip, port);
					if (message) free(message);
					message = generate_stat_rsp(stats);
					xmit_protocol_message(&cnx, message, TO_CLIENT);
					break;
				default:
					break;
			}

			if (message) free(message);
			if (hostip) free(hostip);

		}
	}
	else {
		/* client mode */
		int pktsize = 0;

		/* init statistics */
		struct exp_stats_t *stats = NULL;

		/* init cnx to server */
		ret = init_client_connection(&cnx);
		if (ret < 0) {
			fprintf(stderr, "Failed to initialize client\n");
			return 1;
		}
		
		/* calculate stuff about client behavior (pkt rate based on load, etc.) */
		double avg_pkt_size = mode_avg_pkt_size(pktmodes) + (double)SZ_HDR_PLUS_ETH*8.0;
		double pps = load/avg_pkt_size;

		/* usec to sleep between packets (target value to achieve target bps) */
		int sleep_time = (int)(1.0/pps * 1000000.0);

		printf("Avg pkt size:\t%f b (%f B)\nPPS:\t\t%f\nSleeptime:\t%d us\n", avg_pkt_size, avg_pkt_size/8.0, pps, sleep_time);
		printf("----------------------\n\n");

		/* start tx */
		message = generate_strt_msg();
		xmit_protocol_message(&cnx, message, TO_SERVER);
		if (message) free(message);

		/* stuff to the duration of the experiment */
		struct timeval expstart, curtime;
		long long remaining_time = 0;
		gettimeofday(&expstart, NULL);

		/* 
		 * Sleep stuff...We have a target sleep time to maintain target bitrate. 
		 * Each time, we measure how much it took to do pkt processing, tx and 
		 * sleep and calculate its offset from the target. In the next tx, we 
		 * compensate for the difference by subtracting the offset from the time
		 * to sleep. startts, endts: timestamps to measure how much a loop took.
		 */
		struct timeval startts;
		struct timeval endts;
		long offset = 0;
		long time_to_sleep = sleep_time;

		/* loop and generate packets */
		do {
			gettimeofday(&startts, NULL);

			/* draw packet size */
			pktsize = mode_draw_pkt_size(pktmodes);

			/* generate & tx dummy packet */
			message = generate_data(pktsize);
			xmit_protocol_message(&cnx, message, TO_SERVER);
			if (message) free(message);

			/* update pkt counter */
			stats_update_counters(&stats, "127.0.0.1", port, pktsize, 0);

			/* sleep */
			time_to_sleep -= offset;
			if (time_to_sleep > 0) {
				mysleep(time_to_sleep);
			}
			gettimeofday(&endts, NULL);
			offset = endts.tv_sec*1000000 + endts.tv_usec - startts.tv_sec * 1000000 - startts.tv_usec - sleep_time;

			gettimeofday(&curtime, NULL);
			remaining_time = (long long)expstart.tv_sec*1000000 + (long long)expstart.tv_usec + (long long)duration*1000000 - (long long)curtime.tv_sec * 1000000 - (long long)curtime.tv_usec;
		} while (remaining_time > 0);
		/* end-of-tx */

		/* notify server, request rx stats (5 tries, otherwise quit) */
		int tries = 5;
		struct exp_stats_t* srvstats;
		while (tries) {
			message = generate_stop_msg(); /* a STAT would also do...*/
			xmit_protocol_message(&cnx, message, TO_SERVER);
			if (message) free(message);

			message = recv_protocol_message(&cnx, &mtype, 2000, FROM_SERVER);
			if (!message) {
				tries--;
				continue;
			}

			srvstats = parse_stat_rsp(message);
			if (message) free(message);
			if (srvstats) {
				stats->bytes_rx = srvstats->bytes_rx;
				stats->pkt_rx = srvstats->pkt_rx;
				free(srvstats);
				break;
			}
			else {
				tries--;
			}
		}

		if (!tries) {
				printf("Could not get server report\n");
		}

		show_client_stats(stats, duration);
		stats_free(stats);
		mode_free_all(pktmodes);
	}

	return 0;
}

