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

  Mainline code for gnokii utility. Monitor functions.

*/

#include "config.h"
#include "misc.h"
#include "compat.h"

#include <stdio.h>
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <getopt.h>
#include <signal.h>
#ifndef WIN32
#  include <fcntl.h>
#else
/*
#  include <io.h>
*/
#endif

#include "gnokii-app.h"
#include "gnokii.h"

static gn_cb_message cb_queue[16];
static int cb_ridx = 0;
static int cb_widx = 0;

void monitor_usage(FILE *f)
{
	fprintf(f, _("Monitoring options:\n"
		     "          --monitor [delay|once]\n"
		     "          --getdisplaystatus\n"
		     "          --displayoutput\n"
		     "          --netmonitor {reset|off|field|devel|next|nr}\n"
		));
}

static void callnotifier(gn_call_status call_status, gn_call_info *call_info, struct gn_statemachine *state, void *callback_data)
{
	switch (call_status) {
	case GN_CALL_Incoming:
		fprintf(stdout, _("INCOMING CALL: ID: %d, Number: %s, Name: \"%s\"\n"), call_info->call_id, call_info->number, call_info->name);
		break;
	case GN_CALL_LocalHangup:
		fprintf(stdout, _("CALL %d TERMINATED (LOCAL)\n"), call_info->call_id);
		break;
	case GN_CALL_RemoteHangup:
		fprintf(stdout, _("CALL %d TERMINATED (REMOTE)\n"), call_info->call_id);
		break;
	case GN_CALL_Established:
		fprintf(stdout, _("CALL %d ACCEPTED BY THE REMOTE SIDE\n"), call_info->call_id);
		break;
	case GN_CALL_Held:
		fprintf(stdout, _("CALL %d PLACED ON HOLD\n"), call_info->call_id);
		break;
	case GN_CALL_Resumed:
		fprintf(stdout, _("CALL %d RETRIEVED FROM HOLD\n"), call_info->call_id);
		break;
	default:
		break;
	}

	gn_call_notifier(call_status, call_info, state);
}

static void storecbmessage(gn_cb_message *message, struct gn_statemachine *state, void *callback_data)
{
	if (cb_queue[cb_widx].is_new) {
		/* queue is full */
		return;
	}

	cb_queue[cb_widx] = *message;
	cb_widx = (cb_widx + 1) % (sizeof(cb_queue) / sizeof(gn_cb_message));
}

static gn_error readcbmessage(gn_cb_message *message)
{
	if (!cb_queue[cb_ridx].is_new)
		return GN_ERR_NONEWCBRECEIVED;
	
	*message = cb_queue[cb_ridx];
	cb_queue[cb_ridx].is_new = false;
	cb_ridx = (cb_ridx + 1) % (sizeof(cb_queue) / sizeof(gn_cb_message));

	return GN_ERR_NONE;
}

static void displaycall(int call_id)
{
/* FIXME!!! */
#ifndef WIN32
	gn_call *call;
	struct timeval now, delta;
	char *s;

	if ((call = gn_call_get_active(call_id)) == NULL) {
		fprintf(stdout, _("CALL%d: IDLE\n"), call_id);
		return;
	}

	gettimeofday(&now, NULL);
	switch (call->status) {
	case GN_CALL_Ringing:
	case GN_CALL_Incoming:
		s = _("RINGING");
		timersub(&now, &call->start_time, &delta);
		break;
	case GN_CALL_Dialing:
		s = _("DIALING");
		timersub(&now, &call->start_time, &delta);
		break;
	case GN_CALL_Established:
		s = _("ESTABLISHED");
		timersub(&now, &call->answer_time, &delta);
		break;
	case GN_CALL_Held:
		s = _("ON HOLD");
		timersub(&now, &call->answer_time, &delta);
		break;
	default:
		s = _("UNKNOWN STATE");
		memset(&delta, 0, sizeof(delta));
		break;
	}

	fprintf(stdout, _("CALL%d: %s %s (%s) (callid: %d, duration: %d sec)\n"),
		call_id, s, call->remote_number, call->remote_name,
		call->call_id, (int)delta.tv_sec);
#endif
}

/* Same code is for getnetworkinfo */
void networkinfo_print(gn_network_info *info, void *callback_data)
{
	int cid, lac;

	/* Ugly, ugly, ... */
	if (info->cell_id[2] == 0 && info->cell_id[3] == 0)
		cid = (info->cell_id[0] << 8) + info->cell_id[1];
	else
		cid = (info->cell_id[0] << 24) + (info->cell_id[1] << 16) + (info->cell_id[2] << 8) + info->cell_id[3];
	lac = (info->LAC[0] << 8) + info->LAC[1];

	fprintf(stdout, _("LAC: %04x (%d), CellID: %08x (%d)\n"),
		lac, lac, cid, cid);
}

gn_error monitormode(int argc, char *argv[], gn_data *data, struct gn_statemachine *state)
{
	float rflevel = -1, batterylevel = -1;
	gn_power_source powersource = -1;
	gn_rf_unit rfunit = GN_RF_Arbitrary;
	gn_battery_unit battunit = GN_BU_Arbitrary;
	gn_error error;
	int i, d;

	gn_network_info networkinfo;
	gn_cb_message cbmessage;
	gn_memory_status simmemorystatus   = {GN_MT_SM, 0, 0};
	gn_memory_status phonememorystatus = {GN_MT_ME, 0, 0};
	gn_memory_status dc_memorystatus   = {GN_MT_DC, 0, 0};
	gn_memory_status en_memorystatus   = {GN_MT_EN, 0, 0};
	gn_memory_status fd_memorystatus   = {GN_MT_FD, 0, 0};
	gn_memory_status ld_memorystatus   = {GN_MT_LD, 0, 0};
	gn_memory_status mc_memorystatus   = {GN_MT_MC, 0, 0};
	gn_memory_status on_memorystatus   = {GN_MT_ON, 0, 0};
	gn_memory_status rc_memorystatus   = {GN_MT_RC, 0, 0};

	gn_sms_status smsstatus = {0, 0, 0, 0};
	errno = 0;
	if (argc > optind) {
		d = !strcasecmp(argv[optind], "once") ? -1 : gnokii_atoi(argv[optind]);
		if (errno) {
			fprintf(stderr, _("Argument to --monitor delay needs to be positive number\n"));
			return GN_ERR_FAILED;
		}
	} else
		d = 1;

	gn_data_clear(data);

	/*
	 * We do not want to monitor serial line forever - press Ctrl+C to
	 * stop the monitoring mode.
	 */
	signal(SIGINT, interrupted);

	fprintf(stderr, _("Entering monitor mode...\n"));

	/*
	 * Loop here indefinitely - allows you to see messages from GSM code
	 * in response to unknown messages etc. The loops ends after pressing
	 * the Ctrl+C.
	 */
	data->rf_unit = &rfunit;
	data->rf_level = &rflevel;
	data->battery_unit = &battunit;
	data->battery_level = &batterylevel;
	data->power_source = &powersource;
	data->sms_status = &smsstatus;
	data->network_info = &networkinfo;
	data->on_cell_broadcast = storecbmessage;
	data->call_notification = callnotifier;
	data->reg_notification = networkinfo_print;
	data->callback_data = NULL;

	gn_sm_functions(GN_OP_SetCallNotification, data, state);

	memset(cb_queue, 0, sizeof(cb_queue));
	cb_ridx = 0;
	cb_widx = 0;
	gn_sm_functions(GN_OP_SetCellBroadcast, data, state);

	memset(&networkinfo, 0, sizeof(gn_network_info));
	if (gn_sm_functions(GN_OP_GetNetworkInfo, data, state) == GN_ERR_NONE) {
		fprintf(stdout, _("Network: %s, %s (%s)\n"),
			gn_network_name_get(networkinfo.network_code), gn_country_name_get(networkinfo.network_code),
			(networkinfo.network_code ? networkinfo.network_code : _("undefined")));
		networkinfo_print(&networkinfo, NULL);
	}

	while (!bshutdown) {
		if (gn_sm_functions(GN_OP_GetRFLevel, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("RFLevel: %d\n"), (int)rflevel);

		if (gn_sm_functions(GN_OP_GetBatteryLevel, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("Battery: %d\n"), (int)batterylevel);

		if (gn_sm_functions(GN_OP_GetPowersource, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("Power Source: %s\n"), gn_power_source2str(powersource));

		data->memory_status = &simmemorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("SIM: Used %d, Free %d\n"), simmemorystatus.used, simmemorystatus.free);

		data->memory_status = &phonememorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("Phone: Used %d, Free %d\n"), phonememorystatus.used, phonememorystatus.free);

		data->memory_status = &dc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("DC: Used %d, Free %d\n"), dc_memorystatus.used, dc_memorystatus.free);

		data->memory_status = &en_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("EN: Used %d, Free %d\n"), en_memorystatus.used, en_memorystatus.free);

		data->memory_status = &fd_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("FD: Used %d, Free %d\n"), fd_memorystatus.used, fd_memorystatus.free);

		data->memory_status = &ld_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("LD: Used %d, Free %d\n"), ld_memorystatus.used, ld_memorystatus.free);

		data->memory_status = &mc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("MC: Used %d, Free %d\n"), mc_memorystatus.used, mc_memorystatus.free);

		data->memory_status = &on_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("ON: Used %d, Free %d\n"), on_memorystatus.used, on_memorystatus.free);

		data->memory_status = &rc_memorystatus;
		if (gn_sm_functions(GN_OP_GetMemoryStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("RC: Used %d, Free %d\n"), rc_memorystatus.used, rc_memorystatus.free);

		if (gn_sm_functions(GN_OP_GetSMSStatus, data, state) == GN_ERR_NONE)
			fprintf(stdout, _("SMS Messages: Unread %d, Number %d\n"), smsstatus.unread, smsstatus.number);

		gn_call_check_active(state);
		for (i = 0; i < GN_CALL_MAX_PARALLEL; i++)
			displaycall(i);
		if (readcbmessage(&cbmessage) == GN_ERR_NONE)
			fprintf(stdout, _("Cell broadcast received on channel %d: %s\n"), cbmessage.channel, cbmessage.message);

		if (d < 0)
			break;
		sleep(d);
	}

	data->on_cell_broadcast = NULL;
	error = gn_sm_functions(GN_OP_SetCellBroadcast, data, state);

	fprintf(stderr, _("Leaving monitor mode...\n"));

	return error;
}

static void printdisplaystatus(int status)
{
	fprintf(stdout, _("Call in progress: %-3s\n"),
		status & (1 << GN_DISP_Call_In_Progress) ? _("on") : _("off"));
	fprintf(stdout, _("Unknown: %-3s\n"),
		status & (1 << GN_DISP_Unknown)          ? _("on") : _("off"));
	fprintf(stdout, _("Unread SMS: %-3s\n"),
		status & (1 << GN_DISP_Unread_SMS)       ? _("on") : _("off"));
	fprintf(stdout, _("Voice call: %-3s\n"),
		status & (1 << GN_DISP_Voice_Call)       ? _("on") : _("off"));
	fprintf(stdout, _("Fax call active: %-3s\n"),
		status & (1 << GN_DISP_Fax_Call)         ? _("on") : _("off"));
	fprintf(stdout, _("Data call active: %-3s\n"),
		status & (1 << GN_DISP_Data_Call)        ? _("on") : _("off"));
	fprintf(stdout, _("Keyboard lock: %-3s\n"),
		status & (1 << GN_DISP_Keyboard_Lock)    ? _("on") : _("off"));
	fprintf(stdout, _("SMS storage full: %-3s\n"),
		status & (1 << GN_DISP_SMS_Storage_Full) ? _("on") : _("off"));
}

#ifdef WIN32
#  define ESC "\033"
#else
#  define ESC "\e"
#endif

static void newoutputfn(gn_display_draw_msg *drawmessage)
{
	int x, y, n;
	static int status;
	static unsigned char screen[GN_DRAW_SCREEN_MAX_HEIGHT][GN_DRAW_SCREEN_MAX_WIDTH];
	static bool init = false;

	if (!init) {
		for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++)
			for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
				screen[y][x] = ' ';
		status = 0;
		init = true;
	}

	fprintf(stdout, ESC "[1;1H");

	switch (drawmessage->cmd) {
	case GN_DISP_DRAW_Clear:
		for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++)
			for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
				screen[y][x] = ' ';
		break;

	case GN_DISP_DRAW_Text:
		x = drawmessage->data.text.x*GN_DRAW_SCREEN_MAX_WIDTH / 84;
		y = drawmessage->data.text.y*GN_DRAW_SCREEN_MAX_HEIGHT / 48;
		n = strlen(drawmessage->data.text.text);
		if (n > GN_DRAW_SCREEN_MAX_WIDTH)
			return;
		if (x + n > GN_DRAW_SCREEN_MAX_WIDTH)
			x = GN_DRAW_SCREEN_MAX_WIDTH - n;
		if (y > GN_DRAW_SCREEN_MAX_HEIGHT)
			y = GN_DRAW_SCREEN_MAX_HEIGHT - 1;
		memcpy(&screen[y][x], drawmessage->data.text.text, n);
		break;

	case GN_DISP_DRAW_Status:
		status = drawmessage->data.status;
		break;

	default:
		return;
	}

	for (y = 0; y < GN_DRAW_SCREEN_MAX_HEIGHT; y++) {
		for (x = 0; x < GN_DRAW_SCREEN_MAX_WIDTH; x++)
			fprintf(stdout, "%c", screen[y][x]);
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
	printdisplaystatus(status);
}

gn_error displayoutput(gn_data *data, struct gn_statemachine *state)
{
	gn_error error;
	gn_display_output output;

	gn_data_clear(data);
	memset(&output, 0, sizeof(output));
	output.output_fn = newoutputfn;
	data->display_output = &output;

	error = gn_sm_functions(GN_OP_DisplayOutput, data, state);
	console_raw();
#ifndef WIN32
	fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);
#endif

	switch (error) {
	case GN_ERR_NONE:
		/* We do not want to see texts forever - press Ctrl+C to stop. */
		signal(SIGINT, interrupted);

		fprintf (stderr, _("Entered display monitoring mode...\n"));
		fprintf (stderr, ESC "c" );

		/* Loop here indefinitely - allows you to read texts from phone's
		   display. The loops ends after pressing the Ctrl+C. */
		while (!bshutdown) {
			char buf[105];
			memset(&buf[0], 0, 102);
/*			while (read(0, buf, 100) > 0) {
				fprintf(stderr, _("handling keys (%d).\n"), strlen(buf));
				if (GSM && GSM->HandleString && GSM->HandleString(buf) != GN_ERR_NONE)
					fprintf(stderr, _("Key press simulation failed.\n"));
				memset(buf, 0, 102);
			}*/
			gn_sm_loop(1, state);
			gn_sm_functions(GN_OP_PollDisplay, data, state);
		}
		fprintf (stderr, _("Shutting down\n"));

		fprintf (stderr, _("Leaving display monitor mode...\n"));

		output.output_fn = NULL;
		error = gn_sm_functions(GN_OP_DisplayOutput, data, state);
		if (error != GN_ERR_NONE)
			fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	default:
		fprintf (stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}

	return error;
}

/* Getting the status of the display. */
gn_error getdisplaystatus(gn_data *data, struct gn_statemachine *state)
{
	gn_error error = GN_ERR_INTERNALERROR;
	int status;

	gn_data_clear(data);
	data->display_status = &status;

	error = gn_sm_functions(GN_OP_GetDisplayStatus, data, state);

	switch (error) {
	case GN_ERR_NONE:
		printdisplaystatus(status);
		break;
	default:
		fprintf(stderr, _("Error: %s\n"), gn_error_print(error));
		break;
	}
	
	return error;
}

/* Displays usage of --netmonitor command */
int netmonitor_usage(FILE *f, int exitval)
{
	fprintf(f, _(" usage: --netmonitor {reset|off|field|devel|next|nr}\n"
		));
	return exitval;
}

gn_error netmonitor(char *m, gn_data *data, struct gn_statemachine *state)
{
	unsigned char mode;
	gn_netmonitor nm;
	gn_error error;

	if (!strcmp(m, "reset"))
		mode = 0xf0;
	else if (!strcmp(m, "off"))
		mode = 0xf1;
	else if (!strcmp(m, "field"))
		mode = 0xf2;
	else if (!strcmp(m, "devel"))
		mode = 0xf3;
	else if (!strcmp(m, "next"))
		mode = 0x00;
	else {
		mode = gnokii_atoi(m);
		if (errno || mode < 1 || mode >= 0xf0)
			return netmonitor_usage(stderr, -1);
	}

	nm.field = mode;
	memset(&nm.screen, 0, 50);
	data->netmonitor = &nm;

	error = gn_sm_functions(GN_OP_NetMonitor, data, state);

	if (nm.screen)
		fprintf(stdout, "%s\n", nm.screen);

	return error;
}
