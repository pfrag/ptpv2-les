/**
 * stats.c -- Data structures and routines to keep track of experiment 
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

#include "stats.h"

struct exp_stats_t *stats_lookup_host(struct exp_stats_t *stats, char *host, int port) {
	struct exp_stats_t *cur = stats;

	while (cur) {
		if (!strcmp(cur->host, host) || cur->port == port)  return cur;
		cur = cur->next;
	}

	return NULL;
}

void stats_remove_host(struct exp_stats_t **stats, char *host, int port) {
	struct exp_stats_t *cur = *stats;
	struct exp_stats_t *prev = NULL;

	while (cur) {
		if (!strcmp(cur->host, host) || cur->port == port) {
			if (prev) {
				prev->next = cur->next;
			}
			else {
				/* removing list head */
				*stats = cur->next;
			}
			free(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}

struct exp_stats_t *stats_add_host(struct exp_stats_t **stats, char *host, int port) {
	/* already exists? */
	struct exp_stats_t *newnode = stats_lookup_host(*stats, host, port);
	if (newnode) return newnode;

	/* doesn't exist, create new one */
	newnode = (struct exp_stats_t*)malloc(sizeof(struct exp_stats_t));
	memset(newnode, 0, sizeof(struct exp_stats_t));
	strcpy(newnode->host, host);
	newnode->port = port;
	newnode->next = *stats;
	*stats = newnode;
	
	return newnode;
}

void stats_update_counters(struct exp_stats_t **stats, char *host, int port, int pktsize, int rx) {
	/* lookup or add if it does not exist */
	struct exp_stats_t *cur = stats_add_host(stats, host, port);

	if (rx) {
		cur->pkt_rx++;
		cur->bytes_rx += pktsize;
	}
	else {
		cur->pkt_tx++;
		cur->bytes_tx += pktsize;
	}
}

void stats_reset_counters(struct exp_stats_t **stats, char *host, int port) {
	/* lookup or add if it does not exist */
	struct exp_stats_t *cur = stats_add_host(stats, host, port);

	cur->pkt_rx = 0;
	cur->bytes_rx = 0;
	cur->pkt_tx = 0;
	cur->bytes_tx = 0;
}

void stats_free(struct exp_stats_t *stats) {
	struct exp_stats_t *cur = stats;
	struct exp_stats_t *todel;

	while (cur) {
		todel = cur;
		cur = cur->next;
		if (todel) free(todel);
	}
}

