/*

  G N O K I I

  A Linux/Unix toolset and driver for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 2009 Daniele Forsi

  This file provides functions specific to at commands on Huawei phones.
  See README for more details on supported mobile phones.

*/

#include "config.h"
#include "phones/atgen.h"
#include "phones/athuawei.h"

void at_huawei_init(char* foundmodel, char* setupmodel, struct gn_statemachine *state)
{
	/*
	 * Affected phones: Huawei E172 (E17X according to --identify)
	 *
	 * AT+CNMI=2,1 is supported but +CMTI notifications are sent only with
	 * AT+CNMI=1,1 and only on the second port (eg. /dev/ttyUSB1 not /dev/ttyUSB0)
	 */
	AT_DRVINST(state)->cnmi_mode = 1;
}
