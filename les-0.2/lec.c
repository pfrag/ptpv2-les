/**
 * lec.c -- Simple client for LES testing.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    int mtype;
    int ret;
    char *message;
	struct load_info_t *linfo = NULL;
    struct cnx_info_t info;

    /* init connection with LES */
    info.proto = _PROTO_UDP_;
    strcpy(info.host, "127.0.0.1");
    info.port = 7575;
    ret = init_client_connection(&info);
    if (ret < 0) {
		return 1;
    }

    /* send request */
    message = generate_load_request();
    xmit_protocol_message(&info, message, TO_SERVER);
    free(message);

    /* receive load information */
    message = recv_protocol_message(&info, &mtype, 0, FROM_SERVER);
	if (message) {
	    linfo = parse_load_response(message);
		fprintf(stderr, "%s", message);
	}

	if (linfo) {
		free(linfo);
	}
	if (message) {
		free(message);
	}

	return 0;
}

