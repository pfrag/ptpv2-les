/**
 * stats.h -- Data structures and routines to keep track of experiment 
 * statistics (# pkts sent/received, # bytes sent/received).
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

#ifndef _STATS_H_
#define _STATS_H_

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/**
 * Experiment statistics
 */
struct exp_stats_t {
	char host[80];
	int port;
	long long pkt_tx;
	long long pkt_rx;
	long long bytes_tx;
	long long bytes_rx;
	struct exp_stats_t *next;
};

struct exp_stats_t *stats_lookup_host(struct exp_stats_t *stats, char *host, int port);
void stats_remove_host(struct exp_stats_t **stats, char *host, int port);
struct exp_stats_t *stats_add_host(struct exp_stats_t **stats, char *host, int port);
void stats_update_counters(struct exp_stats_t **stats, char *host, int port, int pktsize, int rx);
void stats_reset_counters(struct exp_stats_t **stats, char *host, int port);
void stats_free(struct exp_stats_t *stats);

#endif
