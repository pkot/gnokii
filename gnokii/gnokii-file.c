/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Gnokii is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Gnokii is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with gnokii; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Copyright (C) 1999-2000  Hugh Blemings & Pavel Janik ml.
  Copyright (C) 1999-2000  Gary Reuter, Reinhold Jordan
  Copyright (C) 1999-2006  Pawel Kot
  Copyright (C) 2000-2002  Marcin Wiacek, Chris Kemp, Manfred Jonsson
  Copyright (C) 2001       Marian Jancar, Bartek Klepacz
  Copyright (C) 2001-2002  Pavel Machek, Markus Plail
  Copyright (C) 2002       Ladis Michl, Simon Huggins
  Copyright (C) 2002-2004  BORBELY Zoltan
  Copyright (C) 2003       Bertrik Sikken
  Copyright (C) 2004       Martin Goldhahn

  Mainline code for gnokii utility. File handling functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>

#include "gnokii-app.h"
#include "gnokii.h"

void file_usage(FILE *f)
{
	fprintf(f, _("File options:\n"
		     "          --getfilelist remote_path\n"
		     "          --getfiledetailsbyid [id]\n"
		     "          --getfileid remote_filename\n"
		     "          --getfile remote_filename [local_filename]\n"
		     "          --getfilebyid id [local_filename]\n"
		     "          --getallfiles remote_path\n"
		     "          --putfile local_filename remote_filename\n"
		     "          --deletefile remote_filename\n"
		     "          --deletefilebyid id\n"
		));
}

static void set_fileid(gn_file *fi, char *arg)
{
	unsigned long j, index, len = 1;
	index = j = atol(arg);
	while (j > 255) {
		len++;
		j /= 256;
	}
	if (len % 2)
		len++;
	fi->id = calloc(len + 1, sizeof(char));
	fi->id[0] = len;
	for (j = len; j > 0; j--) {
		fi->id[j] = index % 256;
		index = (index >> 8);
	}
}

/* Get file list. */
gn_error getfilelist(char *path, gn_data *data, struct gn_statemachine *state)
{
    	gn_file_list fi;
	gn_error error;
	int i;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.path, sizeof(fi.path), "%s", path);

	gn_data_clear(data);
	data->file_list = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileList, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"), path, gn_error_print(error));
	else {
		fprintf(stdout, _("Filelist for path %s:\n"), path);
		for (i = 0; i < fi.file_count; i++) {
			fprintf(stdout, _("  %s\n"), fi.files[i]->name);
			free(fi.files[i]);
		}
	}
	return error;
}

gn_error getfiledetailsbyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
    	gn_file_list fil;
	gn_error error;
	int i;

	memset(&fi, 0, sizeof(fi));
	memset(&fil, 0, sizeof(fil));

	gn_data_clear(data);
	data->file = &fi;
	data->file_list = &fil;
	/* default parameter is root == 0x01 */
	if (argc == optind) {
		fi.id = calloc(3, sizeof(char));
		if (!fi.id)
			return GN_ERR_INTERNALERROR;
		fi.id[0] = 2;
		fi.id[1] = 0x00;
		fi.id[2] = 0x01;
	} else {
		set_fileid(&fi, argv[optind]);
	}

	if ((error = gn_sm_functions(GN_OP_GetFileDetailsById, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get filename: %s\n"), gn_error_print(error));
	else {
		int fileid = 0, counter = 16;
		for (i = fi.id[0]; i > 0; i--) {
			fileid += fi.id[i] * (counter / 16);
			counter *= 16;
		}
		fprintf(stdout, _("| %s (%d)\n"), fi.name, fileid);
		for (i = 0; i < fil.file_count; i++) {
			int j;
		    	gn_file fi2;
		    	gn_file_list fil2;
			gn_error error2;

			dprintf("getting %s\n", fil.files[i]->id);
			memset(&fi2, 0, sizeof(fi2));
			memset(&fil2, 0, sizeof(fil2));

			gn_data_clear(data);
			data->file = &fi2;
			data->file_list = &fil2;
			fi2.id = fil.files[i]->id;

			error2 = gn_sm_functions(GN_OP_GetFileDetailsById, data, state);
			switch (error2) {
			case GN_ERR_NONE:
				fileid = 0;
				counter = 16;
				for (j = fi2.id[0]; j > 0; j--) {
					fileid += fi2.id[j] * (counter / 16);
					counter *= 16;
				}
				fprintf(stdout, _(" - %s (%d)\n"), fi2.name, fileid);
				break;
			default:
				fprintf(stderr, _("Failed to get filename: %s\n"), gn_error_print(error));
				break;
			}
		}
	}
	free(fi.id);
	return error;
}

/* Get file id */
gn_error getfileid(char *filename, gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
	gn_error error;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", filename);

	gn_data_clear(data);
	data->file = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileId, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"),filename, gn_error_print(error));
	else {
		fprintf(stdout, _("Fileid for file %s is %02x %02x %02x %02x %02x %02x\n"), filename, fi.id[0], fi.id[1], fi.id[2], fi.id[3], fi.id[4], fi.id[5]);
	}
	return error;
}

/* Delete file */
gn_error deletefile(char *filename, gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
	gn_error error;

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", filename);

	gn_data_clear(data);
	data->file = &fi;

	if ((error = gn_sm_functions(GN_OP_DeleteFile, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to delete %s: %s\n"), filename, gn_error_print(error));
	else {
		fprintf(stderr, _("Deleted: %s\n"), filename);
	}
	return error;
}

/* Delete file */
gn_error deletefilebyid(char *id, gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
	gn_error error;

	memset(&fi, 0, sizeof(fi));
	set_fileid(&fi, id);

	gn_data_clear(data);
	data->file = &fi;

	if ((error = gn_sm_functions(GN_OP_DeleteFileById, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to delete %s: %s\n"), id, gn_error_print(error));
	else {
		fprintf(stderr, _("Deleted: %s\n"), id);
	}
	return error;
}

int getfile_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getfile remote_filename [localfilename]\n"));
	return exitval;
}

/* Get file */
gn_error getfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
	gn_error error;
	FILE *f;
	char filename2[512];

	if (argc < optind)
		return getfile_usage(stderr, -1);

	memset(filename2, 0, 512);
	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", optarg);

	gn_data_clear(data);
	data->file = &fi;

	data->progress_indication = NULL;

	if ((error = gn_sm_functions(GN_OP_GetFile, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get file %s: %s\n"), optarg, gn_error_print(error));
	else {
		if (argc == optind) {
			if (strrchr(optarg, '/'))
				snprintf(filename2, sizeof(filename2), "%s", strrchr(optarg, '/') + 1);
			else if (strrchr(optarg, '\\'))
				snprintf(filename2, sizeof(filename2), "%s", strrchr(optarg, '\\') + 1);
			else
				snprintf(filename2, sizeof(filename2), "default.dat");
			fprintf(stdout, _("Got file %s.  Save to [%s]: "), optarg, filename2);
			gn_line_get(stdin, filename2, 512);
			if (filename2[0] == 0) {
				if (strrchr(optarg, '/'))
					snprintf(filename2, sizeof(filename2), "%s", strrchr(optarg, '/') + 1);
				else if (strrchr(optarg, '\\'))
					snprintf(filename2, sizeof(filename2), "%s", strrchr(optarg, '\\') + 1);
				else
					snprintf(filename2, sizeof(filename2), "default.dat");
			}
		} else {
			snprintf(filename2, sizeof(filename2), "%s", argv[optind]);
		}
		f = fopen(filename2, "w");
		if (!f) {
			fprintf(stderr, _("Can't open file %s for writing!\n"), filename2);
			return GN_ERR_FAILED;
		}
		if (fwrite(fi.file, fi.file_length, 1, f) < 0) {
			fprintf(stderr, _("Failed to write to file %s.\n"), filename2);
			error = GN_ERR_FAILED;
		}
		fclose(f);
		free(fi.file);
	}
	return error;
}

int getfilebyid_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --getfilebyid id [localfilename]\n"));
	return exitval;
}

/* Get file */
gn_error getfilebyid(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
    	gn_file_list fil;
	gn_error error;
	FILE *f;
	char filename2[512];

	if (argc < optind)
		return getfilebyid_usage(stderr, -1);

	memset(filename2, 0, 512);
	memset(&fi, 0, sizeof(fi));
	set_fileid(&fi, optarg);

	gn_data_clear(data);
	data->file = &fi;
	data->file_list = &fil;

	data->progress_indication = NULL;

	if ((error = gn_sm_functions(GN_OP_GetFileDetailsById, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get file: %s\n"), gn_error_print(error));
	else {
		dprintf("File is %s\n", fi.name);
		fi.file = malloc(fi.file_length);
		error = gn_sm_functions(GN_OP_GetFileById, data, state);
		if (error == GN_ERR_NONE) {
			if (argc == optind) {
				snprintf(filename2, sizeof(filename2), "%s", fi.name);
			} else {
				snprintf(filename2, sizeof(filename2), "%s", argv[optind]);
			}
			f = fopen(filename2, "w");
			if (!f) {
				fprintf(stderr, _("Can't open file %s for writing!\n"), filename2);
				return GN_ERR_FAILED;
			}
			if (fwrite(fi.file, fi.file_length, 1, f) < 0) {
				fprintf(stderr, _("Failed to write to file %s.\n"), filename2);
				error = GN_ERR_FAILED;
			}
			fclose(f);
			free(fi.file);
		} else {
			fprintf(stderr, "%s\n", gn_error_print(error));
		}
	}
	return error;
}

/* Get all files */
gn_error getallfiles(char *path, gn_data *data, struct gn_statemachine *state)
{
    	gn_file_list fi;
	gn_error error;
	int i;
	FILE *f;
	char filename2[512];

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.path, sizeof(fi.path), "%s", path);

	gn_data_clear(data);
	data->file_list = &fi;

	if ((error = gn_sm_functions(GN_OP_GetFileList, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to get info for %s: %s\n"), path, gn_error_print(error));
	else {
		char *pos = strrchr(path, '/');
		if (!pos)
			pos = strrchr(path, '\\');

		if (pos)
			*(pos+1) = 0;

		for (i = 0; i < fi.file_count; i++) {
			data->file = fi.files[i];
			snprintf(filename2, sizeof(filename2), "%s", fi.files[i]->name);
			snprintf(fi.files[i]->name, sizeof(fi.files[i]->name), "%s%s", path, filename2);
			if ((error = gn_sm_functions(GN_OP_GetFile, data, state)) != GN_ERR_NONE)
				fprintf(stderr, _("Failed to get file %s: %s\n"), data->file->name, gn_error_print(error));
			else {
				fprintf(stderr, _("Got file %s.\n"), filename2);
				f = fopen(filename2, "w");
				if (!f) {
					fprintf(stderr, _("Can't open file %s for writing!\n"), filename2);
					return GN_ERR_FAILED;
				}
				if (fwrite(data->file->file, data->file->file_length, 1, f) < 0) {
					fprintf(stderr, _("Failed to write to file %s.\n"), filename2);
					fclose(f);
					return GN_ERR_FAILED; 
				}
				fclose(f);
				free(data->file->file);
			}
			free(fi.files[i]);
		}
	}
	return error;
}

int putfile_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --putfile local_filename remote_filename\n"));
	return exitval;
}

/* Put file */
gn_error putfile(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
    	gn_file fi;
	gn_error error;
	FILE *f;

	if (argc != optind + 1)
		return putfile_usage(stderr, -1);

	memset(&fi, 0, sizeof(fi));
	snprintf(fi.name, 512, "%s", argv[optind]);

	gn_data_clear(data);
	data->file = &fi;

	f = fopen(optarg, "rb");
	if (!f || fseek(f, 0, SEEK_END)) {
		fprintf(stderr, _("Can't open file %s for reading!\n"), optarg);
		return GN_ERR_FAILED;
	}
	fi.file_length = ftell(f);
	rewind(f);
	fi.file = malloc(fi.file_length);
	if (fread(fi.file, 1, fi.file_length, f) != fi.file_length) {
		fprintf(stderr, _("Can't open file %s for reading!\n"), optarg);
		return GN_ERR_FAILED;
	}

	if ((error = gn_sm_functions(GN_OP_PutFile, data, state)) != GN_ERR_NONE)
		fprintf(stderr, _("Failed to put file to %s: %s\n"), argv[optind], gn_error_print(error));

	free(fi.file);
//fclose(f);

	return error;
}
