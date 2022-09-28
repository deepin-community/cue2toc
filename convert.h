/* convert.h - convert data files based on extension
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

#ifndef CONVERT_H
#define CONVERT_H
#include "cue2toc.h"	/* for struct trackspec definition */

struct converter {
	char *ext_from;		/* filename extension of file to be converted */
	char *ext_to;		/* filename extension of file to convert to */
	char *command;		/* command that does the conversion */
	struct converter *next;
};

/* The global list of configured converters */
extern struct converter *converters;

/* Allocate and return a new converter struct */
struct converter *new_converter(void);

/* Add a converter to the list */
void add_converter(struct converter*);

/* Loop throuhg tracklist and convert files if a converter is available */
void convert_files(struct trackspec*);

#endif /* CONVERT_H */
