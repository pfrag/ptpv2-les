/**
 * b64.h -- Base64 encoder/decoder 
 *
 * Copyright (C) 2004-2012 Pantelis A. Frangoudis <pfrag@aueb.gr>
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


#ifndef _B64_H_
#define _B64_H_

#include <string.h>
#include <stdlib.h>
#ifndef B64_LINE_LEN
#define B64_LINE_LEN 64
#endif

static char EncTable[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Calculates the size of a b64 encoded structure
 *(after the encoding the size is about 33% higher...)
 */
int calculate_b64_size(int len);

/**
 * Returns true if c is a character that should be ignored during b64 decoding
 * that is, tab, space, cr, lf, =
 */
int should_be_ignored(unsigned char c);

/**
 * Reads b64 data and removes blank/crfl characters & returns translated b64 
 * byte array. b64 bytes are translated to the original byte values 
 * (e.g. "A" ---> 0, "B" ---> 1, "0" ---> 52).
 * In case of error (e.g. non-ascii char, char that doesn't belong to the 
 * translation table), it returns NULL. Otherwise, sets outlen to the size of
 * the returned array
 */
unsigned char* translate_b64(unsigned char* data, int inlen, int *outlen);

/**
 * Encodes a 3byte block to a 4-byte b64-encoded block
 * len: 1,2,3: if len<3 we must pad with =
 */
void encode_block(unsigned char in[3], unsigned char out[4], int len);

/**
 * Decodes a 4byte b64 block to raw bytes
 */
void decode_block(unsigned char in[4], unsigned char out[3]);

/**
 * b64-encodes a raw byte array of length == datalen.
 * Data is output in lines of B64_LINE_LEN characters
 * datalen: length of raw data byte array
 */
unsigned char *b64_encode(unsigned char *raw, int datalen);

/**
 * Decodes the b64 data ---> raw
 * ("raw" MUST have been allocated before the call...)
 */
unsigned char* b64_decode(unsigned char *b64, int *outlen);

#endif
