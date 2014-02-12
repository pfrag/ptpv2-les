/**
 * window.c -- Sample sliding window.
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

#include "window.h"

/**
 * Add a new sample and slide the window.
 */
void window_slide(struct window_t **win, double sample, int size) {
	int i = 0;

	struct window_t *cur = *win;
	struct window_t *last = NULL;
	struct window_t *prev = NULL;

	while (cur) {
		i++;
		if (i > 1) {
			prev = last;
		}
		last = cur;
		cur = cur->next;
	}

	/* if window is full, remove last item */
	if (i == size) {
		/* pop last */
		if (prev) {
			prev->next = NULL;
		}
		if (*win == last) {
			*win = NULL;
		}
		free(last);
	}

	/* alloc space for new window sample */
	struct window_t *newsample = (struct window_t*)malloc(sizeof(struct window_t));
	memset(newsample, 0, sizeof(struct window_t));
	newsample->sample = sample;
	newsample->next = NULL;

	/* add sample */
	if (*win) {
		newsample->next = *win;
		*win = newsample;
	}
	else {
		*win = newsample;
	}
}

/**
 * Calculate window average.
 */
double window_average(struct window_t *win) {
	double sum = 0.0;
	int samples = 0;
	struct window_t *cur = win;
	while (cur) {
		sum += cur->sample;
		samples++;
		cur = cur->next;
	}

	return (sum/(double)samples);
}

/**
 * Print window samples.
 */
void window_print(struct window_t *win) {
	struct window_t *cur = win;
	while(cur) {
		printf("%f\t", cur->sample);
		cur = cur->next;
	}
	printf("\n");
}

/**
 * Free window memory.
 */
void window_free(struct window_t *win) {
	struct window_t *cur = win;
	struct window_t *toremove = NULL;
	while (cur) {
		toremove = cur;
		cur = cur->next;
		free(toremove);
	}
}

