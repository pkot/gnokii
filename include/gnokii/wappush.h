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

  Copyright (C) 2006 Igor Popik

  Include file for the binary SMS push functions. 
  
  Based on wapforum document WAP-167-ServiceInd-20010731-a

*/

#define WBXML_VERSION 0x01
#define WAPPush_CHARSET 0x6a /* UTF-8 106 */
#define PDU_TYPE_Push 0x06
#define CONTENT_TYPE 0xae /* application/vnd.wap.sic */

/* wbxml tag tokens masks */
typedef enum {
	TOKEN_KNOWN		= 0x00, /* token known */
	TOKEN_KNOWN_C		= 0x40, /* token known with content */
	TOKEN_KNOWN_A		= 0x80, /* token known with attributes */
	TOKEN_KNOWN_AC		= 0xC0	/* token known with attr & content */	
} gn_wap_push_tokens_masks;

/* Service indication tag tokens */
typedef enum {
	TAG_END			= 0x01,		/* end of TAG,ATTR list */
	TAG_INLINE		= 0x03,		/* inline string */
	TAG_SI			= 0x05,		/* public ID */
	TAG_INDICATION 		= 0x06, 	/* indication */
	TAG_INFO 		= 0x07,		/* info */
	TAG_ITEM 		= 0x08	 	/* item */
} gn_wap_push_tag_tokens;

typedef enum {
	ATTR_ACT_SIGNAL_NONE = 0x05,	/* action - signal-none */
	ATTR_ACT_SIGNAL_LOW = 0x06,	/* action - signal-low */
	ATTR_ACT_SIGNAL_MEDIUM = 0x07,	/* action - signal-medium */
	ATTR_ACT_SIGNAL_HIGH = 0x08,	/* action - signal-high */
	ATTR_ACT_DELETE = 0x09,		/* action - delete */
	ATTR_CREATED = 0x0a,		/* created */
	ATTR_HREF = 0x0b,		/* href */
	ATTR_HREF_HTTP = 0x0c,		/* http:// */
	ATTR_HREF_HTTP_WWW = 0x0d,	/* http://www. */
	ATTR_HREF_HTTPS = 0x0e,		/* https:// */
	ATTR_HREF_HTTPS_WWW = 0x0f,	/* https://www. */
	ATTR_SI_EXPRES = 0x10,
	ATTR_SI_ID = 0x11,
	ATTR_CLASS = 0x12
} gn_wap_push_tag_attr;
