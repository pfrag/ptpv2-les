/**
 * conffile.h -- Configuration file parser. It parses files with the following 
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

#ifndef _CONFFILE_H_
#define _CONFFILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

int parse_line(char *line, char *key, char *value);
int parse_conffile(char *fname, char **confoptions, char **confvalues, int optlen);

#endif
