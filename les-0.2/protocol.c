/**
 * protocol.c
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


#include "protocol.h"
#include "netfunc.h"

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
	if (!ldata) {
		return NULL;
	}

	/* read message type */
	if (!strncasecmp(data, LREQ_HDR, strlen(LREQ_HDR))) {
		*mtype = MTYPE_LREQ;
		if (sscanf(ldata, LREQ_HDR"\r\ncontent-length: %d", &clen) != 1) {
			free(ldata);
			return NULL;
		}
		
		msize = strlen(LREQ_HDR"\r\ncontent-length: ") + (clen?(int)log10(clen):0) + 3 + clen;
	}
	else
	if (!strncasecmp(data, LRSP_HDR, strlen(LRSP_HDR))) {
		*mtype = MTYPE_LRSP;
		if (sscanf(ldata, LRSP_HDR"\r\ncontent-length: %d", &clen) != 1) {
			free(ldata);
			return NULL;
		}
		
		msize = strlen(LRSP_HDR"\r\ncontent-length: ") + (clen?(int)log10(clen):0) + 3 + clen;
	}
	else {
		free(ldata);
		return NULL;
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
 * Generate an LREQ message, with 0 content length
 */
char *generate_load_request() {
	char *retval = (char *)malloc(26);
	memset(retval, 0, 26);
	strncpy(retval, LREQ_HDR"\r\ncontent-length: 0\r\n", 25);

	return retval;
}

/**
 * Parses a load request: Assumes an LREQ message with zero content len.
 */
int parse_load_request(char *message) {
        if (!message || strlen(message) < 25) {
                return -1;
        }
        if (!strncasecmp(LREQ_HDR"\r\ncontent-length: 0", message, 25)) {
                return 0;
        }
        else {
                return -2;
        }
}

/**
 * Generate an LREQ message.
 */
char *generate_load_response(struct load_info_t *linfo) {
	int mlen;
	int clen;
	int pos;
	char *retval;
	char str_delay_avg[32];

	mlen = 0;
	clen = 0;

	mlen += strlen(LRSP_HDR"\r\nContent-length: \r\n"); /* clen will be added at the end */
	mlen += strlen("Status: ");
	mlen += strlen("Delay-avg: ");
	mlen += strlen("Delay-min: ");
	mlen += strlen("Delay-max: ");
	mlen += strlen("Samples: ");
	mlen += strlen("Load-type: ");

	clen += strlen("Status: ");
	clen += strlen("Delay-avg: ");
	clen += strlen("Delay-min: ");
	clen += strlen("Delay-max: ");
	clen += strlen("Samples: ");
	clen += strlen("Load-type: ");

	memset(str_delay_avg, 0, 32);
	sprintf(str_delay_avg, "%f", linfo->weighted_avg);

	mlen += (int)(linfo->status?log10(linfo->status):0) + 3;
	mlen += strlen(str_delay_avg) + 2;
	mlen += (int)(linfo->min?log10l(linfo->min):0) + 3;
	mlen += (int)(linfo->max?log10l(linfo->max):0) + 3;
	mlen += (int)(linfo->nsamples?log10l(linfo->nsamples):0) + 3;
	mlen += 7; //load type is a floating point num with 3 decimal dgts: X.XXX

	clen += (int)(linfo->status?log10(linfo->status):0) + 3;
	clen += strlen(str_delay_avg) + 2;
	clen += (int)(linfo->min?log10l(linfo->min):0) + 3;
	clen += (int)(linfo->max?log10l(linfo->max):0) + 3;
	clen += (int)(linfo->nsamples?log10l(linfo->nsamples):0) + 3;
	clen += 7; //load type is a floating point num with 3 decimal dgts: X.XXX

	mlen += (int)log10(clen) + 1; /* content-length digits */

	retval = (char *)malloc(mlen + 1);
	memset(retval, 0, mlen + 1);

	double theload = linfo->load_type;
	if (linfo->load_type < 0) theload = 0.0;

	sprintf(
		retval, 
		LRSP_HDR"\r\nContent-Length: %d\r\nStatus: %d\r\nDelay-avg: %s\r\nDelay-min: %lld\r\nDelay-max: %lld\r\nSamples: %d\r\nLoad-type: %1.3f\r\n",
		clen, linfo->status, str_delay_avg, linfo->min, linfo->max, linfo->nsamples, theload);

	return retval;
}

/**
 * Parse an LRSP message and return load information.
 */
struct load_info_t *parse_load_response(char *message) {
	int clen;
	char *lmessage;
	struct load_info_t *retval;

	if (!message || strlen(message) < strlen(LRSP_HDR"\r\nContent-Length: ")) {
		return NULL;
	}

	if (strncasecmp(message, LRSP_HDR"\r\nContent-Length: ", strlen(LRSP_HDR"\r\nContent-Length: "))) {
		return NULL;
	}

	/* convert message to lowercase */
	lmessage = msg_to_lower_case(message, strlen(message));
	if (!lmessage) {
		return NULL;
	}

	retval = (struct load_info_t*)malloc(sizeof(struct load_info_t));
	memset(retval, 0, sizeof(struct load_info_t));

	/* parse */
	if (
		sscanf(
			lmessage, 
			LRSP_HDR"\r\ncontent-length: %d\r\nstatus: %d\r\ndelay-avg: %lf\r\ndelay-min: %Ld\r\ndelay-max: %Ld\r\nsamples: %d\r\nload-type: %lf\r\n",
			&clen, &retval->status, &retval->weighted_avg, &retval->min, &retval->max, &retval->nsamples, &retval->load_type
		) != 7 ) {
		free(lmessage);
		return NULL;
	}

	return retval;
}

