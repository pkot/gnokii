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
  Copyright (C) 2001-2002 Pawe³ Kot
  
  Header file for the various functions, definitions etc. used to implement
  the handset interface.  See gsm-api.c for more details.

*/

#ifndef _gnokii_gsm_api_h
#define _gnokii_gsm_api_h

#include "gnokii/sms.h"
#include "gnokii/call.h"
#include "gnokii/common.h"
#include "gnokii/data.h"
#include "gnokii/networks.h"
#include "gnokii/statemachine.h"
#include "gnokii/virtmodem.h"

API struct gn_cfg_header *gn_cfg_info;

/* SMS */
API gn_error gn_sms_send(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_save(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get_no_validate(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_get_folder_changes(gn_data *data, struct gn_statemachine *state,
				       int has_folders);
API gn_error gn_sms_delete(gn_data *data, struct gn_statemachine *state);
API gn_error gn_sms_delete_no_validate(gn_data *data, struct gn_statemachine *state);
API void gn_sms_default_submit(gn_sms *sms);
API void gn_sms_default_deliver(gn_sms *sms);

/* Call */
API void gn_call_notifier(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state);
API gn_error gn_call_dial(int *call_id, gn_data *data, struct gn_statemachine *state);
API gn_call *gn_call_get_active(int call_id);
API gn_error gn_call_answer(int call_id);
API gn_error gn_call_cancel(int call_id);

/* Statemachine */
API gn_state gn_sm_loop(int timeout, struct gn_statemachine *state);
API gn_error gn_sm_functions(gn_operation op, gn_data *data, struct gn_statemachine *sm);


/* Files */
API gn_error gn_file_ringtone_read(char *filename, gn_ringtone *ringtone);
API gn_error gn_file_ringtone_save(char *filename, gn_ringtone *ringtone);
API gn_error gn_file_bitmap_read(char *filename, gn_bmp *bitmap, gn_phone *info);
API gn_error gn_file_bitmap_save(char *filename, gn_bmp *bitmap, gn_phone *info);
API gn_error gn_file_bitmap_show(char *filename);
API int gn_file_text_save(char *filename, char *text, int mode);

/* Bitmaps */
API gn_error gn_bmp_null(gn_bmp *bmp, gn_phone *info);
API void gn_bmp_point_set(gn_bmp *bmp, int x, int y);
API void gn_bmp_point_clear(gn_bmp *bmp, int x, int y);
API bool gn_bmp_point(gn_bmp *bmp, int x, int y);
API void gn_bmp_clear(gn_bmp *bmp);
API void gn_bmp_resize(gn_bmp *bitmap, gn_bmp_types target, gn_phone *info);
API void gn_bmp_print(gn_bmp *bitmap, FILE *f);

/* SMS bitmap functions */
API int gn_bmp_sms_encode(gn_bmp *bitmap, unsigned char *message);
API gn_error gn_bmp_sms_read(int type, unsigned char *message, unsigned char *code, gn_bmp *bitmap);

/* Misc */
API gn_memory_type gn_str2memory_type(const char *s);
API void gn_data_clear(gn_data *data);
API gn_phone *gn_gsm_info;
API gn_error (*gn_gsm_f)(gn_operation op, gn_data *data,
			 struct gn_statemachine *state);
API gn_error gn_gsm_initialise(struct gn_statemachine *sm);

/* Encoding */
API bool gn_char_def_alphabet(unsigned char *string);

/* Errors */
API char *gn_error_print(gn_error e);

/* These functions are used to search for network info. */
API char *gn_network_name_get(char *network_code);
API char *gn_network_code_get(char *network_name);

API char *gn_country_name_get(char *country_code);
API char *gn_country_code_get(char *country_name);

API bool gn_network_get(gn_network *network, int index);
API bool gn_country_get(gn_country *country, int index);

/* Ringtones */
API u8 gn_ringtone_pack(gn_ringtone *ringtone, unsigned char *package, int *maxlength);
API gn_error gn_ringtone_unpack(gn_ringtone *ringtone, unsigned char *package, int maxlength);
API int gn_note_get(int number);

/* Config file */
API char *gn_cfg_get(struct gn_cfg_header *cfg, const char *section, const char *key);
API int gn_cfg_read(char **bindir);
API bool gn_cfg_phone_load(const char *iname, struct gn_statemachine *state);

API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *addon);
API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry);

API int gn_vcal_file_event_read(char *filename, gn_calnote *cnote, int number);
API int gn_vcal_file_todo_read(char *filename, gn_todo *ctodo, int number);

API void (*gn_elog_handler)(const char *fmt, va_list ap);
API void gn_elog_write(const char *fmt, ...);

API int gn_line_get(FILE *file, char *line, int count);

API char *gn_device_lock(const char *);
API bool gn_device_unlock(char *);

API char *gn_model_get(const char *);
API gn_phone_model *gn_phone_model_get(const char *);


#endif	/* _gnokii_gsm_api_h */
