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
  Copyright (C) 2002      Pawel Kot

  Header file for GSM networks.

*/

#ifndef _gnokii_networks_h
#define _gnokii_networks_h

/* This type is used to hold information about various GSM networks. */
typedef struct {
	char *code; /* GSM network code */
	char *name; /* GSM network name */
} gn_network;

/* This type is used to hold information about various GSM countries. */
typedef struct {
	char *code; /* GSM country code */
	char *name; /* GSM country name */
} gn_country;

GNOKII_API char *gn_network_name_get(char *network_code);
GNOKII_API char *gn_network_code_get(char *network_name);
GNOKII_API char *gn_network_code_find(char *network_name, char *country_name);

GNOKII_API char *gn_country_name_translate(char *country_name);
GNOKII_API char *gn_country_name_get(char *country_code);
GNOKII_API char *gn_country_code_get(char *country_name);

GNOKII_API int gn_network_get(gn_network *network, int index);
GNOKII_API int gn_country_get(gn_country *country, int index);

GNOKII_API char *gn_network2country(char *network);

#endif	/* _gnokii_networks_h */
