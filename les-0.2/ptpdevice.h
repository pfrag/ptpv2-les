/**
 * ptpdevice.h -- Communication with a serial device to get delay measurements.
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

#ifndef _PTPDEVICE_H_
#define _PTPDEVICE_H_

#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct termios old_term;

/**
 * Open the tty device to read delay values and configure it.
 */
int dev_init_comm(char *tty, int speed);

/**
 * Read a line of output from the ptp device (96bytes+crlf).
 * Returns the number of bytes read (including the trailing \n\n) and removes
 * \n\n from the end of the line.
 */
int dev_read_line(int fid, char *line, int len, int timeout);

/**
 * Close and reset serial dev
 */
void dev_close(int fd);

/**
 * Resynchronize with serial device. We expect that each line is followed by
 * \n\n, so we read until we come across 2 consecutive \n chars. Normaly, it is 
 * run once on program start, but if exclusive access does not succeed on 
 * the serial device and some other process attempts to read from /dev/tty..., 
 * the program should try this function until it succeeds in reading a line
 * of the expected format.
 */
void dev_data_sync(int fd);

/**
 * Parse a line read from the ptp device and calculate the 1-way delay included
 * in the sample.
 */
long long extract_sample_delay(char *line);

/**
 * If the T3 and T4 values are the same across two samples, then it's a SYNC.
 */
int is_sync(char *last_sample, char *sample);

#endif

