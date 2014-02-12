/**
 * conffile.c -- Configuration file parser. It parses files with the following 
 * format: 
 * 
 * # comment
 * key1 value1
 * key2 value2
 * ....
 *
 * Keys are case-insensitive.
 *
 * Copyright (C) 2011 Pantelis A. Frangoudis <pfrag@aueb.gr>
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

#include "conffile.h"

int parse_line(char *line, char *key, char *value) {
	int len = strlen(line);

	int i = 0;
	int j = 0;
	int k = 0;

	/* removes space from the beginning of line */
	for (i=0; i<len; i++) {
		if ( !isspace(line[i]) ) {
			break;
		}
	}

	/* if its a comment ---> do nothing */
	if (line[i] == '#') {
		return 0;
	}

	/* if its an empty line, do nothing */
	if (line[i] == '\0') {
		return 0;
	}

	/* gets key */
	for (j=i; j<len; j++) {
		if ( isspace(line[j]) ) {
			break;
		}
		key[k] = line[j];
		k++;
	}

	/* removes trailing space */
	while (isspace(line[j]) && j<len) {
		j++;
	}

	/* key without value */
	if (j == len) {
		*value = '\0';
		return 1;
	}

	/* the rest of it is the value */
	strncpy(value, line + j, len - j);

	/* remove trailing whitespace from value (replace them with \0) */
	for (i=(strlen(value) - 1); i>=0; i--) {
		if (isspace(value[i])) {
				value[i] = '\0';
		}
		else {
				break;
		}
	}
	return 2;
}

int parse_conffile(char *fname, char **confoptions, char **confvalues, int optlen) {
	FILE *fp;
	char *line;
	char key[80];
	char value[80];
	size_t linelen;
	int i;
	int ret;

	fp = fopen(fname, "r");
	if (!fp) {
		return -1;
	}

	linelen = 0;
	line = NULL;
	while ( (linelen = getline(&line, &linelen, fp)) != -1) {
		if (!line) continue;

		/* remove newline character from line */
		for (i = linelen - 1; i >= 0; i--) {
			if (line[i] != '\r' && line[i] != '\n') break;
			line[i] = '\0';
		}

		memset(key, 0, 80);
		memset(value, 0, 80);

		ret = parse_line(line, key, value);

		/* if it's a comment, ignore line */
		if (!ret) {
			if (line) free(line);
			line = NULL;
			continue;
		}
		
		for (i = 0; i < optlen; i++) {
			if (!strcasecmp(confoptions[i], key)) {
				if (*confvalues[i]) {
					/* redefinition of a key */
					if (line) free(line);
					return -3;
				}
				strncpy(confvalues[i], value, strlen(value));
				break;			
			}
		}
		if (i == optlen) {
			/* invalid configuration option */
			if (line) free(line);
			return -2;
		}

		if (line) free(line);
		line = NULL;
	}

	if (line) free(line);

	fclose(fp);

	return 0;
}

