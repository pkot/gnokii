/*

  S M S D

  A Linux/Unix tool for the mobile phones.

  This file is part of gnokii.

  Copyright (C) 1999 Pavel Janik ml., Hugh Blemings
  Copyright (C) 1999-2011 Jan Derfinak

  Utils functions for SMSD plugins
  
*/

#include "config.h"

#include "utils.h"

/* Escapes ' and \ with \. */
/* Returned value needs to be free with g_free(). */
gchar *strEscape (const gchar *const s)
{
  GString *str = g_string_new (s);
  register gint i = 0;
  gchar *ret;
  
  while (str->str[i] != '\0')
  {
    if (str->str[i] == '\\' || str->str[i] == '\'')
      g_string_insert_c (str, i++, '\\');
    i++;
  }
  
  ret = str->str;
  g_string_free (str, FALSE);
  
  return (ret);
}

/* Escapes ' with '' */
/* SQLite escapes the single quote by puttin two of them in a row, */
/* it doesn't need to escape the backslash */
/* Returned value needs to be free with g_free(). */
gchar *strEscapeSingleQuote (const gchar *const s)
{
  GString *str = g_string_new (s);
  register gint i = 0;
  gchar *ret;
  
  while (str->str[i] != '\0')
  {
    if (str->str[i] == '\'')
      g_string_insert_c (str, i++, '\'');
    i++;
  }
  
  ret = str->str;
  g_string_free (str, FALSE);
  
  return (ret);
}

