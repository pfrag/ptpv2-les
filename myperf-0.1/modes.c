/**
 * modes.c -- Data structures and routines for generating packet sizes.
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

#include "modes.h"

/**
 * Adds a new mode at the head of the list. The *modes pointer is modified.
 */
struct pkt_mode_t *mode_add(struct pkt_mode_t **modes, double ratio, int low, int high, char type) {
	struct pkt_mode_t *newmode = (struct pkt_mode_t *)malloc(sizeof(struct pkt_mode_t));
	memset(newmode, 0, sizeof(struct pkt_mode_t));
	newmode->ratio = ratio;
	newmode->size_range_low = low;
	newmode->size_range_high = high;
	newmode->type = type;
	newmode->next = *modes;
	*modes = newmode;
	return newmode;
}

/**
 * Free the list of modes.
 */
void mode_free_all(struct pkt_mode_t *modes) {
	struct pkt_mode_t *cur = modes;
	struct pkt_mode_t *todel;

	while (cur) {
		todel = cur;
		cur = cur->next;
		if (todel) free(todel);
	}
}

/**
 * Parses a string specifying packet size distributions and returns a list of modes.
 * Example: 0.4:70/0.4:1470/0.2:U70-1470 means 40% 70byte pkts, 40% 1470 pkts and 
 * the rest uniformly chosen in the (70-1470) range.
 * Mode ratios should add up to 1.0. Otherwise, NULL is returned.
 */
struct pkt_mode_t *mode_parse(char *str) {
        char input[128];
		char range[16];
        char *saveptr1;
        char *saveptr2;
        char *saveptr3;
		char *endptr;
        char *strratio, *strsize;
        char *token;
		double ratio;
		double sizehigh;
		double sizelow;
		char *strhigh, *strlow;
		char mtype;
		struct pkt_mode_t *modes = NULL;
		int failure = 0;
		double sum_ratios = 0.0;

        strncpy(input, str, 128);
		token = strtok_r(input, "/", &saveptr1);
        do {
                if (!token) break;
                strratio = strtok_r(token, ":", &saveptr2);
                strsize = strtok_r(NULL, ":", &saveptr2);

				/* parse ratio */
				errno = 0;
				ratio = strtod(strratio, &endptr);
				if (errno || (strratio == endptr) || *endptr != '\0') {
					failure = 1;
					break;
				}
				sum_ratios += ratio;

				/* parse pkt size range */
				if (*strsize == 'U') {
					mtype = 'U';
					/* uniform distribution, parse range */
					strncpy(range, strsize + 1, 16);
					strlow = strtok_r(range, "-", &saveptr3);
		            strhigh = strtok_r(NULL, "-", &saveptr3);
					errno = 0;
					sizelow = strtol(strlow, &endptr, 10);
					if ((errno == ERANGE && (sizelow == LONG_MAX || sizelow == LONG_MIN)) || (errno != 0 && sizelow == 0) || endptr == strlow || *endptr != '\0') {
						failure = 1;
						break;
					}
					errno = 0;
					sizehigh = strtol(strhigh, &endptr, 10);
					if ((errno == ERANGE && (sizehigh == LONG_MAX || sizehigh == LONG_MIN)) || (errno != 0 && sizehigh == 0) || endptr == strhigh || *endptr != '\0') {
						failure = 1;
						break;
					}
				}
				else {
					/* fixed size pkts */
					mtype = 'F';
					errno = 0;
					sizehigh = strtod(strsize, &endptr);
					if (errno || strsize == endptr || *endptr != '\0') {
						failure = 1;
						break;
					}
					sizelow = sizehigh;
				}

					
				mode_add(&modes, ratio, sizelow, sizehigh, mtype);

                token = strtok_r(NULL, "/", &saveptr1);
        } while (1);

		if (failure || (float)sum_ratios != 1.0) {
			mode_free_all(modes);
			modes = NULL;
		}

		return modes;
}

/**
 * Based on the specified distributions, draw a packet size.
 */
int mode_draw_pkt_size(struct pkt_mode_t *modes) {
	struct pkt_mode_t *cur = modes;
	double threshold = 0.0;
	double rn = ((double)random()) / (double)RAND_MAX;

	int pktsize = 0;

	while (!pktsize && cur) {
		threshold += cur->ratio;
		if (rn < threshold) {
			if (cur->type == 'F') pktsize = cur->size_range_low;
			else pktsize = random() % (cur->size_range_high - cur->size_range_low) + cur->size_range_low;
		}
		cur = cur->next;
	}

	return pktsize;
}

/**
 * Calculate average packet size (in bits).
 */
double mode_avg_pkt_size(struct pkt_mode_t *modes) {
	double partial = 0;
	double avg = 0;
	struct pkt_mode_t *cur = modes;
	while (cur) {
		if (cur->type == 'F') {
			avg += ((double)cur->size_range_low) * cur->ratio;
		}
		else {
			avg += ((double)cur->size_range_low + (double)(cur->size_range_high - cur->size_range_low)/2.0)*cur->ratio;
		}
		cur = cur->next;
	}
	return avg*8.0;
}

/**
 * Lists all available modes.
 */
void mode_output(struct pkt_mode_t *modes) {
	struct pkt_mode_t *cur = modes;
	int i = 1;
	printf("Packet size distribution:\n------------------\n");
	while (cur) {
		printf("Mode %d (%c): Ratio %f, [%d,%d]\n", i, cur->type, cur->ratio, cur->size_range_low, cur->size_range_high);
		i++;
		cur = cur->next;
	}
}

