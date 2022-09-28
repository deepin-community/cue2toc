
/* conf.c - configuration file parsing
 * Copyright (C) 2004 Matthias Czapla <dermatsch@gmx.de>
 *
 * This file is part of cue2toc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include "error.h"
#include "conf.h"
#include "config.h"
#include "convert.h"

/* Length of longest config option, incl. terminating Null */
#define MAX_OPT_LEN  10

/* Comment character */
#define COMMENT_CHAR '#'

/* Options for use by other modules. Set by config. */
int convert = 1;
int cdtext = 1;
int quiet = 0;
int debug = 0;

/* Our configuration file */
static char *conffile = "~/.cue2tocrc";

/* The valid options */
enum opt { 
	 QUIET,   CDTEXT,   CONVERT,   CONVERTER, E_O_F, UNKNOWN };
static const char opts[][MAX_OPT_LEN] = {
	"QUIET", "CDTEXT", "CONVERT", "CONVERTER" };

/* For error messages while parsing config file */
static int line = 1;

/* Ignore comments and whitespace and get the next option. Eat the
   equal sign so that get_string() doesnt have to (arent we nice?) */
enum opt
get_opt(FILE *f)
{
	int c;
	char buf[MAX_OPT_LEN + 1];
	int i = 0;

	/* eat whitespace and comments */
	do {
		c = fgetc(f);
		if (c == '\n') line++;
		if (c == COMMENT_CHAR) {
			while (c != '\n' && c != EOF)
				c = fgetc(f);
			if (c == '\n') line++;
		}
	} while (isspace(c));
	if (c == EOF) return E_O_F;

	while (!isspace(c) && c != '=' && c != EOF && i < MAX_OPT_LEN) {
		buf[i++] = toupper(c);
		c = fgetc(f);
	}
	buf[i] = '\0';

	if (c == '\n') line++;

	if (c == EOF)
		err_quit("%s:%d: Premature end of file", conffile, line);
	if (!isspace(c) && c != '=')
		return UNKNOWN;
	while (isspace(c))
		if ((c = fgetc(f)) == '\n')
			line++;
	if (c != '=')
		err_quit("%s:%d: Syntax error (epected '=')", conffile, line);

	if	(strcmp(buf, opts[QUIET]) == 0)		return QUIET;
	else if	(strcmp(buf, opts[CDTEXT]) == 0)	return CDTEXT;
	else if (strcmp(buf, opts[CONVERT]) == 0)	return CONVERT;
	else if (strcmp(buf, opts[CONVERTER]) == 0)	return CONVERTER;
	else						return UNKNOWN;
}

/* Add character c to string *s. Grow *s as needed. len is the current size
   of *s, pos the position where to put c. Return new size of *s. */
size_t
add_char(char **s, size_t len, size_t pos, char c)
{
	size_t sz = 4;	/* must be >= 2 */
	size_t new_len = len;

	if (len == 0)
		if ((*s = malloc((new_len = sz))) == NULL)
			err_quit("Memory allocation error");
	while (pos > new_len - 2) {
		new_len += sz;
		if ((*s = realloc(*s, new_len)) == NULL)
			err_quit("Memory allocation error");
	}
	(*s)[pos] = c;
	(*s)[pos + 1] = '\0';

	return new_len;
}

/* Get and return the string option argument */
static char*
get_string(FILE *f)
{
	char *s = NULL;
	size_t len = 0;
	size_t pos = 0;
	int c;
	int quoted = 0;

	/* eat whitespace and comments */
	do {
		c = fgetc(f);
		if (c == '\n') line++;
	} while (isspace(c));

	if (c == '"') {
		quoted = 1;
		c = fgetc(f);
		if (c == '\n') line++;
	}

	/* If the value starts with a double quote we stop reading only if
	   we encounter the closing quote, unless it is preceded by a
	   backslash in which case the double quote is put in the place of
	   the backslash.
	   If the value is not quoted we stop at the first whitespace or EOF.
	 */
	for (;;) {
		if (c == '"' && quoted) {
			if (pos > 0 && s[pos - 1] == '\\')
				len = add_char(&s, len, pos - 1, c);
			else {
				len = add_char(&s, len, pos, '\0');
				break;
			}
		} else if (isspace(c) && !quoted) {
			len = add_char(&s, len, pos, '\0');
			break;
		} else if (c == EOF) {
			if (quoted)
				err_quit("%s:%d: Premature end of file",
					conffile, line);
			else {
				len = add_char(&s, len, pos, '\0');
				break;
			}
		} else
			len = add_char(&s, len, pos++, c);

		c = fgetc(f);
		if (c == '\n') line++;
	}

	return s;
}

/* Return true if s equals one of "yes", "y", "true" or "1" (case
   insensitive) */
static int
yes(char *s)
{
	char *tmp = s;

	while (*tmp) {
		*tmp = tolower(*tmp);
		tmp++;
	}

	if (strcmp(s, "yes") == 0 || strcmp(s, "y") == 0
	    || strcmp(s, "true") == 0 || strcmp(s, "1") == 0)
		return 1;

	return 0;
}

/* Parse the configuration file and set things like the de/encryption
   commands etc. (well, actually there is nothing more currently) */
static void
parse_file(const char *file)
{
	FILE *f;
	enum opt option;
	char *s;
	struct converter *converter;

	if ((f = fopen(file, "r")) == NULL)
		err_sys("Could not open file \"%s\" for reading", file);

	option = get_opt(f);
	while (option != E_O_F) {
		switch (option) {
		case QUIET:
			if (debug) fprintf(stderr, "Option %s: ", opts[QUIET]);

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires argument "
					 "-- %s\n", conffile, line,
					 opts[QUIET]);
			quiet = yes(s);
			free(s);

			if (debug) fprintf(stderr, "%s\n", quiet ? "yes" :"no");
			break;
		case CDTEXT:
			if (debug) fprintf(stderr, "Option %s: ", opts[CDTEXT]);

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires argument "
					 "-- %s\n", conffile, line,
					 opts[CDTEXT]);
			cdtext = yes(s);
			free(s);

			if (debug) fprintf(stderr, "%s\n", cdtext ? "yes":"no");
			break;
		case CONVERT:
			if(debug)fprintf(stderr,"Option %s: ",opts[CONVERT]);

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires argument "
					 "-- %s\n", conffile, line,
					 opts[CONVERT]);
			convert = yes(s);
			free(s);

			if (debug) fprintf(stderr, "%s\n", convert ?"yes":"no");
			break;
		case CONVERTER:
			if (debug) fprintf (stderr, "Option %s: ",
					    opts[CONVERTER]);

			converter = new_converter();

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires three "
					 "arguments -- %s\n", conffile, line,
					 opts[CONVERTER]);
			converter->ext_from = s;

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires three "
					 "arguments -- %s\n", conffile, line,
					 opts[CONVERTER]);
			converter->ext_to = s;

			if (strlen(s = get_string(f)) == 0)
				err_quit("%s:%d: Option requires three "
					 "arguments -- %s\n", conffile, line,
					 opts[CONVERTER]);
			converter->command = s;

			add_converter(converter);

			if (debug) fprintf (stderr, "\"%s\" \"%s\" \"%s\"\n",
					    converter->ext_from,
					    converter->ext_to,
					    converter->command);
			break;
		case UNKNOWN:
			err_quit("%s:%d: Unknown option", conffile, line);
			break;
		default:
			err_quit("You found a bug. This get_opt return value "
				 "cannot be handled.");
		}
		option = get_opt(f);
	}

	if (debug) fprintf(stderr, "Done parsing config file\n");

}

/* Construct absolute pathnames of configuration file and parse it. */
void
config(void)
{
	char *conf = NULL;
	struct stat statbuf;

	/* set dir to absolute path of our directory */
	if (conffile[0] == '~') {
		if (getenv("HOME") == NULL)
			err_quit("HOME not defined");
		if ((conf = malloc(strlen(getenv("HOME"))
				  + strlen(conffile))) == NULL)
			err_quit("Memory allocation error");
		strcpy(conf, getenv("HOME"));
		strcat(conf, &conffile[1]);
	} else {
		conf = conffile;
	}

	if (debug) fprintf(stderr, "Parsing config file %s\n", conf);

	if (stat(conf, &statbuf) != -1) {
		parse_file(conf);
	} else
		if (errno != ENOENT) /* Its ok if conffile doesnt exist */
			err_sys("Error stat'ing %s", conffile);
		else if (debug) fprintf(stderr, "No config file\n");
}
