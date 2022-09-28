/* main.c - handle command line arguments
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "cue2toc.h"
#include "error.h"
#include "conf.h"
#include "convert.h"

void usage(void);

int
main(int argc, char *argv[])
{
	char *outfile = NULL;
	char *infile = NULL;
	int c;

	struct cuesheet *cs;

	prog = argv[0];
	opterr = 0;	/* we do error msgs ourselves */

	config();	/* Read configuration file */

	while ((c = getopt(argc, argv, ":dhno:qv")) != -1)
		switch (c) {
		case 'd': debug = 1; break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
			break;
		case 'n': cdtext = 0; break;	/* for compatibility */
		case 'o':
			if (strcmp(optarg, "-") == 0)
				outfile = NULL;		/* use stdout */
			else
				outfile = optarg;
			break;
		case 'q': quiet = 1; break;
		case 'v':
			printf("%s\n", PACKAGE_STRING);
			printf("Report bugs to <%s>\n", PACKAGE_BUGREPORT);
			exit(EXIT_SUCCESS);
			break;
		case ':': err_quit("Options requires argument -- %c", optopt);
		case '?': err_quit("Illegal option -- %c", optopt);
		default: err_quit("Unhandled command line option. D'oh!");
		}
	

	switch(argc - optind) {
	case 0:
		infile = NULL;	/* use stdin */
		break;
	case 1:
		if (strcmp(argv[optind], "-") == 0)
			infile = NULL;	/* use stdin */
		else
			infile = argv[optind];
		break;
	default:
		err_quit("Too many arguments");
	}

	cs = read_cue(infile);
	if (convert)
		convert_files(cs->tracklist);
	write_toc(outfile, cs);

	return EXIT_SUCCESS;
}

void
usage(void)
{
	printf("Usage: %s [-dhqv] [-o tocfile] [cuefile]\n",
	       prog);
	printf("  -d\t\tprint debugging info\n");
	printf("  -h\t\tdisplay this help message\n");
	printf("  -o tocfile\twrite output to tocfile\n");
	printf("  -q\t\tbe quiet\n");
	printf("  -v\t\tdisplay version information\n");
}
