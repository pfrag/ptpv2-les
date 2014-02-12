/**
 * protocol.h
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

#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include "netfunc.h"
#include "b64.h"

#define PEEK_DATA_LEN 30

#define MTYPE_LREQ				10
#define MTYPE_LRSP				20

#define LREQ_HDR "LREQ"
#define LRSP_HDR "LRSP"

#define STATUS_OK				200
#define STATUS_DEV_UNAVAIL		400
#define STATUS_GEN_ERR			444

#define LOAD_HIGH				'H'
#define LOAD_MEDIUM				'M'
#define LOAD_LOW				'L'

/**
 * Delay/load statistics and information
 */
struct load_info_t {
	int status; //server/response status
	int nsamples;
	long long sample_sum;
	double avg;
	double weighted_avg;
	long long min;
	long long max;
	double load_type;
};

/**
 * Peek connection and retrieve message info. Then, read the appropriate
 * amount of data and message type return the message string for parsing.
 */
char *recv_protocol_message(struct cnx_info_t *info, int *mtype, int timeout, int from_client);

/**
 * Just call the lower-level write_data method to send a message.
 */
int xmit_protocol_message(struct cnx_info_t *info, char *message, int to_server);

/*************************************/

/**
 * Generate an LREQ message. This is a message with 0 content length.
 */
char *generate_load_request();

/**
 * Parses an LREQ request
 */
int parse_load_request(char *message);

/**
 * Generates an LRSP message
 */
char *generate_load_response(struct load_info_t *);

/**
 * Parses an LRSP response message and returns load information
 */
struct load_info_t *parse_load_response(char *message);

/*************************************/

char *msg_to_lower_case(char *msg, int len);

#endif

