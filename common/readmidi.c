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

  Copyright (C) 1999-2000 Hugh Blemings & Pavel Janík ml.
  Copyright (C) 2003      Tomi Ollila, BORBELY Zoltan

  Functions to read and write common file types.

*/

/*
 * This file was downloaded from http://www.guru-group.fi/~too/sw/xring/
 * The page states this is a GPL'd program, so I added an explicit GPL
 * header (required by savannah). -- bozo
 */

/* embedding modified midifile.h and midifile.c into this file */

/***** midifile.h ******/

struct MF {
/* definitions for MIDI file parsing code */
	int (*Mf_getc) (struct MF *);
	void (*Mf_header) (struct MF *, int, int, int);
	void (*Mf_trackstart) (struct MF *);
	void (*Mf_trackend) (struct MF *);
	void (*Mf_noteon) (struct MF *, int, int, int);
	void (*Mf_noteoff) (struct MF *, int, int, int);
	void (*Mf_pressure) (struct MF *, int, int, int);
	void (*Mf_parameter) (struct MF *, int, int, int);
	void (*Mf_pitchbend) (struct MF *, int, int, int);
	void (*Mf_program) (struct MF *, int, int);
	void (*Mf_chanpressure) (struct MF *, int, int);
	void (*Mf_sysex) (struct MF *, int, char *);
	void (*Mf_metamisc) (struct MF *, int, int, char *);
	void (*Mf_seqspecific) (struct MF *, int, int, char *);
	void (*Mf_seqnum) (struct MF *, int);
	void (*Mf_text) (struct MF *, int, int, char *);
	void (*Mf_eot) (struct MF *);
	void (*Mf_timesig) (struct MF *, int, int, int, int);
	void (*Mf_smpte) (struct MF *, int, int, int, int, int);
	void (*Mf_tempo) (struct MF *, long);
	void (*Mf_keysig) (struct MF *, int, int);
	void (*Mf_arbitrary) (struct MF *, int, char *);
	void (*Mf_error) (struct MF *, char *);
/* definitions for MIDI file writing code */
	int (*Mf_putc) (struct MF *, int);
	long (*Mf_getpos) (struct MF *);
	int (*Mf_setpos) (struct MF *, long);
	void (*Mf_writetrack) (struct MF *, int);
	void (*Mf_writetempotrack) (struct MF *);
	/* variables */
	int Mf_nomerge;		/* 1 => continue'ed system exclusives are */
	/* not collapsed. */
	long Mf_currtime;	/* current time in delta-time units */

/* private stuff */
	long Mf_toberead;
	long Mf_numbyteswritten;

	char *Msgbuff;		/* message buffer */
	int Msgsize;		/* Size of currently allocated Msg */
	int Msgindex;		/* index of next available location in Msg */

};

float mf_ticks2sec(unsigned long ticks, int division, unsigned int tempo);
unsigned long mf_sec2ticks(float secs, int division, unsigned int tempo);

void mferror(struct MF *mf, char *s);

/*void mfwrite(); */


/* MIDI status commands most significant bit is 1 */
#define note_off         	0x80
#define note_on          	0x90
#define poly_aftertouch  	0xa0
#define control_change    	0xb0
#define program_chng     	0xc0
#define channel_aftertouch      0xd0
#define pitch_wheel      	0xe0
#define system_exclusive      	0xf0
#define delay_packet	 	(1111)

/* 7 bit controllers */
#define damper_pedal            0x40
#define portamento	        0x41
#define sostenuto	        0x42
#define soft_pedal	        0x43
#define general_4               0x44
#define	hold_2		        0x45
#define	general_5	        0x50
#define	general_6	        0x51
#define general_7	        0x52
#define general_8	        0x53
#define tremolo_depth	        0x5c
#define chorus_depth	        0x5d
#define	detune		        0x5e
#define phaser_depth	        0x5f

/* parameter values */
#define data_inc	        0x60
#define data_dec	        0x61

/* parameter selection */
#define non_reg_lsb	        0x62
#define non_reg_msb	        0x63
#define reg_lsb		        0x64
#define reg_msb		        0x65

/* Standard MIDI Files meta event definitions */
#define	meta_event		0xFF
#define	sequence_number 	0x00
#define	text_event		0x01
#define copyright_notice 	0x02
#define sequence_name    	0x03
#define instrument_name 	0x04
#define lyric	        	0x05
#define marker			0x06
#define	cue_point		0x07
#define channel_prefix		0x20
#define	end_of_track		0x2f
#define	set_tempo		0x51
#define	smpte_offset		0x54
#define	time_signature		0x58
#define	key_signature		0x59
#define	sequencer_specific	0x74

/* Manufacturer's ID number */
#define Seq_Circuits (0x01)	/* Sequential Circuits Inc. */
#define Big_Briar    (0x02)	/* Big Briar Inc.           */
#define Octave       (0x03)	/* Octave/Plateau           */
#define Moog         (0x04)	/* Moog Music               */
#define Passport     (0x05)	/* Passport Designs         */
#define Lexicon      (0x06)	/* Lexicon                  */
#define Tempi        (0x20)	/* Bon Tempi                */
#define Siel         (0x21)	/* S.I.E.L.                 */
#define Kawai        (0x41)
#define Roland       (0x42)
#define Korg         (0x42)
#define Yamaha       (0x43)

/* miscellaneous definitions */
#define MThd 0x4d546864
#define MTrk 0x4d54726b
#define lowerbyte(x) ((unsigned char)(x & 0xff))
#define upperbyte(x) ((unsigned char)((x & 0xff00)>>8))

/* the midifile interface */
void midifile(struct MF *mf);

/***** midifile.c ******/

/*
 * midifile 1.11
 * 
 * Read and write a MIDI file.  Externally-assigned function pointers are 
 * called upon recognizing things in the file.
 *
 * Original release ?
 * June 1989 - Added writing capability, M. Czeiszperger.
 *
 *          The file format implemented here is called
 *          Standard MIDI Files, and is part of the Musical
 *          instrument Digital Interface specification.
 *          The spec is avaiable from:
 *
 *               International MIDI Association
 *               5316 West 57th Street
 *               Los Angeles, CA 90056
 *
 *          An in-depth description of the spec can also be found
 *          in the article "Introducing Standard MIDI Files", published
 *          in Electronic Musician magazine, April, 1989.
 * 
 */

#include "config.h"
#include "misc.h"
#include "gnokii.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NULLFUNC NULL

static void readheader(struct MF *mf);
static int readtrack(struct MF *mf);
static void chanmessage(struct MF *mf, int status, int c1, int c2);
static void msginit(struct MF *mf);
static void msgadd(struct MF *mf, int c);
static void metaevent(struct MF *mf, int type);
static void sysex(struct MF *mf);
static int msgleng(struct MF *mf);
static void badbyte(struct MF *mf, int c);
static void biggermsg(struct MF *mf);
static void mf_write_track_chunk(struct MF *mf, int which_track);
static void mf_write_header_chunk(struct MF *mf, int format, int ntracks, int division);


static long readvarinum(struct MF *mf);
static long read32bit(struct MF *mf);
static long to32bit(int, int, int, int);
static int read16bit(struct MF *mf);
static int to16bit(int, int);
static char *msg(struct MF *mf);
static void write32bit(struct MF *mf, unsigned long data);
static void write16bit(struct MF *mf, int data);
static int eputc(struct MF *mf, unsigned char c);
static void WriteVarLen(struct MF *mf, unsigned long value);

/* The only non-static function in this file. */
void mfread(struct MF *mf)
{
	if (mf->Mf_getc == NULLFUNC)
		mferror(mf, "mfread() called without setting Mf_getc");

	readheader(mf);
	while (readtrack(mf))
		;
}

/* for backward compatibility with the original lib */
void midifile(struct MF *mf)
{
	mfread(mf);
}

/* read through the "MThd" or "MTrk" header string */
static int readmt(struct MF *mf, char *s)
{
	int n = 0;
	char *p = s;
	int c = 0;

	while (n++ < 4 && (c = mf->Mf_getc(mf)) != EOF) {
		if (c != *p++) {
			char buff[32];
			snprintf(buff, sizeof(buff), "expecting %s", s);
			mferror(mf, buff);
		}
	}
	return c;
}

/* read a single character and abort on EOF */
static int egetc(struct MF *mf)
{
	int c = mf->Mf_getc(mf);

	if (c == EOF)
		mferror(mf, "premature EOF");
	mf->Mf_toberead--;
	return c;
}

/* read a header chunk */
static void readheader(struct MF *mf)
{
	int format, ntrks, division;

	if (readmt(mf, "MThd") == EOF)
		return;

	mf->Mf_toberead = read32bit(mf);
	format = read16bit(mf);
	ntrks = read16bit(mf);
	division = read16bit(mf);

	if (mf->Mf_header)
		(*mf->Mf_header) (mf, format, ntrks, division);

	/* flush any extra stuff, in case the length of header is not 6 */
	while (mf->Mf_toberead > 0)
		(void) egetc(mf);
}

static int readtrack(struct MF *mf)
{				/* read a track chunk */
	/* This array is indexed by the high half of a status byte.  It's */
	/* value is either the number of bytes needed (1 or 2) for a channel */
	/* message, or 0 (meaning it's not  a channel message). */
	static int chantype[] = {
		0, 0, 0, 0, 0, 0, 0, 0,	/* 0x00 through 0x70 */
		2, 2, 2, 2, 1, 1, 2, 0	/* 0x80 through 0xf0 */
	};
	long lookfor;
	int c, c1, type;
	int sysexcontinue = 0;	/* 1 if last message was an unfinished sysex */
	int running = 0;	/* 1 when running status used */
	int status = 0;		/* status value (e.g. 0x90==note-on) */
	int needed;

	if (readmt(mf, "MTrk") == EOF)
		return (0);

	mf->Mf_toberead = read32bit(mf);
	mf->Mf_currtime = 0;

	if (mf->Mf_trackstart)
		(*mf->Mf_trackstart) (mf);

	while (mf->Mf_toberead > 0) {

		mf->Mf_currtime += readvarinum(mf);	/* delta time */

		c = egetc(mf);

		if (sysexcontinue && c != 0xf7)
			mferror(mf, "didn't find expected continuation of a sysex");

		if ((c & 0x80) == 0) {	/* running status? */
			if (status == 0)
				mferror(mf, "unexpected running status");
			running = 1;
		} else {
			status = c;
			running = 0;
		}

		needed = chantype[(status >> 4) & 0xf];

		if (needed) {	/* ie. is it a channel message? */

			if (running)
				c1 = c;
			else
				c1 = egetc(mf);
			chanmessage(mf, status, c1, (needed > 1) ? egetc(mf) : 0);
			continue;;
		}

		switch (c) {

		case 0xff:	/* meta event */

			type = egetc(mf);
			lookfor = mf->Mf_toberead - readvarinum(mf);
			msginit(mf);

			while (mf->Mf_toberead > lookfor)
				msgadd(mf, egetc(mf));

			metaevent(mf, type);
			break;

		case 0xf0:	/* start of system exclusive */

			lookfor = mf->Mf_toberead - readvarinum(mf);
			msginit(mf);
			msgadd(mf, 0xf0);

			while (mf->Mf_toberead > lookfor)
				msgadd(mf, c = egetc(mf));

			if (c == 0xf7 || mf->Mf_nomerge == 0)
				sysex(mf);
			else
				sysexcontinue = 1;	/* merge into next msg */
			break;

		case 0xf7:	/* sysex continuation or arbitrary stuff */

			lookfor = mf->Mf_toberead - readvarinum(mf);

			if (!sysexcontinue)
				msginit(mf);

			while (mf->Mf_toberead > lookfor)
				msgadd(mf, c = egetc(mf));

			if (!sysexcontinue) {
				if (mf->Mf_arbitrary)
					(*mf->Mf_arbitrary) (mf, msgleng(mf), msg(mf));
			} else if (c == 0xf7) {
				sysex(mf);
				sysexcontinue = 0;
			}
			break;
		default:
			badbyte(mf, c);
			break;
		}
	}
	if (mf->Mf_trackend)
		(*mf->Mf_trackend) (mf);
	return (1);
}

static void badbyte(struct MF *mf, int c)
{
	char buff[32];

	snprintf(buff, sizeof(buff), "unexpected byte: 0x%02x", c);
	mferror(mf, buff);
}

static void metaevent(struct MF *mf, int type)
{
	int leng = msgleng(mf);
	char *m = msg(mf);

	switch (type) {
	case 0x00:
		if (mf->Mf_seqnum)
			(*mf->Mf_seqnum) (mf, to16bit(m[0], m[1]));
		break;
	case 0x01:		/* Text event */
	case 0x02:		/* Copyright notice */
	case 0x03:		/* Sequence/Track name */
	case 0x04:		/* Instrument name */
	case 0x05:		/* Lyric */
	case 0x06:		/* Marker */
	case 0x07:		/* Cue point */
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
		/* These are all text events */
		if (mf->Mf_text)
			(*mf->Mf_text) (mf, type, leng, m);
		break;
	case 0x2f:		/* End of Track */
		if (mf->Mf_eot)
			(*mf->Mf_eot) (mf);
		break;
	case 0x51:		/* Set tempo */
		if (mf->Mf_tempo)
			(*mf->Mf_tempo) (mf, to32bit(0, m[0], m[1], m[2]));
		break;
	case 0x54:
		if (mf->Mf_smpte)
			(*mf->Mf_smpte) (mf, m[0], m[1], m[2], m[3], m[4]);
		break;
	case 0x58:
		if (mf->Mf_timesig)
			(*mf->Mf_timesig) (mf, m[0], m[1], m[2], m[3]);
		break;
	case 0x59:
		if (mf->Mf_keysig)
			(*mf->Mf_keysig) (mf, m[0], m[1]);
		break;
	case 0x7f:
		if (mf->Mf_seqspecific)
			(*mf->Mf_seqspecific) (mf, type, leng, m);
		break;
	default:
		if (mf->Mf_metamisc)
			(*mf->Mf_metamisc) (mf, type, leng, m);
	}
}

static void sysex(struct MF *mf)
{
	if (mf->Mf_sysex)
		(*mf->Mf_sysex) (mf, msgleng(mf), msg(mf));
}

static void chanmessage(struct MF *mf, int status, int c1, int c2)
{
	int chan = status & 0xf;

	switch (status & 0xf0) {
	case 0x80:
		if (mf->Mf_noteoff)
			(*mf->Mf_noteoff) (mf, chan, c1, c2);
		break;
	case 0x90:
		if (mf->Mf_noteon)
			(*mf->Mf_noteon) (mf, chan, c1, c2);
		break;
	case 0xa0:
		if (mf->Mf_pressure)
			(*mf->Mf_pressure) (mf, chan, c1, c2);
		break;
	case 0xb0:
		if (mf->Mf_parameter)
			(*mf->Mf_parameter) (mf, chan, c1, c2);
		break;
	case 0xe0:
		if (mf->Mf_pitchbend)
			(*mf->Mf_pitchbend) (mf, chan, c1, c2);
		break;
	case 0xc0:
		if (mf->Mf_program)
			(*mf->Mf_program) (mf, chan, c1);
		break;
	case 0xd0:
		if (mf->Mf_chanpressure)
			(*mf->Mf_chanpressure) (mf, chan, c1);
		break;
	}
}

/* readvarinum - read a varying-length number, and return the */
/* number of characters it took. */

static long readvarinum(struct MF *mf)
{
	long value;
	int c;

	c = egetc(mf);
	value = c;
	if (c & 0x80) {
		value &= 0x7f;
		do {
			c = egetc(mf);
			value = (value << 7) + (c & 0x7f);
		} while (c & 0x80);
	}
	return (value);
}

static long to32bit(int c1, int c2, int c3, int c4)
{
	long value = 0L;

	value = (c1 & 0xff);
	value = (value << 8) + (c2 & 0xff);
	value = (value << 8) + (c3 & 0xff);
	value = (value << 8) + (c4 & 0xff);
	return (value);
}

static int to16bit(int c1, int c2)
{
	return ((c1 & 0xff) << 8) + (c2 & 0xff);
}

static long read32bit(struct MF *mf)
{
	int c1, c2, c3, c4;

	c1 = egetc(mf);
	c2 = egetc(mf);
	c3 = egetc(mf);
	c4 = egetc(mf);
	return to32bit(c1, c2, c3, c4);
}

static int read16bit(struct MF *mf)
{
	int c1, c2;
	c1 = egetc(mf);
	c2 = egetc(mf);
	return to16bit(c1, c2);
}

/* static */
void mferror(struct MF *mf, char *s)
{
	if (mf->Mf_error)
		(*mf->Mf_error) (mf, s);
	exit(1);
}

/* The code below allows collection of a system exclusive message of */
/* arbitrary length.  The Msgbuff is expanded as necessary.  The only */
/* visible data/routines are msginit(), msgadd(), msg(), msgleng(). */

#define MSGINCREMENT 128

static void msginit(struct MF *mf)
{
	mf->Msgindex = 0;
}

static char *msg(struct MF *mf)
{
	return (mf->Msgbuff);
}

static int msgleng(struct MF *mf)
{
	return (mf->Msgindex);
}

static void msgadd(struct MF *mf, int c)
{
	/* If necessary, allocate larger message buffer. */
	if (mf->Msgindex >= mf->Msgsize)
		biggermsg(mf);
	mf->Msgbuff[mf->Msgindex++] = c;
}

static void biggermsg(struct MF *mf)
{
	char *newmess;
	char *oldmess = mf->Msgbuff;
	int oldleng = mf->Msgsize;

	mf->Msgsize += MSGINCREMENT;
	newmess = (char *) malloc((unsigned) (sizeof(char) * mf->Msgsize));

	if (newmess == NULL)
		mferror(mf, "malloc error!");

	/* copy old message into larger new one */
	if (oldmess != NULL) {
		register char *p = newmess;
		register char *q = oldmess;
		register char *endq = &oldmess[oldleng];

		for (; q != endq; p++, q++)
			*p = *q;
		free(oldmess);
	}
	mf->Msgbuff = newmess;
}

/*
 * mfwrite() - The only fuction you'll need to call to write out
 *             a midi file.
 *
 * format      0 - Single multi-channel track
 *             1 - Multiple simultaneous tracks
 *             2 - One or more sequentially independent
 *                 single track patterns                
 * ntracks     The number of tracks in the file.
 * division    This is kind of tricky, it can represent two
 *             things, depending on whether it is positive or negative
 *             (bit 15 set or not).  If  bit  15  of division  is zero,
 *             bits 14 through 0 represent the number of delta-time
 *             "ticks" which make up a quarter note.  If bit  15 of
 *             division  is  a one, delta-times in a file correspond to
 *             subdivisions of a second similiar to  SMPTE  and  MIDI
 *             time code.  In  this format bits 14 through 8 contain
 *             one of four values - 24, -25, -29, or -30,
 *             corresponding  to  the  four standard  SMPTE and MIDI
 *             time code frame per second formats, where  -29
 *             represents  30  drop  frame.   The  second  byte
 *             consisting  of  bits 7 through 0 corresponds the the
 *             resolution within a frame.  Refer the Standard MIDI
 *             Files 1.0 spec for more details.
 * fp          This should be the open file pointer to the file you
 *             want to write.  It will have be a global in order
 *             to work with Mf_putc.  
 */
void mfwrite(struct MF *mf, int format, int ntracks, int division)
{
	int i;

	if (mf->Mf_putc == NULLFUNC)
		mferror(mf, "mfmf_write() called without setting Mf_putc");

	if (mf->Mf_writetrack == NULLFUNC)
		mferror(mf, "mfmf_write() called without setting Mf_writetrack");

	if (mf->Mf_getpos == NULLFUNC)
		mferror(mf, "mfmf_write() called without setting Mf_getpos");

	if (mf->Mf_setpos == NULLFUNC)
		mferror(mf, "mfmf_write() called without setting Mf_setpos");

	/* every MIDI file starts with a header */
	mf_write_header_chunk(mf, format, ntracks, division);

	/* In format 1 files, the first track is a tempo map */
	if (format == 1 && (mf->Mf_writetempotrack)) {
		(*mf->Mf_writetempotrack) (mf);
	}

	/* The rest of the file is a series of tracks */
	for (i = 0; i < ntracks; i++)
		mf_write_track_chunk(mf, i);
}

static void mf_write_track_chunk(struct MF *mf, int which_track)
{
	unsigned long trkhdr, trklength;
	long offset, place_marker;


	trkhdr = MTrk;
	trklength = 0;

	/* Remember where the length was written, because we don't
	   know how long it will be until we've finished writing */
	offset = (*mf->Mf_getpos) (mf);

#ifdef DEBUG
	printf("offset = %d\n", (int) offset);
#endif

	/* Write the track chunk header */
	write32bit(mf, trkhdr);
	write32bit(mf, trklength);

	mf->Mf_numbyteswritten = 0L;	/* the header's length doesn't count */

	if (mf->Mf_writetrack) {
		(*mf->Mf_writetrack) (mf, which_track);
	}

	/* mf_write End of track meta event */
	eputc(mf, 0);
	eputc(mf, meta_event);
	eputc(mf, end_of_track);

	eputc(mf, 0);

	/* It's impossible to know how long the track chunk will be beforehand,
	   so the position of the track length data is kept so that it can
	   be written after the chunk has been generated */
	place_marker = (*mf->Mf_getpos) (mf);

	/* This method turned out not to be portable because the
	   parameter returned from ftell is not guaranteed to be
	   in bytes on every machine */
	/* track.length = place_marker - offset - (long) sizeof(track); */

#ifdef DEBUG
	printf("length = %d\n", (int) trklength);
#endif

	if (mf->Mf_setpos(mf, offset) < 0)
		mferror(mf, "error seeking during final stage of write");

	trklength = mf->Mf_numbyteswritten;

	/* Re-mf_write the track chunk header with right length */
	write32bit(mf, trkhdr);
	write32bit(mf, trklength);

	mf->Mf_setpos(mf, place_marker);
}				/* End gen_track_chunk() */


static void mf_write_header_chunk(struct MF *mf, int format, int ntracks, int division)
{
	unsigned long ident, length;

	ident = MThd;		/* Head chunk identifier                    */
	length = 6;		/* Chunk length                             */

	/* individual bytes of the header must be written separately
	   to preserve byte order across cpu types :-( */
	write32bit(mf, ident);
	write32bit(mf, length);
	write16bit(mf, format);
	write16bit(mf, ntracks);
	write16bit(mf, division);
}				/* end gen_header_chunk() */


/*
 * mf_write_midi_event()
 * 
 * Library routine to mf_write a single MIDI track event in the standard MIDI
 * file format. The format is:
 *
 *                    <delta-time><event>
 *
 * In this case, event can be any multi-byte midi message, such as
 * "note on", "note off", etc.      
 *
 * delta_time - the time in ticks since the last event.
 * type - the type of meta event.
 * chan - The midi channel.
 * data - A pointer to a block of chars containing the META EVENT,
 *        data.
 * size - The length of the meta-event data.
 */
int mf_write_midi_event(struct MF *mf, unsigned long delta_time, unsigned int type, unsigned int chan, unsigned char *data, unsigned long size)
{
	int i;
	unsigned char c;

	WriteVarLen(mf, delta_time);

	/* all MIDI events start with the type in the first four bits,
	   and the channel in the lower four bits */
	c = type | chan;

	if (chan > 15)
		mferror(mf, "error: MIDI channel greater than 16\n");

	eputc(mf, c);

	/* write out the data bytes */
	for (i = 0; i < size; i++)
		eputc(mf, data[i]);

	return (size);
}				/* end mf_write MIDI event */

/*
 * mf_write_meta_event()
 *
 * Library routine to mf_write a single meta event in the standard MIDI
 * file format. The format of a meta event is:
 *
 *          <delta-time><FF><type><length><bytes>
 *
 * delta_time - the time in ticks since the last event.
 * type - the type of meta event.
 * data - A pointer to a block of chars containing the META EVENT,
 *        data.
 * size - The length of the meta-event data.
 */
int mf_write_meta_event(struct MF *mf, unsigned long delta_time, unsigned char type, unsigned char *data, unsigned long size)
{
	int i;

	WriteVarLen(mf, delta_time);

	/* This marks the fact we're writing a meta-event */
	eputc(mf, meta_event);

	/* The type of meta event */
	eputc(mf, type);

	/* The length of the data bytes to follow */
	WriteVarLen(mf, size);

	for (i = 0; i < size; i++) {
		if (eputc(mf, data[i]) != data[i])
			return (-1);
	}
	return (size);
}				/* end mf_write_meta_event */

void mf_write_tempo(struct MF *mf, unsigned long tempo)
{
	/* Write tempo */
	/* all tempos are written as 120 beats/minute, */
	/* expressed in microseconds/quarter note     */
	eputc(mf, 0);
	eputc(mf, meta_event);
	eputc(mf, set_tempo);

	eputc(mf, 3);
	eputc(mf, (unsigned) (0xff & (tempo >> 16)));
	eputc(mf, (unsigned) (0xff & (tempo >> 8)));
	eputc(mf, (unsigned) (0xff & tempo));
}

/*
 * Write multi-length bytes to MIDI format files
 */
static void WriteVarLen(struct MF *mf, unsigned long value)
{
	unsigned long buffer;

	buffer = value & 0x7f;
	while ((value >>= 7) > 0) {
		buffer <<= 8;
		buffer |= 0x80;
		buffer += (value & 0x7f);
	}
	while (1) {
		eputc(mf, (unsigned) (buffer & 0xff));

		if (buffer & 0x80)
			buffer >>= 8;
		else
			return;
	}
}				/* end of WriteVarLen */


/*
 * write32bit()
 * write16bit()
 *
 * These routines are used to make sure that the byte order of
 * the various data types remains constant between machines. This
 * helps make sure that the code will be portable from one system
 * to the next.  It is slightly dangerous that it assumes that longs
 * have at least 32 bits and ints have at least 16 bits, but this
 * has been true at least on PCs, UNIX machines, and Macintosh's.
 *
 */
static void write32bit(struct MF *mf, unsigned long data)
{
	eputc(mf, (unsigned) ((data >> 24) & 0xff));
	eputc(mf, (unsigned) ((data >> 16) & 0xff));
	eputc(mf, (unsigned) ((data >> 8) & 0xff));
	eputc(mf, (unsigned) (data & 0xff));
}

static void write16bit(struct MF *mf, int data)
{
	eputc(mf, (unsigned) ((data & 0xff00) >> 8));
	eputc(mf, (unsigned) (data & 0xff));
}

/* write a single character and abort on error */
static int eputc(struct MF *mf, unsigned char c)
{
	int return_val;

	if ((mf->Mf_putc) == NULLFUNC) {
		mferror(mf, "Mf_putc undefined");
		return (-1);
	}

	return_val = (mf->Mf_putc) (mf, c);

	if (return_val == EOF)
		mferror(mf, "error writing");

	mf->Mf_numbyteswritten++;
	return (return_val);
}


unsigned long mf_sec2ticks(float secs, int division, unsigned int tempo)
{
	return (long) (((secs * 1000.0) / 4.0 * division) / tempo);
}


/* 
 * This routine converts delta times in ticks into seconds. The
 * else statement is needed because the formula is different for tracks
 * based on notes and tracks based on SMPTE times.
 *
 */
float mf_ticks2sec(unsigned long ticks, int division, unsigned int tempo)
{
	float smpte_format, smpte_resolution;

	if (division > 0)
		return ((float)
			(((float) (ticks) * (float) (tempo)) /
			 ((float) (division) * 1000000.0)));
	else {
		smpte_format = upperbyte(division);
		smpte_resolution = lowerbyte(division);
		return (float) ((float) ticks /
				(smpte_format * smpte_resolution *
				 1000000.0));
	}
}				/* end of ticks2sec() */

/* code to utilize the interface */

#define TRACE(x, y) do { if (x) printf y; } while (0)


typedef unsigned long ulg;
typedef unsigned char uch;
typedef unsigned int ui;

struct MFX {
	struct MF mfi;

	int division;
	int trackstate;

	int prevnoteonpitch;	/* -1, nothing, 0 pause, 1-x note. */
	ulg prevnoteontime;

	gn_ringtone *ringtone;
	FILE *istream;
	gn_error err;
};

enum { TRK_NONE, TRK_READING, TRK_FINISHED };


static void lm_error(struct MF *mf, char *s);

static int lm_getc(struct MF *mf);
static void lm_header(struct MF *mf, int, int, int);
static void lm_trackstart(struct MF *mf);
static void lm_trackend(struct MF *mf);
static void lm_tempo(struct MF *, long);
static void lm_noteon(struct MF *, int, int, int);
static void lm_noteoff(struct MF *, int, int, int);



static void lm_error(struct MF *mf, char *s)
{
	struct MFX *mfx = (struct MFX *) mf;

	if (mfx->err == GN_ERR_NONE)
		mfx->err = GN_ERR_WRONGDATAFORMAT;
	fprintf(stderr, "%s\n", s);
}

static int lm_getc(struct MF *mf)
{
	struct MFX *mfx = (struct MFX *) mf;
	int ch;

	if ((ch = fgetc(mfx->istream)) == EOF)
		return -1;
	return ch;
}

static void lm_header(struct MF *mf, int format, int ntrks, int division)
{
	struct MFX *mfx = (struct MFX *) mf;

	TRACE(0, ("lm_header(%p, %d, %d, %d)\n", mf, format, ntrks, division));

	mfx->division = division;
}

/* this is just a quess */
static void lm_tempo(struct MF *mf, long tempo)
{
	struct MFX *mfx = (struct MFX *) mf;

	TRACE(0, ("lm_tempo(%p, %ld)\n", mf, tempo));

	if (mfx->trackstate != TRK_FINISHED)
		mfx->ringtone->tempo = 60000000 / tempo;
}


static void addnote(struct MFX *mfx, int pitch, int duration, int special)
{
	gn_ringtone_note *note;
	int scale;
	int lengths[] = { 4, 8, 16, 32, 64, 128 };
	            /*  c, c#,  d, d#,  e,  f, f#,  g, g#,  a, a#,  h */
	int notes[] = { 0,  1,  2,  3,  4,  6,  7,  8,  9, 10, 11, 12 };

	/* truncate the ringtone if it's too long */
	if (mfx->ringtone->notes_count == GN_RINGTONE_MAX_NOTES-1)
		return;

	note = mfx->ringtone->notes + mfx->ringtone->notes_count;
	mfx->ringtone->notes_count++;

	if (pitch == 0) {
		/* pause */
		note->note = 255;
	} else {
		pitch--;
		scale = pitch / 12;
		if (scale < 4)
			scale = 0;
		else if (scale > 7)
			scale = 3;
		else
			scale -= 4;
		note->note = 14 * (pitch / 12 - 4) + notes[pitch % 12];
	}

	if (duration < sizeof(lengths) / sizeof(lengths[0]))
		note->duration = lengths[duration];
	else
		note->duration = 4;
	if (special)
		note->duration *= 1.5;
}

/* currently supported */
static	/*  N  32  32. 16  16.   8    8.   4    4.   2    2.   1     1.  */
int vals[] = { 15, 38, 54, 78, 109, 156, 218, 312, 437, 625, 875, 1250 };

static void writenote(struct MFX *mfx, int delta)
{
	ulg millinotetime = delta * 250 / mfx->division;
	int i;
	int duration;
	int special;

	for (i = 0; i < sizeof vals / sizeof vals[0]; i++) {
		if (millinotetime < vals[i])
			break;
	}

	if (i == 0)
		return;

	i--;
	duration = i / 2;
	special = i & 1;

	addnote(mfx, mfx->prevnoteonpitch, duration, special);	/* XXX think this */
}


static void lm_trackstart(struct MF *mf)
{
	struct MFX *mfx = (struct MFX *) mf;

	TRACE(0, ("lm_trackstart(%p)\n", mf));

	if (mfx->trackstate == TRK_NONE)
		mfx->trackstate = TRK_READING;

	mfx->prevnoteonpitch = -1;
}

static void lm_trackend(struct MF *mf)
{
	struct MFX *mfx = (struct MFX *) mf;
	long time = mf->Mf_currtime;

	TRACE(0, ("lm_trackend(%p)\n", mf));

	if (mfx->trackstate == TRK_READING && mfx->ringtone->notes_count > 0)
		mfx->trackstate = TRK_FINISHED;

	if (mfx->prevnoteonpitch >= 0)
		writenote(mfx, time - mfx->prevnoteontime);

	mfx->prevnoteonpitch = -1;
}

static void lm_noteon(struct MF *mf, int chan, int pitch, int vol)
{
	struct MFX *mfx = (struct MFX *) mf;
	long time = mf->Mf_currtime;

	TRACE(0, ("lm_noteon(%p, %d, %d, %d)\n", mf, chan, pitch, vol));

	if (vol == 0)		/* kludge? to handle some (format 1? midi files) */
		return;

	if (mfx->trackstate != TRK_READING)
		return;

	if (mfx->prevnoteonpitch >= 0)
		writenote(mfx, time - mfx->prevnoteontime);

	if (vol == 0)
		mfx->prevnoteonpitch = 0;
	else
		mfx->prevnoteonpitch = pitch + 1;

	mfx->prevnoteontime = time;
}

static void lm_noteoff(struct MF *mf, int chan, int pitch, int vol)
{
	struct MFX *mfx = (struct MFX *) mf;
	long time = mf->Mf_currtime;

	TRACE(0, ("lm_noteoff(%p, %d, %d, %d)\n", mf, chan, pitch, vol));

	if (mfx->prevnoteonpitch >= 0) {
		writenote(mfx, time - mfx->prevnoteontime);
		mfx->prevnoteonpitch = -1;
	}
	mfx->prevnoteonpitch = 0;
	mfx->prevnoteontime = time;
}

static int lm_putc(struct MF *mf, int c)
{
	struct MFX *mfx = (struct MFX *) mf;

	return fputc(c, mfx->istream);
}

static long lm_getpos(struct MF *mf)
{
	struct MFX *mfx = (struct MFX *) mf;

	return ftell(mfx->istream);
}

static int lm_setpos(struct MF *mf, long pos)
{
	struct MFX *mfx = (struct MFX *) mf;

	return fseek(mfx->istream, pos, SEEK_SET);
}

static void lm_writetrack(struct MF *mf, int track)
{
	struct MFX *mfx = (struct MFX *) mf;
	gn_ringtone_note *note;
	int i, delta;
	int volume = 100;
	char data[2];
	int notes[] = {0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 11};

	mf_write_tempo(mf, 60000000 / mfx->ringtone->tempo);

	for (i = 0; i < mfx->ringtone->notes_count; i++) {
		note = mfx->ringtone->notes + i;
		delta = note->duration * mfx->division / 32;
		if (note->note == 0xff) {
			data[0] = 0;
			data[1] = 0;
			mf_write_midi_event(mf, delta, note_off, 1, data, 2);
		} else {
			data[0] = 12 * (4 + note->note / 14) + notes[note->note % 14];
			data[1] = volume;
			mf_write_midi_event(mf, 1, note_on, 1, data, 2);
			data[1] = 0;
			mf_write_midi_event(mf, delta, note_off, 1, data, 2);
		}
	}
}

gn_error file_midi_load(FILE * file, gn_ringtone * ringtone)
{
	struct MFX mfxi;
	struct MF *mf;

	ringtone->location = 0;
	snprintf(ringtone->name, sizeof(ringtone->name), "GNOKII");
	ringtone->tempo = 120;
	ringtone->notes_count = 0;

	memset(&mfxi, 0, sizeof(struct MFX));
	mf = &mfxi.mfi;

	mfxi.ringtone = ringtone;
	mfxi.istream = file;
	mfxi.err = GN_ERR_NONE;

	/* set variables to their initial values */
	mfxi.division = 0;
	mfxi.trackstate = TRK_NONE;
	mfxi.prevnoteonpitch = -1;

	mf->Mf_getc = lm_getc;

	mf->Mf_header = lm_header;
	mf->Mf_tempo = lm_tempo;
	mf->Mf_trackstart = lm_trackstart;
	mf->Mf_trackend = lm_trackend;
	mf->Mf_noteon = lm_noteon;
	mf->Mf_noteoff = lm_noteoff;

	mf->Mf_error = lm_error;

	midifile(mf);

	return mfxi.err;
}

gn_error file_midi_save(FILE * file, gn_ringtone * ringtone)
{
	struct MFX mfxi;
	struct MF *mf;

	memset(&mfxi, 0, sizeof(struct MFX));
	mf = &mfxi.mfi;

	mfxi.ringtone = ringtone;
	mfxi.istream = file;
	mfxi.err = GN_ERR_NONE;

	/* set variables to their initial values */
	mfxi.division = 480;

	mf->Mf_putc = lm_putc;
	mf->Mf_getpos = lm_getpos;
	mf->Mf_setpos = lm_setpos;
	mf->Mf_writetrack = lm_writetrack;
	mf->Mf_writetempotrack = NULLFUNC;

	mf->Mf_error = lm_error;

	mfwrite(mf, 0, 1, mfxi.division);

	return mfxi.err;
}
