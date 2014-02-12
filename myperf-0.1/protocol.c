/**
 * protocol.c
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


#include "protocol.h"
#include "netfunc.h"
#include "stats.h"

/**
 * Converts the message body to lower case
 */
char *msg_to_lower_case(char *msg, int len) {
	char *retval;
	int i;

	retval = (char *)malloc(len + 1);
	memset(retval, 0, len + 1);
	
	memcpy(retval, msg, 4);

	for (i = 4; i < len; i++) {
		retval[i] = (char)tolower((unsigned char)msg[i]);
	}
	return retval;
}

/**
 * Peek connection and retrieve message info. Then, read the appropriate
 * amount of data and message type and return the message string for parsing.
 */
char *recv_protocol_message(struct cnx_info_t *info, int *mtype, int timeout, int from_client) {
	int clen;
	int msize;
	char data[PEEK_DATA_LEN];
	char *ldata;
	memset(data, 0, PEEK_DATA_LEN);

	char *message;

	int hdr_len = peek_data(info, data, PEEK_DATA_LEN, timeout, from_client);
	if (hdr_len < 4) {
		return NULL;
	}

	ldata = msg_to_lower_case(data, hdr_len);

	/* read message type */
	if (!strncasecmp(data, STRT_HDR, strlen(STRT_HDR))) {
		*mtype = MTYPE_STRT;
		msize = 4;
	}
	else
	if (!strncasecmp(data, STOP_HDR, strlen(STOP_HDR))) {
		*mtype = MTYPE_STOP;
		msize = 4;
	}
	else if (!strncasecmp(data, STAT_HDR, strlen(STAT_HDR))) {
		*mtype = MTYPE_STAT;
		if (sscanf(ldata, STAT_HDR"\r\ncontent-length: %d", &clen) == 1) {
			msize = strlen(STAT_HDR"\r\ncontent-length: ") + (clen?(int)log10(clen):0) + 3 + clen;
		}
		else {
			msize = 4;
		}
	}
	else {
		//data packet
		if (sscanf(ldata, "%d", &msize) != 1) {
			//no length information, ignore data packet
			if (ldata) free(ldata);
			return NULL;
		}
		*mtype = MTYPE_DATA;
	}

	if (ldata) free(ldata);

	message = (char*)malloc(msize + 1);
	memset(message, 0, msize + 1);

	/* read message */
	if (read_data(info, message, msize, from_client) < 1) {
		if (message) free(message);
		return NULL;
	}
	return message;
}

/**
 * Just call the lower-level write_data method to send a message.
 */
int xmit_protocol_message(struct cnx_info_t *info, char *message, int to_server) {
	return write_data(info, (void *)message, strlen(message), to_server);
}

/**
 * Generate a STRT message
 */
char *generate_strt_msg() {
	char *retval = (char *)malloc(5);
	memset(retval, 0, 5);
	strncpy(retval, STRT_HDR"\0", 5);

	return retval;
}

/**
 * Generate a STOP message
 */
char *generate_stop_msg() {
	char *retval = (char *)malloc(5);
	memset(retval, 0, 5);
	strncpy(retval, STOP_HDR"\0", 5);

	return retval;
}

/**
 * Generate a STAT request
 */
char *generate_stat_req() {
	char *retval = (char *)malloc(5);
	memset(retval, 0, 5);
	strncpy(retval, STOP_HDR"\0", 5);

	return retval;
}

/**
 * Generate a STAT response.
 */
char *generate_stat_rsp(struct exp_stats_t *stats) {
	int mlen;
	int clen;
	int pos;
	char *retval;
	char str_bytes_rcv[32];
	char str_pkt_rcv[32];

	mlen = 0;
	clen = 0;

	mlen += strlen(STAT_HDR"\r\nContent-length: \r\n"); /* clen will be added at the end */
	mlen += strlen("Bytes-rcv: ");
	mlen += strlen("Pkt-rcv: ");

	clen += strlen("Bytes-rcv: ");
	clen += strlen("Pkt-rcv: ");

	memset(str_bytes_rcv, 0, 32);
	sprintf(str_bytes_rcv, "%lld", stats->bytes_rx);
	memset(str_pkt_rcv, 0, 32);
	sprintf(str_pkt_rcv, "%lld", stats->pkt_rx);

	mlen += strlen(str_bytes_rcv) + 2;
	mlen += strlen(str_pkt_rcv) + 2;
	clen += strlen(str_bytes_rcv) + 2;
	clen += strlen(str_pkt_rcv) + 2;

	mlen += (int)log10(clen) + 1; /* content-length digits */

	retval = (char *)malloc(mlen + 1);
	memset(retval, 0, mlen + 1);

	sprintf(
		retval, 
		STAT_HDR"\r\nContent-Length: %d\r\nBytes-rcv: %s\r\nPkt-rcv: %s\r\n",
		clen, str_bytes_rcv, str_pkt_rcv);

	return retval;
}

/**
 * Parse a STAT response
 */
struct exp_stats_t *parse_stat_rsp(char *message) {
	int clen;
	char *lmessage;
	struct exp_stats_t *retval;

	if (!message || strlen(message) < strlen(STAT_HDR"\r\nContent-Length: ")) {
		return NULL;
	}

	if (strncasecmp(message, STAT_HDR"\r\nContent-Length: ", strlen(STAT_HDR"\r\nContent-Length: "))) {
		return NULL;
	}

	/* convert message to lowercase */
	lmessage = msg_to_lower_case(message, strlen(message));
	if (!lmessage) {
		return NULL;
	}

	retval = (struct exp_stats_t*)malloc(sizeof(struct exp_stats_t));
	memset(retval, 0, sizeof(struct exp_stats_t));

	/* parse */
	if (
		sscanf(
			lmessage, 
			STAT_HDR"\r\ncontent-length: %d\r\nbytes-rcv: %lld\r\npkt-rcv: %lld\r\n",
			&clen, &retval->bytes_rx, &retval->pkt_rx
		) != 3 ) {
		free(lmessage);
		return NULL;
	}

	if (lmessage) free(lmessage);

	return retval;
}

/**
 * Generate a data message. It's dummy data starting with the data length.
 */
char *generate_data(int len) {
	char *retval = (char*)malloc(len + 1);
	memset(retval, 'Z', len + 1);
	sprintf(retval, "%d", len);
	retval[(int)log10(len) + 1] = 'Z';
	retval[len] = '\0';
	return retval;
}

/**
 * Data message starts with the data length. Just parse and return it.
 */
int parse_data(char *message) {
	int retval;
	char *end;

	if (!message) {
		return -1;
	}

	/* parse */
	retval = strtol(message, &end, 10);

	return retval;
}

