/**
 * b64.c -- Base64 encoder/decoder 
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

#include "b64.h"

/**
 * Calculates the size of a b64 encoded structure
 *(after the encoding the size is about 33% higher...)
 */
int calculate_b64_size(int len) {
	switch (len % 3) {
		case 1:
			return (4.0/3.0*(len+2) + 2*(( 4.0/3.0*(len+2) )/(double)B64_LINE_LEN + 1)); //+1: final crlf, 2*...because crlf=2bytes
		case 2:
			return (4.0/3.0*(len+1) + 2*(( 4.0/3.0*(len+1) )/(double)B64_LINE_LEN + 1));
		case 0:
			return (4*(len/3) + 2*(4.0*(len/3)/(double)B64_LINE_LEN + 1));
	}
	return 0;
}

/**
 * Returns true if c is a character that should be ignored during b64 decoding
 * that is, tab, space, cr, lf, =
 */
int should_be_ignored(unsigned char c) {
	if ( (c == 10) || (c == 13) || (c == 32) || (c == 9) ) {
		return 1;
	}
	return 0;
}

/**
 * Reads b64 data and removes blank/crfl characters & returns translated b64 
 * byte array. b64 bytes are translated to the original byte values 
 * (e.g. "A" ---> 0, "B" ---> 1, "0" ---> 52).
 * In case of error (e.g. non-ascii char, char that doesn't belong to the 
 * translation table), it returns NULL. Otherwise, sets outlen to the size of
 * the returned array
 */
unsigned char* translate_b64(unsigned char* data, int inlen, int *outlen) {
	int i = 0;
	int j = -1;
	unsigned char* translated = (unsigned char*)malloc(inlen);
	memset(translated, 0, inlen);
	for (i=0;i<inlen;i++) {
		if (!should_be_ignored(data[i])) {
			//not a whitespace
			j++;

			//we could use something like a lookup table,etc...
			if (  (data[i] >= 65) && (data[i] <= 90) ) {
				//capital A-Z
				translated[j] = data[i] - 65;
			}
			else
			if  ((data[i] >= 97) && (data[i] <= 122)) {
				//lowercase a-z
				translated[j] = data[i] - 71;
			}
			else
			if ((data[i] >= 48) && (data[i] <= 57)) {
				//0-9
				translated[j] = data[i] + 4;
			}
			else
			if (data[i] == 43) {
				//+
				translated[j] = 62;
			}
			else
			if (data[i] == 47) {
				// /
				translated[j] = 63;
			}
			else
			if (data[i] == 61) {
				//padding(=)
				translated[j] = 0;
			}
			else {
				//data error
				if (translated) free(translated);
				return NULL;
			}
		} //else ignore it...
	}
	*outlen = j + 1;
	return translated;
}
/**
 * Encodes a 3byte block to a 4-byte b64-encoded block
 * len: 1,2,3: if len<3 we must pad with =
 */
void encode_block(unsigned char in[3], unsigned char out[4], int len) {
	out[0] = EncTable[ in[0] >> 2 ];
	out[1] = EncTable[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
	out[2] = (unsigned char) (len > 1 ? EncTable[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
	out[3] = (unsigned char) (len > 2 ? EncTable[ in[2] & 0x3f ] : '=');
}

/**
 * Decodes a 4byte b64 block to raw bytes
 */
void decode_block(unsigned char in[4], unsigned char out[3]) {
	out[0] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
	out[1] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
	out[2] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/**
 * b64-encodes a raw byte array of length == datalen.
 * Data is output in lines of B64_LINE_LEN characters
 * datalen: length of raw data byte array
 */
unsigned char *b64_encode(unsigned char *raw, int datalen) {
	int i = 0;
	int curpos = 0; //current position in b64 byte array (i.e. how many bytes we have appended to b64)
	int charsinline = 0; //how many chars are in the current line

	unsigned char *b64 = (unsigned char*)malloc(calculate_b64_size(datalen) + 1);
	memset(b64, 0, calculate_b64_size(datalen) + 1);

//	for (i = 0; i<datalen-3; i+=3) {
	for (i = 0; i < datalen / 3; i++) {
		encode_block(raw + i*3, b64 + curpos, 3);
		curpos+=4;
		charsinline += 4;
		if (charsinline == B64_LINE_LEN) {
			//we suppose that B64_LINE_LEN is multiple of 4
			charsinline = 0;
			b64[curpos] = '\r';
			b64[curpos+1] = '\n';
			curpos+=2; //added crlf
		}
	}
	if (datalen % 3) {
		//if we have spare characters because datalen is not multiple of 3...
		encode_block(raw + i*3, b64 + curpos, datalen % 3);
	}
//	else {
//		encode_block(raw + i*3, b64 + curpos, 3);
//	}
	
	return b64;
}

/**
 * Decodes the b64 data ---> raw
 */
unsigned char* b64_decode(unsigned char *b64, int *outlen) {
	//Call translate_b64 to remove whitespace+crlf characters and padding
	//and to convert b64 ---> original byte values
	//and then operate on "translated"
	int rawpos = 0;
	int i = 0;
	int len = 0; //length of the "processed" b64 array
	int inlen = strlen(b64);
	unsigned char *translated = translate_b64(b64, inlen, &len);

	unsigned char *raw = (unsigned char*)malloc(strlen(b64)); //malloc more space than necessary
	memset(raw, 0, strlen(b64));

	if (!translated) {
		//data error - translate_b64  returned null...
		if (raw) free(raw);
		return NULL;
	}

	for (i=0; i<len; i+=4) {
		decode_block(translated + i, raw + rawpos);
		rawpos+=3;
	}

	if (strstr(b64, "==")) {
		rawpos-=2;
	}
	else
	if (strstr(b64, "=")) {
		rawpos--;
	}

	*outlen = rawpos;

	free(translated);
	return raw;
}

