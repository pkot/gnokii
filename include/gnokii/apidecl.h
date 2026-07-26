/*

  G N O K I I

  A Linux/Unix toolset and driver for Nokia the phones.

  This file is part of gnokii.

  Header file for the GNOKII_API declaration.

*/

#ifndef _gnokii_apidecl_h
#define _gnokii_apidecl_h

#if defined(_WIN32) || defined(_WIN64)
#  if defined(LIBGNOKII_DLL_EXPORT)
#    define GNOKII_API __declspec(dllexport)
#  else
#    define GNOKII_API __declspec(dllimport)
#  endif
#elif (__GNUC__ >= 4 || __GNUC__ == 3 && __GNUC_MINOR__ > 3)
#  define GNOKII_API __attribute__ ((visibility("default")))
#else
#  define GNOKII_API
#endif

#endif
