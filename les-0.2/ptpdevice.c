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

#include "ptpdevice.h"

/**
 * Open the tty device to read delay values and configure it.
 */
int dev_init_comm(char *tty, int speed) {
	int fid;
	struct termios new_term;
	
//	if ((fid = open(tty, O_RDONLY | O_NDELAY | O_NOCTTY)) == -1) {
	if ((fid = open(tty, O_RDONLY | O_NOCTTY)) == -1) {
		fprintf(stderr, "Error: Could not open %s\n", tty);
		return -1;
	}
	else if (tcgetattr(fid, &old_term) < 0) {
		fprintf(stderr, "Error: Could not get attributes");
		return -2;
	}

	if (ioctl(fid, TIOCEXCL) < 0) {
		fprintf(stderr, "Warning: Could not set exclusive mode on serial device\n");
	}

	fcntl(fid, F_SETFL, 0);
	new_term = old_term;
	//non-canonical mode
	new_term.c_cflag |= ICANON;
	new_term.c_cflag |= CREAD;

	//set terminal speed
	cfsetspeed(&new_term, speed);

	//no flow control
	new_term.c_cflag &= ~CRTSCTS;

	//other stuff
	new_term.c_iflag = IGNPAR | ICRNL;

	tcflush(fid, TCIFLUSH);
	if (tcsetattr(fid, TCSANOW, &new_term) < 0) {
		fprintf(stderr, "Error: Could not set attributes");
		return -3;
	}

	return fid;
}

/**
 * Read a line of output from the ptp device (96bytes+crlf).
 * Returns the number of bytes read (including the trailing \n\n) and removes
 * \n\n from the end of the line.
 */
int dev_read_line(int fid, char *line, int len, int timeout) {
	int bytes_read = 0;
	int total = 0;
	int remaining = len;
	char data[len];
	char *offset = line;
	fd_set fds;
	struct timeval tv;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;

	while (remaining > 0) {
		memset(data, 0, len);
		//check timeout
		FD_ZERO(&fds);
		FD_SET(fid, &fds);
		select(fid + 1, &fds, NULL, NULL, &tv);

		if (FD_ISSET(fid, &fds)) {
			//try to read the remaining data
			bytes_read = read(fid, offset, remaining);
			remaining -= bytes_read;
			offset += bytes_read;
			total += bytes_read;
		}
		else return total;
	}

	//string ends with \n\n. Just fix it right...
	if (line[total - 2] == '\n') line[total - 2] = '\0';
	return total;
}

/**
 * Close and reset serial dev
 */
void dev_close(int fd) {
	if (fd > 0) {
		tcsetattr(fd, TCSANOW, &old_term);
		close(fd);
	}
}

/**
 * Resynchronize with serial device. We expect that each line is followed by
 * \n\n, so we read until we come across 2 consecutive \n chars. Normaly, it is 
 * run once on program start, but if exclusive access does not succeed on 
 * the serial device and some other process attempts to read from /dev/tty..., 
 * the program should try this function until it succeeds in reading a line
 * of the expected format.
 */
void dev_data_sync(int fd) {
	//sync...consume data until you find \n\n
	char c = 0;

	do {
		read(fd, &c, 1);
		if (c == '\n') {
			read(fd, &c, 1);
			if (c == '\n') return;
		}
	} while(1);
}

/**
 * Parse a line read from the ptp device and calculate the 1-way delay included
 * in the sample.
 */
long long extract_sample_delay(char *line) {
	int x;
	long long t1, t2, t3, t4;
	sscanf(line, "%d %d:%d:%d.%d %Ld %Ld %Ld %Ld", &x, &x, &x, &x, &x, &t1, &t2, &t3, &t4);
	return (t2 - t1 + t4 - t3)/2;
}

/**
 * If the T3 and T4 values are the same across two samples, then it's a SYNC.
 */
int is_sync(char *last_sample, char *sample) {
	int x;
	long long y, t3old, t4old, t3, t4, retval;
	sscanf(last_sample, "%d %d:%d:%d.%d %Ld %Ld %Ld %Ld", &x, &x, &x, &x, &x, &y, &y, &t3old, &t4old);
	sscanf(sample, "%d %d:%d:%d.%d %Ld %Ld %Ld %Ld", &x, &x, &x, &x, &x, &y, &y, &t3, &t4);

	if (t3 == t3old && t4 == t4old) return 1;
	return 0;
}

