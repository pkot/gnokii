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

  Copyright (C) 2002 Pawe³ Kot

  Internal API for libgnokii

*/

#ifndef _gnokii_internal_h
#define _gnokii_internal_h

#include "gsm-sms.h"
#include "gsm-common.h"
#include "gsm-data.h"
#include "gsm-statemachine.h"

/* SMS */
gn_error sms_parse(gn_data *data, int offset);
gn_error sms_request(gn_data *data, struct gn_statemachine *state);
gn_error sms_prepare(gn_sms *sms, gn_sms_raw *rawsms);
gn_timestamp *gn_misc_unpack_timestamp(u8 *Number, gn_timestamp *dt);

/* Statemachine */
gn_error sm_initialise(struct gn_statemachine *state);
gn_error sm_message_send(struct gn_statemachine *state, u16 messagesize, u8 messagetype, void *message);
gn_error sm_wait_for(struct gn_statemachine *state, gn_data *data, unsigned char messagetype);
void sm_incoming_function(struct gn_statemachine *state, u8 messagetype, void *message, u16 messagesize);
void sm_reset(struct gn_statemachine *state);
gn_error sm_error_get(struct gn_statemachine *state, unsigned char messagetype);
gn_error sm_block_timeout(struct gn_statemachine *state, gn_data *data, int waitfor, int t);
gn_error sm_block(struct gn_statemachine *state, gn_data *data, int waitfor);
gn_error sm_block_no_retry_timeout(struct gn_statemachine *state, gn_data *data, int waitfor, int t);
gn_error sm_block_no_retry(struct gn_statemachine *state, gn_data *data, int waitfor);
void sm_message_dump(int messagetype, unsigned char *message, int length);
void sm_unhandled_frame_dump(struct gn_statemachine *state, int messagetype, unsigned char *message, int length);

extern void hex2bin(unsigned char *dest, const unsigned char *src, unsigned int len);
extern void bin2hex(unsigned char *dest, const unsigned char *src, unsigned int len);

int char_unpack_7bit(unsigned int offset, unsigned int in_length, unsigned int out_length,
		     unsigned char *input, unsigned char *output);
int char_pack_7bit(unsigned int offset, unsigned char *input, unsigned char *output,
		   unsigned int *in_len);

unsigned int char_decode_unicode(unsigned char* dest, const unsigned char* src, int len);
unsigned int char_encode_unicode(unsigned char* dest, const unsigned char* src, int len);

void char_decode_ascii(unsigned char* dest, const unsigned char* src, int len);
unsigned int char_encode_ascii(unsigned char* dest, const unsigned char* src, unsigned int len);

void char_decode_hex(unsigned char* dest, const unsigned char* src, int len);
void char_encode_hex(unsigned char* dest, const unsigned char* src, int len);

void char_decode_ucs2(unsigned char* dest, const unsigned char* src, int len);
void char_encode_ucs2(unsigned char* dest, const unsigned char* src, int len);

unsigned char char_encode_def_alphabet(unsigned char value);
unsigned char char_decode_def_alphabet(unsigned char value);

int char_encode_uni_alphabet(unsigned char const *value, wchar_t *dest);
int char_decode_uni_alphabet(wchar_t value, unsigned char *dest);

extern char *char_get_bcd_number(u8 *number);
extern int char_semi_octet_pack(char *number, unsigned char *output, gn_gsm_number_type type);

/* Ringtones */
int gn_vcal_read_file_event(char *filename, gn_calnote *cnote, int number);
int gn_vcal_read_file_todo(char *filename, gn_todo *cnote, int number);

int gn_vcal_get_time(gn_timestamp *dt, char *time);
int gn_calnote_fill(gn_calnote *note, char *type, char *text, char *desc,
		    char *time, char *alarm);
int gn_todo_fill(gn_todo *note, char *text, char *todo_priority);

/* Ringtone Files */
gn_error gn_file_ringtone_read(char *filename, gn_ringtone *ringtone);
gn_error gn_file_ringtone_save(char *filename, gn_ringtone *ringtone);

gn_error file_save_rttl(FILE *file, gn_ringtone *ringtone);
gn_error file_save_ott(FILE *file, gn_ringtone *ringtone);

gn_error file_load_rttl(FILE *file, gn_ringtone *ringtone);
gn_error file_load_ott(FILE *file, gn_ringtone *ringtone);

/* Bitmap Files */
gn_error gn_file_bitmap_read(char *filename, gn_bmp *bitmap, gn_phone *info);
gn_error gn_file_bitmap_save(char *filename, gn_bmp *bitmap, gn_phone *info);
int gn_file_text_save(char *filename, char *text, int mode);
gn_error gn_file_bitmap_show(char *filename);

void file_save_nol(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_save_ngg(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_save_nsl(FILE *file, gn_bmp *bitmap, gn_phone *info);
void file_save_nlm(FILE *file, gn_bmp *bitmap);
void file_save_ota(FILE *file, gn_bmp *bitmap);
void file_save_bmp(FILE *file, gn_bmp *bitmap);

#ifdef XPM
void file_save_xpm(char *filename, gn_bmp *bitmap);
#endif

gn_error file_load_ngg(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_load_nol(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_load_nsl(FILE *file, gn_bmp *bitmap);
gn_error file_load_nlm(FILE *file, gn_bmp *bitmap);
gn_error file_load_ota(FILE *file, gn_bmp *bitmap, gn_phone *info);
gn_error file_load_bmp(FILE *file, gn_bmp *bitmap);

#ifdef XPM
gn_error file_load_xpm(char *filename, gn_bmp *bitmap);
#endif

int ringtone_encode_sms(unsigned char *message, gn_ringtone *ringtone);
int imelody_encode_sms(unsigned char *imelody, unsigned char *message);
gn_error phonebook_decode(unsigned char *blockstart, int length,
			  gn_data *data, int blocks, int memtype, int speeddial_pos);
gn_error calnote_decode(unsigned char *message, int length, gn_data *data);

int sms_nokia_pack_smart_message_part(unsigned char *msg, unsigned int size,
				      unsigned int type, bool first);
int sms_nokia_encode_text(unsigned char *text, unsigned char *message, bool first);
int sms_nokia_encode_bitmap(gn_bmp *bitmap, unsigned char *message, bool first);

struct gn_cfg_header *cfg_read_file(const char *filename);
typedef void (*cfg_get_foreach_func)(const char *section, const char *key, const char *value);
void cfg_get_foreach(struct gn_cfg_header *cfg, const char *section, cfg_get_foreach_func func);
char *cfg_set(struct gn_cfg_header *cfg, const char *section, const char *key, const char *value);
int cfg_write_file(struct gn_cfg_header *cfg, const char *filename);

#endif /* _gnokii_api_h */
