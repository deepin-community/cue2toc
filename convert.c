/* convert.c - convert data files based on extension
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

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "convert.h"
#include "error.h"
#include "conf.h"
#include "cue2toc.h"

/* The global list of configured converters */
struct converter *converters = NULL;

/* For debugging info the number of the current track */
static int trackno = 1;

/* Allocate, initialize and return a new converter structure */
struct converter*
new_converter(void)
{
	struct converter *converter;

	if ((converter = malloc(sizeof(struct converter))) == NULL)
		err_quit("Memory allocation error");

	converter->ext_from = converter->ext_to = converter->command = NULL;
	converter->next = NULL;

	return converter;
}

/* Add a new converter to global list of converters */
void
add_converter(struct converter *converter)
{
	struct converter *tmp;

	if (converters == NULL) {
		converters = converter;
		return;
	}

	tmp = converters;

	while (tmp->next != NULL)
		tmp = tmp->next;

	tmp->next = converter;
	tmp->next->next = NULL;		/* just to be safe */
}

/* Find a converter for filename s */
struct converter*
find_converter(const char *s)
{
	char *ext;
	struct converter *converter = converters;
	int i;

	
	while (converter != NULL) {
		if (strlen(s) <= strlen(converter->ext_from)) {
			converter = converter->next;
			continue;
		}

		if ((ext = malloc(strlen(converter->ext_from) + 1)) == NULL)
			err_quit("Memory allocation error");

		strcpy(ext, &s[strlen(s) - strlen(converter->ext_from)]);

		for (i = 0; i < strlen(ext); i++)
			ext[i] = tolower(ext[i]);

		if (strcmp(ext, converter->ext_from) == 0) {
			free(ext);
			return converter;
		}

		free(ext);
		converter = converter->next;
	}

	return NULL;	/* No matching converter found */
}


/* Convert file of this track if a converter is available */
void
do_track(struct trackspec *tr)
{
	struct converter *converter;
	char *c2t_from_prefix = "C2T_FROM=";
	char *c2t_to_prefix   = "C2T_TO=";
	char *c2t_from;
	char *c2t_to;
	char *c2t_to_filename;	/* needed to check if exists */
	struct stat statbuf;
	int status;

	if ((converter = find_converter(tr->filename)) == NULL)
		return;		/* No converter for this filename */

	if (debug) fprintf(stderr, "Found a converter for track %d: %s\n",
			   trackno, converter->command);

	if ((c2t_from = malloc(strlen(c2t_from_prefix)
			       + strlen(tr->filename) + 1)) == NULL)
		err_quit("Memory allocation error");
	if ((c2t_to = malloc(strlen(c2t_to_prefix) + strlen(tr->filename)
			     - strlen(converter->ext_from)
			     + strlen(converter->ext_to) + 1)) == NULL)
		err_quit("Memory allocation error");
	if ((c2t_to_filename = malloc(strlen(tr->filename)
			     - strlen(converter->ext_from)
			     + strlen(converter->ext_to) + 1)) == NULL)
		err_quit("Memory allocation error");

	/* Construct environment string for C2T_FROM */
	strcpy(c2t_from, c2t_from_prefix);
	strcat(c2t_from, tr->filename);

	/* Construct environment string for C2T_TO */
	strcpy(c2t_to, c2t_to_prefix);
	strncat(c2t_to, tr->filename,
		strlen(tr->filename) - strlen(converter->ext_from));
	strcat(c2t_to, converter->ext_to);

	/* Construct filename of C2T_TO file for existence check */
	strncpy(c2t_to_filename, tr->filename, strlen(tr->filename)
					       - strlen(converter->ext_from));
	c2t_to_filename[strlen(tr->filename)
			- strlen(converter->ext_from)] = '\0';
	strcat(c2t_to_filename, converter->ext_to);

	if (stat(c2t_to_filename, &statbuf) != -1) { /* it exists */
		if (debug) fprintf(stderr, "%s already exists\n",
				   c2t_to_filename);
		free(c2t_from);
		free(c2t_to);
		strcpy(tr->filename, c2t_to_filename);
		free(c2t_to_filename);

		return;
	}

	if (debug) fprintf(stderr, "Exporting \"%s\"\n", c2t_from);
	if (putenv(c2t_from) == -1)
		err_sys("putenv error");

	if (debug) fprintf(stderr, "Exporting \"%s\"\n", c2t_to);
	if (putenv(c2t_to) == -1)
		err_sys("putenv error");

	/* Execute the conversion command */
	if (!quiet)
		fprintf(stderr, "%s: Track %d: Converting \"%s\" to \"%s\"\n",
			prog, trackno, tr->filename, c2t_to_filename);

	if ((status = system(converter->command)) == -1)
		err_sys("system error");

	if (debug) fprintf(stderr, "Conversion command returned with "
			   "exit status %d\n", WEXITSTATUS(status));

	/* Replace filename in cuesheet with the new one */
	strcpy(tr->filename, c2t_to_filename);

	/* We cant free the strings c2t_from and c2t_to because they are
	   part of the environement. */
	free(c2t_to_filename);
}

/* Loop through tracklist and convert files if a converter is available */
void
convert_files(struct trackspec *tr)
{
	if (debug) fprintf(stderr, "Converting files\n");

	while (tr != NULL) {
		do_track(tr);
		trackno++;
		tr = tr->next;
	}
}
