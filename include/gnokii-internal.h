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

  Copyright (C) 1999-2000 Hugh Blemings, Pavel Janik
  Copyright (C) 2000-2004 Pawel Kot, BORBELY Zoltan
  Copyright (C) 2000      Marcin Wiacek, Chris Kemp
  Copyright (C) 2002      Ladis Michl
  Copyright (C) 2003      Osma Suominen, Martin Goldhahn

  Internal API for libgnokii

*/

#ifndef _gnokii_internal_h
#define _gnokii_internal_h

#include "compat.h"
#include "misc.h"

#if !defined(GNOKII_DEPRECATED)
#  define GNOKII_DEPRECATED /* do not warn about deprecated functions here */
#endif

#include "gnokii.h"

/* SMS */
gn_error gn_sms_parse(gn_data *data);
gn_error gn_sms_pdu2raw(gn_sms_raw *rawsms, unsigned char *pdu, int pdu_len, int flags);
gn_error gn_sms_request(gn_data *data, struct gn_statemachine *state);
gn_error sms_prepare(gn_sms *sms, gn_sms_raw *rawsms);
gn_timestamp *sms_timestamp_unpack(unsigned char *number, gn_timestamp *dt);
unsigned char *sms_timestamp_pack(gn_timestamp *dt, unsigned char *number);

/* Statemachine */
gn_error sm_initialise(struct gn_statemachine *state);
gn_error sm_message_send(u16 messagesize, u8 messagetype, void *message, struct gn_statemachine *state);
gn_error sm_wait_for(unsigned char messagetype, gn_data *data, struct gn_statemachine *state);
void sm_incoming_function(u8 messagetype, void *message, u16 messagesize, struct gn_statemachine *state);
void sm_incoming_acknowledge(struct gn_statemachine *state);
void sm_reset(struct gn_statemachine *state);
gn_error sm_error_get(unsigned char messagetype, struct gn_statemachine *state);
gn_error sm_block_timeout(int waitfor, int t, gn_data *data, struct gn_statemachine *state);
gn_error sm_block(int waitfor, gn_data *data, struct gn_statemachine *state);
gn_error sm_block_no_retry_timeout(int waitfor, int t, gn_data *data, struct gn_statemachine *state);
gn_error sm_block_no_retry(int waitfor, gn_data *data, struct gn_statemachine *state);
gn_error sm_block_ack(struct gn_statemachine *state);
void sm_message_dump(gn_log_func_t lfunc, int messagetype, unsigned char *message, int length);
void sm_unhandled_frame_dump(int messagetype, unsigned char *message, int length, struct gn_statemachine *state);

extern void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len);
extern void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len);

int char_7bit_unpack(unsigned int offset, unsigned int in_length, unsigned int out_length,
		     unsigned char *input, unsigned char *output);
int char_7bit_pack(unsigned int offset, unsigned char *input, unsigned char *output,
		   unsigned int *in_len);

int char_mblen(const char *src);

unsigned int char_unicode_decode(unsigned char* dest, const unsigned char* src, int len);
unsigned int char_unicode_encode(unsigned char* dest, const unsigned char* src, int len);

int char_default_alphabet_decode(unsigned char* dest, const unsigned char* src, int len);
size_t char_ascii_encode(char *dest, size_t dest_len, const char *src, size_t len);

void char_hex_decode(unsigned char* dest, const unsigned char* src, int len);
size_t char_hex_encode(char *dest, size_t dest_len, const char *src, size_t len);

void char_ucs2_decode(unsigned char* dest, const unsigned char* src, int len);
size_t char_ucs2_encode(char *dest, size_t dest_len, const char *src, size_t len);

unsigned char char_def_alphabet_encode(unsigned int value);
unsigned int char_def_alphabet_decode(unsigned char value);

size_t char_uni_alphabet_encode(const char *value, size_t n, wchar_t *dest, MBSTATE *mbs);
int char_uni_alphabet_decode(wchar_t value, unsigned char *dest, MBSTATE *mbs);

extern char *char_bcd_number_get(u8 *number);
extern int char_semi_octet_pack(char *number, unsigned char *output, gn_gsm_number_type type);

int ucs2_encode(char *outstring, int outlen, const char *instring, int inlen);

int char_def_alphabet_ext_count(unsigned char *input, int length);
bool char_def_alphabet_ext(unsigned int value);

/* Ringtones */
int vcal_time_get(gn_timestamp *dt, char *time);
int calnote_fill(gn_calnote *note, char *type, char *text, char *desc,
		 char *time, char *alarm);
int todo_fill(gn_todo *note, char *text, char *todo_priority);

/* Ringtone Files */
gn_error file_rtttl_save(FILE *file, gn_ringtone *ringtone);
gn_error file_ott_save(FILE *file, gn_ringtone *ringtone);
gn_error file_midi_save(FILE *file, gn_ringtone *ringtone);
gn_error file_nokraw_save(FILE *file, gn_ringtone *ringtone, int dct4);

gn_error file_rtttl_load(FILE *file, gn_ringtone *ringtone);
gn_error file_ott_load(FILE *file, gn_ringtone *ringtone);
gn_error file_midi_load(FILE *file, gn_ringtone *ringtone);
gn_error file_nokraw_load(FILE *file, gn_ringtone *ringtone);

/* Bitmap Files */

void file_nol_save(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_ngg_save(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_nsl_save(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_nlm_save(FILE *file, gn_bmp *bitmap);
void file_ota_save(FILE *file, gn_bmp *bitmap);
void file_bmp_save(FILE *file, gn_bmp *bitmap);

#ifdef XPM
void file_xpm_save(char *filename, gn_bmp *bitmap);
#endif

gn_error file_ngg_load(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_nol_load(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_nsl_load(FILE *file, gn_bmp *bitmap);
gn_error file_nlm_load(FILE *file, gn_bmp *bitmap);
gn_error file_ota_load(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_bmp_load(FILE *file, gn_bmp *bitmap);

#ifdef XPM
gn_error file_xpm_load(char *filename, gn_bmp *bitmap);
#endif

int ringtone_sms_encode(unsigned char *message, gn_ringtone *ringtone);
int imelody_sms_encode(unsigned char *imelody, unsigned char *message);
gn_error phonebook_decode(unsigned char *blockstart, int length,
			  gn_data *data, int blocks, int memtype, int speeddial_pos);
gn_error calnote_decode(unsigned char *message, int length, gn_data *data);
gn_error calnote_get_alarm(int alarmdiff, gn_timestamp *time, gn_timestamp *alarm);

int sms_nokia_smart_message_part_pack(unsigned char *msg, unsigned int size,
				      unsigned int type, bool first);
int sms_nokia_text_encode(unsigned char *text, unsigned char *message, bool first);
int sms_nokia_bitmap_encode(gn_bmp *bitmap, unsigned char *message, bool first);

struct gn_cfg_header *cfg_file_read(const char *filename);
typedef void (*cfg_foreach_func)(const char *section, const char *key, const char *value);
void cfg_foreach(const char *section, cfg_foreach_func func);
char *cfg_set(struct gn_cfg_header *cfg, const char *section, const char *key, const char *value);
int cfg_file_write(struct gn_cfg_header *cfg, const char *filename);
/* Get some information about the given phone */
gn_phone_model *gn_cfg_get_phone_model(struct gn_cfg_header *cfg, const char *product_name);

gn_error isdn_cause2gn_error(char **src, char **msg, unsigned char loc, unsigned char cause);

int utf8_decode(char *dest, size_t destlen, const char *src, size_t inlen);
int utf8_encode(char *dest, int destlen, const char *src, int inlen);

int string_base64(const char *instring);
int base64_decode(char *dest, int destlen, const char *src, int inlen);
int base64_encode(char *dest, int destlen, const char *src, int inlen);

int utf8_base64_decode(char *dest, int destlen, const char *src, int inlen);
int utf8_base64_encode(char *dest, int destlen, const char *src, int inlen);

int add_slashes(char *dest, char *src, int maxlen, int len);
int strip_slashes(char *dest, const char *src, int maxlen, int len);

#endif /* _gnokii_internal_h */
