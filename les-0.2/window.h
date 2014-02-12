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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Sample window structure.
 */
struct window_t {
	double sample;
	struct window_t* next;
};

/**
 * Add a new sample and slide the window.
 */
void window_slide(struct window_t **win, double sample, int size);

/**
 * Calculate window average.
 */
double window_average(struct window_t *win);

/**
 * Print window samples.
 */
void window_print(struct window_t *win);

/**
 * Free window memory.
 */
void window_free(struct window_t *win);

#endif

