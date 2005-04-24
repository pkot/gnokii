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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2001-2005 Pawel Kot
  Copyright (C) 2001      Chris Kemp
  copyright (C) 2002      Markus Plail
  Copyright (C) 2002-2003 BORBELY Zoltan, Ladis Michl
  Copyright (C) 2004      Martin Goldhahn
  
  Main header file for gnokii. Include just this file in your app.
*/

#ifndef _gnokii_h
#define _gnokii_h

#ifdef __cplusplus
extern "C" {
#endif

/* Some portability definitions first */
#if defined(__linux__)
#  include <stdint.h>
#  include <sys/time.h>
#elif defined(__svr4__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__MACH__)
#  include <inttypes.h>
#  include <sys/time.h>
#elif defined(_MSC_VER) && defined(WIN32)
#  include <Winsock.h>	/* for struct timeval */
typedef unsigned char uint8_t;
#endif

#include <stdarg.h>
	
#ifndef API
#  if defined(WIN32) && defined(GNOKIIDLL_IMPORTS)
#    define API __declspec(dllimport)
#  else
#    define API
#  endif
#endif

#define LIBGNOKII_VERSION_STRING "2.0.2"
#define LIBGNOKII_VERSION_MAJOR 2
#define LIBGNOKII_VERSION_MINOR 0
#define LIBGNOKII_VERSION_RELEASE 2
#define LIBGNOKII_MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

#define LIBGNOKII_VERSION \
  LIBGNOKII_MAKE_VERSION(LIBGNOKII_VERSION_MAJOR,LIBGNOKII_VERSION_MINOR,LIBGNOKII_VERSION_RELEASE)

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

extern API struct gn_cfg_header *gn_cfg_info;

/* Files */
API int gn_file_text_save(char *filename, char *text, int mode);

/* Misc */
API gn_memory_type gn_str2memory_type(const char *s);
API char *gn_memory_type2str(gn_memory_type mt);
API void gn_data_clear(gn_data *data);
extern API gn_phone *gn_gsm_info;
extern API gn_error (*gn_gsm_f)(gn_operation op, gn_data *data,
			 struct gn_statemachine *state);
API gn_error gn_gsm_initialise(struct gn_statemachine *sm);
API int gn_timestamp_isvalid(gn_timestamp dt);

/* Config file */
API char *gn_cfg_get(struct gn_cfg_header *cfg, const char *section, const char *key);
API int gn_cfg_read(char **bindir); /* DEPRECATED */
API int gn_cfg_file_read(const char *filename);
API int gn_cfg_memory_read(const char **lines);
API int gn_cfg_read_default();
API int gn_cfg_phone_load(const char *iname, struct gn_statemachine *state);

/* In/Out routines, file formats */
API int gn_phonebook2vcard(FILE *f, gn_phonebook_entry *entry, char *location);
API int gn_vcard2phonebook(FILE *f, gn_phonebook_entry *entry);

API int gn_phonebook2ldif(FILE *f, gn_phonebook_entry *entry);
API int gn_ldif2phonebook(FILE *f, gn_phonebook_entry *entry);

/* reads internal gnokii raw phonebook format */
API gn_error gn_file_phonebook_raw_parse(gn_phonebook_entry *entry, char *buffer);
API gn_error gn_file_phonebook_raw_write(FILE *f, gn_phonebook_entry *entry, char *memory_type_string);

/* DEPRECATED */
API int gn_vcal_file_event_read(char *filename, gn_calnote *cnote, int number);
API int gn_vcal_file_todo_read(char *filename, gn_todo *ctodo, int number);

API int gn_calnote2ical(FILE *f, gn_calnote *calnote);
API int gn_ical2calnote(FILE *f, gn_calnote *calnote, int id);

API int gn_todo2ical(FILE *f, gn_todo *ctodo);
API int gn_ical2todo(FILE *f, gn_todo *ctodo, int id);

API void gn_number_sanitize(char *number, int maxlen);
API void gn_phonebook_entry_sanitize(gn_phonebook_entry *entry);

/* Debugging */
extern API gn_log_target gn_log_debug_mask;
extern API gn_log_target gn_log_rlpdebug_mask;
extern API gn_log_target gn_log_xdebug_mask;
extern API void (*gn_elog_handler)(const char *fmt, va_list ap);
API void gn_log_debug(const char *fmt, ...);
API void gn_log_rlpdebug(const char *fmt, ...);
API void gn_log_xdebug(const char *fmt, ...);
API void gn_elog_write(const char *fmt, ...);
typedef API void (*gn_log_func_t)(const char *fmt, ...);

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
API gn_error gn_call_check_active(struct gn_statemachine *state);

#ifdef __cplusplus
}
#endif

#endif	/* _gnokii_h */
