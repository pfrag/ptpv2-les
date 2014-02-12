/**
 * modes.h -- Data structures and routines for generating packet sizes.
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

#ifndef _MODES_H_
#define _MODES_H_

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

/**
 * Packet size mode.
 */
struct pkt_mode_t {
	/* mixing proportion (ratio of packets following this mode */
	double ratio;

	/* packet size range */
	int	size_range_low;
	int size_range_high;

	/* F: fixed, U: uniform */
	char type;

	struct pkt_mode_t *next;
};

/**
 * Adds a new mode at the head of the list. The *modes pointer is modified.
 */
struct pkt_mode_t *mode_add(struct pkt_mode_t **modes, double ratio, int low, int high, char type);

/**
 * Free the list of modes.
 */
void mode_free_all(struct pkt_mode_t *modes);

/**
 * Parses a string specifying packet size distributions and returns a list of modes.
 * Example: 0.4:70/0.4:1470/0.2:U70-1470 means 40% 70byte pkts, 40% 1470 pkts and 
 * the rest uniformly chosen in the (70-1470) range.
 * Mode ratios should add up to 1.0. Otherwise, NULL is returned.
 */
struct pkt_mode_t *mode_parse(char *str);

/**
 * Based on the specified distributions, draw a packet size.
 */
int mode_draw_pkt_size(struct pkt_mode_t *modes);

/**
 * Calculate average packet size (in bits).
 */
double mode_avg_pkt_size(struct pkt_mode_t *modes);

/**
 * Lists all available modes.
 */
void mode_output(struct pkt_mode_t *modes);
#endif

