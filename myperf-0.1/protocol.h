/**
 * protocol.h
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
#include "stats.h"

#define MTYPE_STRT				10
#define MTYPE_STOP				20
#define MTYPE_STAT				30
#define MTYPE_DATA				40

#define STRT_HDR "STRT"
#define STOP_HDR "STOP"
#define STAT_HDR "STAT"

#define PEEK_DATA_LEN			30

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
 * Generate a STRT message
 */
char *generate_strt_msg();

/**
 * Generates a STOP message
 */
char *generate_stop_msg();

/**
 * Generates a STAT request
 */
char *gererate_stat_req();

/**
 * Generates a STAT response
 */
char *generate_stat_rsp(struct exp_stats_t *stats);

/**
 * Generate a packet with len bytes
 */
char *generate_data(int len);

/**
 * Parse a STAT response
 */
struct exp_stats_t *parse_stat_rsp(char *msg);

/**
 * Data message starts with the data length. Just parse and return it.
 */
int parse_data(char *message);

/*************************************/

char *msg_to_lower_case(char *msg, int len);

#endif

