/*

  S M S D

  A Linux/Unix tool for the mobile phones.

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

  Copyright (C) 1999 Pavel Janik ml., Hugh Blemings
  Copyright (C) 1999-2005 Jan Derfinak
  
  This file is a module to smsd for SQLite db server.

 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <glib.h>
#include "smsd.h"
#include "gnokii.h"
#include "compat.h"
#include "utils.h"

static sqlite3 *ppDbInbox;
static sqlite3 *ppDbOutbox;

GNOKII_API void DB_Bye(void)
{
    if (ppDbInbox)
        sqlite3_close(ppDbInbox);

    if (ppDbOutbox)
        sqlite3_close(ppDbOutbox);
}

GNOKII_API gint DB_ConnectInbox(DBConfig connect)
{
    int rc;
    rc = sqlite3_open(connect.db, &ppDbInbox);
    if (rc != SQLITE_OK) {
        g_print(_("Connection to database '%s' on failed.\n"), connect.db);
        g_print(_("Error: %s\n"), sqlite3_errmsg(ppDbInbox));
        return 1;
    }
    return 0;
}

GNOKII_API gint DB_ConnectOutbox(DBConfig connect)
{
    int rc;
    rc = sqlite3_open(connect.db, &ppDbOutbox);
    if (rc != SQLITE_OK) {
        g_print(_("Connection to database '%s' on failed.\n"), connect.db);
        g_print(_("Error: %s\n"), sqlite3_errmsg(ppDbOutbox));
        return 1;
    }
    return 0;
}

GNOKII_API gint DB_InsertSMS(const gn_sms * const data, const gchar * const phone)
{
    GString *buf, *phnStr, *text;
    time_t rawtime;
    struct tm * timeinfo;
    int error, i;
    sqlite3_stmt *stmt;

    if (phone[0] == '\0')
        phnStr = g_string_new("");
    else {
        phnStr = g_string_sized_new(32);
        g_string_printf(phnStr, "'%s',", phone);
    }

    text = g_string_sized_new(480);
    g_string_append(text, strEscape((gchar *) data->user_data[0].u.text));

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    buf = g_string_sized_new(1024);
    g_string_printf(buf, "INSERT INTO inbox (\"number\", \"smsdate\", \"insertdate\",\
                         \"text\", %s \"processed\") VALUES ('%s', \
                         '%02d-%02d-%02d %02d:%02d:%02d',\
                         '%02d-%02d-%02d %02d:%02d:%02d', '%s', %s 0)",
            phone[0] != '\0' ? "\"phone\"," : "", data->remote.number,
            data->smsc_time.year, data->smsc_time.month,
            data->smsc_time.day, data->smsc_time.hour,
            data->smsc_time.minute, data->smsc_time.second,
            timeinfo->tm_year + 1900, timeinfo->tm_mon,
            timeinfo->tm_mday, timeinfo->tm_hour,
            timeinfo->tm_min, timeinfo->tm_sec,
            text->str, phnStr->str);

    g_string_free(text, TRUE);
    g_string_free(phnStr, TRUE);

    error = sqlite3_prepare_v2(ppDbInbox, buf->str, -1, &stmt, NULL);
    if (error != SQLITE_OK) {
        g_print(_("%d: Parsing query %s failed!"), __LINE__, buf->str);
        g_print(_("Error: %s"), sqlite3_errmsg(ppDbInbox));
        return SMSD_NOK;
    }

    /* run query */
    for (i = 0; i < 6; i++) {
        error = sqlite3_step(stmt);
        if (error == SQLITE_DONE)
            break;

        sleep(1);
        sqlite3_reset(stmt);
    }

    if (error != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        g_print(_("%d: INSERT INTO inbox failed.\n"), __LINE__);
        g_print(_("Error %d: %s\n"), error, sqlite3_errmsg(ppDbInbox));
        g_print(_("Query: %s\n"), buf->str);
        g_string_free(buf, TRUE);
        return SMSD_NOK;
    }

    sqlite3_finalize(stmt);
    g_string_free(buf, TRUE);
    return SMSD_OK;
}

GNOKII_API void DB_Look(const gchar * const phone)
{
    GString *buf, *phnStr, *timebuf;
    gint ret1, numError, error;
    time_t rawtime;
    struct tm * timeinfo;
    sqlite3_stmt * stmt;

    if (phone[0] == '\0')
        phnStr = g_string_new("");
    else {
        phnStr = g_string_sized_new(32);
        g_string_printf(phnStr, "AND phone = '%s'", phone);
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    timebuf = g_string_sized_new(25);
    g_string_printf(timebuf, "'%02d:%02d:%02d'",
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    sqlite3_exec(ppDbOutbox, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    /* poll for outgoing messages */
    buf = g_string_sized_new(256);
    g_string_printf(buf, "SELECT id, number, text, dreport FROM outbox \
                        WHERE processed=0 \
                        AND %s >= not_before \
                        AND %s <= not_after \
                        %s", timebuf->str, timebuf->str, phnStr->str);

    g_string_free(phnStr, TRUE);

    ret1 = sqlite3_prepare_v2(ppDbOutbox, buf->str, -1, &stmt, NULL);
    if (ret1 != SQLITE_OK) {
        g_print(_("%d: Parsing query %s failed!"), __LINE__, buf->str);
        g_print(_("Error: %s"), sqlite3_errmsg(ppDbInbox));
        return;
    }

    g_string_printf(timebuf, "'%02d-%02d-%02d %02d:%02d:%02d'",
            timeinfo->tm_year, timeinfo->tm_mon,
            timeinfo->tm_mday, timeinfo->tm_hour,
            timeinfo->tm_min, timeinfo->tm_sec
            );

    ret1 = sqlite3_step(stmt);
    while (ret1 == SQLITE_ROW) {
        int gerror = 0;
        gn_sms sms;

        gn_sms_default_submit(&sms);
        memset(&sms.remote.number, 0, sizeof (sms.remote.number));
        sms.delivery_report = sqlite3_column_int(stmt, 3);

        strncpy(sms.remote.number, sqlite3_column_text(stmt, 1), sizeof (sms.remote.number) - 1);
        sms.remote.number[sizeof (sms.remote.number) - 1] = '\0';
        if (sms.remote.number[0] == '+')
            sms.remote.type = GN_GSM_NUMBER_International;
        else
            sms.remote.type = GN_GSM_NUMBER_Unknown;

        strncpy((gchar *) sms.user_data[0].u.text, sqlite3_column_text(stmt, 2), 10 * GN_SMS_MAX_LENGTH + 1);
        sms.user_data[0].u.text[10 * GN_SMS_MAX_LENGTH] = '\0';
        sms.user_data[0].length = strlen((gchar *) sms.user_data[0].u.text);
        sms.user_data[0].type = GN_SMS_DATA_Text;
        sms.user_data[1].type = GN_SMS_DATA_None;
        if (!gn_char_def_alphabet(sms.user_data[0].u.text))
            sms.dcs.u.general.alphabet = GN_SMS_DCS_UCS2;

        gn_log_xdebug("Sending SMS: %s, %s\n", sms.remote.number, sms.user_data[0].u.text);

        numError = 0;
        do {
            error = WriteSMS(&sms);
            sleep(1);
        } while ((error == GN_ERR_TIMEOUT || error == GN_ERR_FAILED) && numError++ < 3);

        /* mark sended */
        g_string_printf(buf, "UPDATE outbox SET processed=1, error='%d', \
                        processed_date=%s \
                        WHERE id=%d",
                gerror, timebuf->str, sqlite3_column_int(stmt, 0)
                );

        sqlite3_exec(ppDbOutbox, buf->str, NULL, NULL, NULL);
        ret1 = sqlite3_step(stmt);
    }

    /* rollback if found any errors */
    if (ret1 != SQLITE_DONE) {
        g_print(_("%d: SELECT FROM outbox command failed.\n"), __LINE__);
        g_print(_("Error: %s\n"), sqlite3_errmsg(ppDbOutbox));
        sqlite3_finalize(stmt);
        sqlite3_exec(ppDbOutbox, "ROLLBACK TRANSACTION;", NULL, NULL, NULL);

        g_string_free(timebuf, TRUE);
        g_string_free(buf, TRUE);
        return;
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(ppDbOutbox, "COMMIT;", NULL, NULL, NULL);

    g_string_free(timebuf, TRUE);
    g_string_free(buf, TRUE);
}
