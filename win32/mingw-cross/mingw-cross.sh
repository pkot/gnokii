#!/bin/sh

##### config begin #####
MINGW_PREFIX=mingw-
LEX=flex
MAKE=make
SECURITY='SECURITY'
NO_SHARED=
##### config end #######

CC="$MINGW_PREFIX"gcc
AR="$MINGW_PREFIX"ar
RANLIB="$MINGW_PREFIX"ranlib
VERSION=`cat VERSION`
confsubst=config.cache

cat <<EOF >$confsubst
s%@VERSION@%$VERSION%g
s%@SHELL@%/bin/sh%g
s%@RM@%rm%g
s%@FIND@%find%g
s%@MAKE@%$MAKE%g
s%@CC@%$CC%g
s%@CFLAGS@%-Wall -g%g
s%@CPPFLAGS@%%g
s%@AR@%$AR%g
s%@RANLIB@%$RANLIB%g
s%@LIBS@%%g
s%@LEX@%$LEX%g
s%@OWN_GETOPT@%1%g
s%@NO_SHARED@%$NO_SHARED%g
s%@WIN32@%1%g
s%@WIN32_CROSS@%1%g
s%@USE_NLS@%no%g
s%@XPM_LIBS@%%g
s%@GTK_LIBS@%%g
s%@PTHREAD_CFLAGS@%%g
EOF
sed -f $confsubst <Makefile.global.in > Makefile.global

cat <<EOF >$confsubst
s%#undef VERSION%#define VERSION "$VERSION"%g
s%#undef const%#define const const%g
EOF
for i in $SECURITY WIN32 \
	HAVE_ALLOCA HAVE_ALLOCA_H \
	HAVE_CTYPE_H HAVE_FCNTL_H HAVE_INTTYPES_H HAVE_LIMITS_H HAVE_MALLOC_H \
	HAVE_MEMORY_H HAVE_STDARG_H HAVE_STDDEF_H HAVE_STDINT_H HAVE_STDLIB_H \
	HAVE_STRING_H HAVE_SYS_FILE_H HAVE_SYS_PARAM_H HAVE_SYS_STAT_H \
	HAVE_SYS_TIME_H HAVE_SYS_TYPES_H \
	HAVE_LONG_LONG HAVE_LONG_DOUBLE HAVE_INTTYPES_H_WITH_UINTMAX \
	HAVE_UNSIGNED_LONG_LONG \
	HAVE_SNPRINTF HAVE_VSNPRINTF HAVE_GETCWD HAVE_MEMCPY HAVE_MKTIME \
	HAVE_PUTENV HAVE_STRCASECMP HAVE_STRDUP HAVE_STRFTIME HAVE_STRSTR \
	HAVE_STRTOK HAVE_STRTOL HAVE_STRTOUL
do
    echo "s%^#undef $i$%#define $i 1%g"
done >>$confsubst
sed -f $confsubst <include/config.h.in >include/config.h

rm $confsubst
