/*

  $Id$

  G N O K I I

  A Linux/Unix toolset and driver for Nokia mobile phones.

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

  Copyright (C) 1999, 2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2003 Pawe³ Kot
  
  Main header file for gnokii. Include just this file in your app.
*/

#ifndef _gnokii_h
#define _gnokii_h

#ifndef API
#  define API
#endif

struct gn_statemachine;

#include <gnokii/error.h>
#include <gnokii/common.h>
#include <gnokii/data.h>
#include <gnokii/encoding.h>
#include <gnokii/sms.h>
#include <gnokii/call.h>
#include <gnokii/networks.h>
#include <gnokii/bitmaps.h>
#include <gnokii/ringtones.h>
#include <gnokii/virtmodem.h>
#include <gnokii/rlp-common.h>

#include <gnokii/statemachine.h>

API struct gn_cfg_header *gn_cfg_info;

/* Files */
API int gn_file_text_save(char *filename, char *text, int mode);

/* Misc */
API gn_memory_type gn_str2memory_type(const char *s);
API void gn_data_clear(gn_data *data);
API gn_phone *gn_gsm_info;
API gn_error (*gn_gsm_f)(gn_operation op, gn_data *data,
			 struct gn_statemachine *state);
API gn_error gn_gsm_initialise(struct gn_statemachine *sm);

/* Config file */
API char *gn_cfg_get(struct gn_cfg_header *cfg, const char *section, const char *key);
API int gn_cfg_read(char **bindir);
API int gn_cfg_phone_load(const char *iname, struct gn_statemachine *state);

API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *addon);
API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry);

API int gn_vcal_file_event_read(char *filename, gn_calnote *cnote, int number);
API int gn_vcal_file_todo_read(char *filename, gn_todo *ctodo, int number);

API void (*gn_elog_handler)(const char *fmt, va_list ap);
API void gn_elog_write(const char *fmt, ...);

API int gn_line_get(FILE *file, char *line, int count);

API char *gn_device_lock(const char *);
API int gn_device_unlock(char *);

API char *gn_model_get(const char *);
API gn_phone_model *gn_phone_model_get(const char *);

/* SMS */
API gn_error gn_sms_send(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_save(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get_no_validate(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get_folder_changes(gn_data *data, struct gn_statemachine *state,
				       int has_folders);
API gn_error gn_sms_delete(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_delete_no_validate(gn_data *data, struct gn_statemachine *state);

/* Call service */
API gn_error gn_call_dial(int *call_id, gn_data *data, struct gn_statemachine *state);

#endif	/* _gnokii_h */
