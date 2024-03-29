dnl
dnl Makefile for the GNOKII tool suite.
dnl
dnl  Copyright (C) 1999 Hugh Blemings & Pavel Janik ml.
dnl                2000 Karel Zak, Pawel Kot
dnl                2002 BORBELY Zoltan
dnl

dnl
dnl IMPORTANT NOTE
dnl The first PKG_CHECK_MODULES call must not be conditional
dnl

AC_INIT([gnokii]
		[0.6.32git],
		[gnokii-users@nongnu.org],
		[gnokii],
		[http://www.gnokii.org/])

# Before making a release, the GNOKII_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C-A+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
GNOKII_LT_VERSION_CURRENT=7
GNOKII_LT_VERSION_REVISION=0
GNOKII_LT_VERSION_AGE=0
AC_SUBST(GNOKII_LT_VERSION_CURRENT)
AC_SUBST(GNOKII_LT_VERSION_REVISION)
AC_SUBST(GNOKII_LT_VERSION_AGE)

dnl xgnokii version
XVERSION=1.0

dnl smsd version
SVERSION=1.5.3

AC_CANONICAL_TARGET
AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_SRCDIR([gnokii/gnokii.c])
dnl gettext doesn't like the config.status being somewhere else :(
dnl AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.8 dist-bzip2])

AC_PREFIX_DEFAULT("/usr/local")

dnl ======================== Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_LEX
AC_PROG_OBJC
# If there's no flex/lex utility and there's no generated common/gnvcal.c file, we won't be able to compile
if test "x$LEX" = "x:" && test ! -f common/gnvcal.c; then
	echo "You need one of the following:"
	echo " - installed lex/flex"
	echo " - generated common/gnvcal.c file (included in release tarballs, not included in GIT)"
	exit 1
fi

AC_PATH_PROG(RM, rm, no)
AC_PATH_PROG(FIND, find, no)
AC_CHECK_PROGS(MAKE, gmake make)
AC_CHECK_PROGS(AR, ar)
AC_PROG_INSTALL

dnl ======================= Localisation and i18n

IT_PROG_INTLTOOL([0.35.0])
AM_LANGINFO_CODESET

dnl Add the languages which your application supports to po/LINGUAS
GETTEXT_PACKAGE=gnokii
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define to the Gettext package name])
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AM_ICONV


case x"$host_os" in
xfreebsd)
	CPPFLAGS="$CFLAGS -I/usr/local/include"
	LDFLAGS="$LDFLAGS -L/usr/local/lib"
	;;
xnetbsd*)
	CPPFLAGS="$CFLAGS -I/usr/pkg/include/"
	LDFLAGS="$LDFLAGS -L/usr/pkg/lib/"
	;;
esac

dnl ======================== Libtool versioning
_LT_SET_OPTION([LT_INIT],[win32-dll])
define([AC_PROVIDE_AC_LIBTOOL_WIN32_DLL])
LT_INIT

#
# Make libtool use --silent when --silent is passed to make
#
changequote(,)dnl
LIBTOOL="${LIBTOOL} \$(shell echo \"\$(MFLAGS)\" | awk '/^[^ ]*s/ { print \"--silent\" }')"
changequote([,])dnl

GNOKII_LT_VERSION=$GNOKII_LT_VERSION_CURRENT:$GNOKII_LT_VERSION_REVISION:$GNOKII_LT_VERSION_AGE
AC_SUBST(GNOKII_LT_VERSION)

# As used in common/gnokii.h
LIBGNOKII_VERSION_MAJOR=`expr $GNOKII_LT_VERSION_CURRENT - $GNOKII_LT_VERSION_AGE`
LIBGNOKII_VERSION_MINOR=$GNOKII_LT_VERSION_AGE
LIBGNOKII_VERSION_RELEASE=$GNOKII_LT_VERSION_REVISION
LIBGNOKII_VERSION_STRING="$LIBGNOKII_VERSION_MAJOR.$LIBGNOKII_VERSION_MINOR.$LIBGNOKII_VERSION_RELEASE"

AC_SUBST(LIBGNOKII_VERSION_MAJOR)
AC_SUBST(LIBGNOKII_VERSION_MINOR)
AC_SUBST(LIBGNOKII_VERSION_RELEASE)
AC_SUBST(LIBGNOKII_VERSION_STRING)
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [The current version])

dnl ======================== Default setting

dnl Let us have $prefix variable available here
test x"$prefix" = xNONE && prefix="$ac_default_prefix"

debug="yes"
xdebug="yes"
rlpdebug="yes"

AC_ARG_ENABLE(fulldebug, 
	[  --enable-fulldebug      compile with full debugging support],
	[ if test x$enable_fulldebug = xyes; then
		if test "x$GCC" = "xyes"; then
			CFLAGS="$CFLAGS -ggdb3 -Wall"
		fi
	  fi ]
)

dnl AC_ARG_ENABLE(debug, 
dnl	[  --enable-debug          compile with debug code],
dnl	[ if test x$enable_debug = xno; then debug="no"; fi ],
dnl	[ debug="yes" ]
dnl )
debug="yes"

AC_ARG_ENABLE(xdebug,
	[  --enable-xdebug         compile with xdebug code],
	[ if test x$enable_xdebug = xno; then xdebug="no"; fi ],
	[ xdebug="yes" ]
)

AC_ARG_ENABLE(rlpdebug,
	[  --enable-rlpdebug       compile with RLP debug code],
	[ if test x$enable_rlpdebug = xno; then rlpdebug="no"; fi ],
	[ rlpdebug="yes" ]
)

if test $debug = "yes"; then
	AC_DEFINE(DEBUG, 1, [Whether debug is turned on])
fi
if test $xdebug = "yes"; then
	AC_DEFINE(XDEBUG, 1, [Whether xdebug is turned on])
fi
if test $rlpdebug = "yes"; then
	AC_DEFINE(RLP_DEBUG, 1, [Whether rlpdebug is turned on])
fi

dnl ======================== Set gcc CFLAGS if we use gcc
if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall"
fi

dnl ======================== Set some gcc CFLAGS to reduce build warnings
if test "x$GCC" = "xyes"; then
  OLD_CFLAGS="$CFLAGS"

  CFLAGS="-Wno-pointer-sign"
  AC_MSG_CHECKING(if gcc supports $CFLAGS compiler option)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ int main() { return 0; } ]])],
  					[ac_cv_have_no_pointer_sign=yes],
					[ac_cv_have_no_pointer_sign=no])
  AC_MSG_RESULT($ac_cv_have_no_pointer_sign)
  CFLAGS="-fvisibility=hidden"
  AC_MSG_CHECKING(if gcc supports $CFLAGS compiler option)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ int main() { return 0; } ]])],
  					[ac_cv_have_hidden_visibility=yes],
					[ac_cv_have_hidden_visibility=no])
  AC_MSG_RESULT($ac_cv_have_hidden_visibility)
  CFLAGS="-fno-strict-aliasing"
  AC_MSG_CHECKING(if gcc supports $CFLAGS compiler option)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[ int main() { return 0; } ]])],
  					[ac_cv_have_strict_aliasing=yes],
					[ac_cv_have_strict_aliasing=no])
  AC_MSG_RESULT($ac_cv_have_strict_aliasing)

  CFLAGS="$OLD_CFLAGS"
  if test $ac_cv_have_no_pointer_sign = yes; then
    CFLAGS="$CFLAGS -Wno-pointer-sign"
  fi
  if test $ac_cv_have_hidden_visibility = yes; then
    CFLAGS="$CFLAGS -fvisibility=hidden"
  fi
  if test $ac_cv_have_strict_aliasing = yes; then
    CFLAGS="$CFLAGS -fno-strict-aliasing"
  fi
fi

dnl ======================== avoid deprecated warnings here
# CFLAGS="$CFLAGS -DGNOKII_DEPRECATED=''"

dnl ======================== Checks for libraries.

AC_ARG_WITH(libpthread,
   [  --with-libpthread=DIR   specifies the base libpthread],
   [ if test x$withval = xyes
     then
      AC_MSG_WARN(Usage is: --with-libpthread=DIR)
     else
      PTHREAD_LIBS="-L$withval/lib/"
      PTHREAD_CFLAGS="-I$withval/include/"
     fi
   ]
)

ACX_PTHREAD
if test "x$PTHREAD_CFLAGS" = "x-pthread"; then
	PTHREAD_LIBS="$PTHREAD_LIBS -pthread"
fi

dnl FIXME: do we really test here if libc contains this function?
if test "x$PTHREAD_LIBS" = xerror; then
   AC_CHECK_FUNC(pthread_attr_init, PTHREAD_LIBS="",
      [AC_MSG_WARN(not found library: pthread!!!)
       AC_MSG_WARN(disabling xgnokii and smsd)])
fi

AC_CACHE_CHECK(for tm_gmtoff in struct tm, ac_cv_have_tm_gmtoff,
	[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <time.h>]],
				[[struct tm t; t.tm_gmtoff = 0]])],
				[ac_cv_have_tm_gmtoff=yes],
				[ac_cv_have_tm_gmtoff=no])])
if test $ac_cv_have_tm_gmtoff = yes; then
	AC_DEFINE(HAVE_TM_GMTON, 1, [Whether tm_gmtoff is available])
fi

AC_CACHE_CHECK(for timer operations, ac_cv_have_timeops,
	[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <sys/time.h>]],
				[[struct timeval tv; timerisset(&tv); timerclear(&tv); timercmp(&tv, &tv, <); timeradd(&tv, &tv, &tv); timersub(&tv, &tv, &tv); ]])],
				[ac_cv_have_timeops=yes],
				[ac_cv_have_timeops=no])])
if test $ac_cv_have_timeops = yes; then
        AC_DEFINE(HAVE_TIMEOPS, 1, [Whether timer operations are available])
fi

have_termios="no"
dnl Checking for setspeed in termios.h
AC_CACHE_CHECK(for cfsetspeed in termios.h, ac_cv_have_cfsetspeed,
	[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]],
				[[struct termios t; cfsetspeed(&t, B9600);]])],
				[ac_cv_have_cfsetspeed=yes],
				[ac_cv_have_cfsetspeed=no])])
if test $ac_cv_have_cfsetspeed = yes; then
        AC_DEFINE(HAVE_CFSETSPEED, 1, [Whether cfsetspeed is available])
        have_termios="yes"
fi

if test $have_termios = "no"; then
	AC_CACHE_CHECK(for cfsetispeed and cfsetospeed in termios.h, ac_cv_have_cfsetiospeed,
		[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]],
				[[struct termios t; cfsetispeed(&t, B9600);  cfsetospeed(&t, B9600);]])],
				[ac_cv_have_cfsetiospeed=yes],
				[ac_cv_have_cfsetiospeed=no])])
	if test $ac_cv_have_cfsetiospeed = yes; then
		AC_DEFINE(HAVE_CFSETISPEED, 1, [Whether cfsetispeed is available])
		AC_DEFINE(HAVE_CFSETOSPEED, 1, [Whether cfsetospeed is available])
		have_termios="yes"
	fi
fi

if test $have_termios = "no"; then
	AC_CACHE_CHECK(for c_ispeed and c_ospeed in struct termios, ac_cv_have_c_iospeed,
		[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]],
				[[struct termios t; t.c_iflag = B9600; t.c_oflag = B9600;]])],
				[ac_cv_have_c_iospeed=yes],
				[ac_cv_have_c_iospeed=yes])])
	if test ac_cv_have_c_iospeed = yes; then
		AC_DEFINE(HAVE_TERMIOS_CSPEED, 1, [Whether c_ispeed and c_ospeed are available])
	fi
fi

dnl ======================== readline support
# Readline included by default unless explicitly asked not to
test "${with_readline+set}" != "set" && with_readline=yes

USE_READLINE="no"

AC_MSG_CHECKING(whether to use readline)
AC_ARG_WITH(readline,
[  --with-readline[=DIR]     Look for readline include/libs in DIR
  --without-readline      Don't include readline support],
[  case "$with_readline" in
  yes)
    AC_MSG_RESULT(yes)

    AC_CHECK_HEADERS(readline.h history.h readline/readline.h)
    AC_CHECK_HEADERS(readline/history.h)

    AC_CHECK_HEADERS(readline.h readline/readline.h,[
      for termlib in ncurses curses termcap terminfo termlib; do
	AC_CHECK_LIB(${termlib}, tgetent, [TERMLIBS="-l${termlib}"; break])
      done
      AC_CHECK_LIB(readline, readline,
        [TERMLIBS="-lreadline $TERMLIBS"
        AC_DEFINE([HAVE_READLINE], [1], [Use readline])
	USE_READLINE="yes"
        break], [TERMLIBS=], $TERMLIBS)])
    ;;
  no)
    AC_MSG_RESULT(no)
    ;;
  *)
    AC_MSG_RESULT(yes)

    # Needed for AC_CHECK_HEADERS and AC_CHECK_LIB to look at
    # alternate readline path
    _ldflags=${LDFLAGS}
    _cppflags=${CPPFLAGS}

    # Add additional search path
    LDFLAGS="-L$with_readline/lib $LDFLAGS"
    CPPFLAGS="-I$with_readline/include $CPPFLAGS"

    AC_CHECK_HEADERS(readline.h history.h readline/readline.h)
    AC_CHECK_HEADERS(readline/history.h)

    AC_CHECK_HEADERS(readline.h readline/readline.h,[
      for termlib in ncurses curses termcap terminfo termlib; do
	AC_CHECK_LIB(${termlib}, tgetent, [TERMLIBS="-l${termlib}"; break])
      done
      AC_CHECK_LIB(readline, readline,
        [TERMLDFLAGS="-L$with_readline/lib"
        TERMCPPFLAGS="-I$with_readline/include"
        CPPFLAGS="-I$with_readline/include $CPPFLAGS"
        TERMLIBS="-lreadline $TERMLIBS"
        AC_DEFINE([HAVE_READLINE], [1], [Use readline])
	USE_READLINE="yes"
        break], [TERMLIBS= CPPFLAGS=$_cppflags], $TERMLIBS)])

    LDFLAGS=$_ldflags
    ;;
  esac],
  AC_MSG_RESULT(no)
)
AC_SUBST(TERMLIBS)
AC_SUBST(TERMLDFLAGS)

dnl ======================== Check for libical
AC_ARG_WITH(libical,
   [  --with-libical=DIR      specifies the base libical],
   [ if test x$withval = xyes
     then
      AC_MSG_WARN(Usage is: --with-libical=DIR)
     else
      ICAL_LIBS="-L$withval/lib/"
      ICAL_CFLAGS="-I$withval/include/"
     fi
   ]
)

USE_LIBICAL="no"
AC_ARG_ENABLE(libical,
       	      AS_HELP_STRING([--disable-libical],
				 			[disable libical support (default is autodetected)]),,
       	      [enable_libical=yes])

if test "$enable_libical" = "yes"; then
	ICAL_LIBS="$ICAL_LIBS -lpthread -lical"
	OLD_CFLAGS="$CFLAGS"
	OLD_LIBS="$LIBS"
	LIBS="$LIBS $ICAL_LIBS"
	CFLAGS="$CFLAGS $ICAL_CFLAGS"
	AC_MSG_CHECKING(whether libical is installed)
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
	   #include <libical/ical.h>
	   ]], [[
	   int priority;
	   icalcomponent *pIcal=NULL;
	   struct icaltimetype atime = icaltime_from_string("20031231T223000");

	   pIcal = icalcomponent_vanew(
	      ICAL_VCALENDAR_COMPONENT,
	      icalproperty_new_version("2.0"),
	      icalproperty_new_prodid("//Gnokii.org//NONSGML v.0.5.8"),
	      0
	   );
	   atime.is_daylight = 1;
	   priority = icalproperty_get_priority(icalcomponent_get_first_property(pIcal, ICAL_PRIORITY_PROPERTY));
	   ]])],[
	      AC_MSG_RESULT(yes)
	      USE_LIBICAL="yes"
	      AC_DEFINE([HAVE_LIBICAL],[1],[Use libical])
	      AC_SUBST(ICAL_LIBS)
	      AC_SUBST(ICAL_CFLAGS)
	   ],[
	      AC_MSG_RESULT(no)
	      ICAL_LIBS=""
	   
	])
	LIBS="$OLD_LIBS"
	CFLAGS="$OLD_CFLAGS"
fi

dnl ======================== Check for libusb
USE_LIBUSB="no"
AC_ARG_ENABLE(libusb,
              AS_HELP_STRING([--disable-libusb],
			  				[disable libusb support (default is autodetected)]),,
              [enable_libusb=yes])
if test "$enable_libusb" = "yes"; then
	AC_MSG_CHECKING(whether libusb is installed)
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <usb.h>]],
		[[struct usb_dev_handle *dev;]])],
		[
			AC_MSG_RESULT(yes)
			AC_DEFINE([HAVE_LIBUSB],[1],[Use libusb])
			USE_LIBUSB="yes"
			USB_LIBS="-lusb"
			AC_SUBST(USB_LIBS)
		],[
			AC_MSG_RESULT(no)
		]
	)
fi

dnl ======================== Checks for Linux Phonet support
USE_SOCKETPHONET="no"
AC_ARG_ENABLE(phonet,
              AS_HELP_STRING([--disable-phonet],
			  				[disable phonet support (default is autodetected)]),,
              [enable_phonet=yes])
if test "$enable_phonet" = "yes"; then
	AC_CHECK_HEADER(linux/phonet.h,
		[AC_DEFINE(HAVE_SOCKETPHONET, 1, [Whether Phonet is available])
		 USE_SOCKETPHONET="yes"],,
		[#include <sys/socket.h>
		 #include <linux/phonet.h>])
fi

dnl ======================== Checks for gethostbyname support
AC_CHECK_FUNC(gethostbyname, ,
	AC_CHECK_LIB(nsl, gethostbyname, TCP_LIBS="-lnsl"
		     AC_SUBST(TCP_LIBS)))
dnl Haiku requires -lnetwork for socket functions
AC_CHECK_FUNC(gethostbyname, ,
	AC_CHECK_LIB(network, gethostbyname, TCP_LIBS="-lnetwork"
		     AC_SUBST(TCP_LIBS)))

dnl ======================== Checks for Linux IrDA support
USE_IRDA="no"
AC_ARG_ENABLE(irda,
              AS_HELP_STRING([--disable-irda],
			  				[disable irda support (default is autodetected)]),,
              [enable_irda=yes])
if test "$enable_irda" = "yes"; then
	AC_CHECK_HEADER(linux/irda.h,
		[AC_DEFINE(HAVE_IRDA, 1, [Whether IrDA is available])
		 USE_IRDA="yes"],,
		[#include <sys/socket.h>
		 #include <sys/ioctl.h>
		 #include <linux/types.h>])
fi

dnl ======================== Defines location for gettext
AC_ARG_WITH(gettext,
	[  --with-gettext=DIR      specifies the base gettext],
	[ if test x$withval = xyes; then
		AC_MSG_WARN(Usage is: --with-gettext=DIR)
	  else
		CFLAGS="$CFLAGS -I$withval"
	  fi
	]
)

dnl ======================== Checks for Bluetooth support
USE_BLUETOOTH="no"
AC_ARG_WITH(bluetooth,
	[  --with-bluetooth=DIR    specifies the base libbluetooth],
	[ if test x$withval = xyes; then
		AC_MSG_WARN(Usage is: --with-bluetooth=DIR)
	  else
		LIBS="$LIBS -L$withval/lib"
		CFLAGS="$CFLAGS -I$withval/net -I$withval/"
	  fi
	]
)
AC_ARG_ENABLE(bluetooth,
              AS_HELP_STRING([--disable-bluetooth],
			  				[disable Bluetooth support (default is autodetected)]),,
              [enable_bluetooth=yes])

case "$host_os" in
dnl ======================== Checks for Linux Bluetooth support
linux*)
	if test "$enable_bluetooth" = "yes"; then
		AC_MSG_NOTICE([checking for the Linux Bluetooth support])
		AC_CACHE_CHECK(for the struct sockaddr_rc in <bluetooth/rfcomm.h>, ac_cv_have_sockaddr_rc,
			[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>
					#include <bluetooth/bluetooth.h>
					#include <bluetooth/rfcomm.h>]],
					[[struct sockaddr_rc rc;]])],
					[ac_cv_have_sockaddr_rc=yes],
					[ac_cv_have_sockaddr_rc=no])])
		if test $ac_cv_have_sockaddr_rc = yes; then
			AC_DEFINE(HAVE_BLUETOOTH, 1, [Whether Bluetooth is available])
			AC_DEFINE(HAVE_BLUETOOTH_BLUEZ,[],[Compile on Linux])
			USE_BLUETOOTH="yes"
			BLUETOOTH_LIBS="-lbluetooth"
			AC_SUBST(BLUETOOTH_LIBS)
		fi
	fi
	;;
dnl ======================== Checks for MacOSX Bluetooth support
darwin*)
	if test "$enable_bluetooth" = "yes"; then
		AC_LANG_PUSH([Objective C])
		AC_CHECK_HEADERS(IOBluetooth/objc/IOBluetoothRFCOMMChannel.h)
		AC_CHECK_HEADERS(IOBluetooth/objc/IOBluetoothDevice.h)
		AC_MSG_CHECKING([for MacOS X Bluetooth support])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <IOBluetooth/objc/IOBluetoothRFCOMMChannel.h>
				#include <IOBluetooth/objc/IOBluetoothDevice.h>]],
				[[IOBluetoothDevice* btDevice = [[IOBluetoothDevice deviceWithAddressString:@"00:00:00:00:00:00"] autorelease];]])],
				[AC_MSG_RESULT(yes)
					ac_cv_have_iobluetooth=yes],
				[AC_MSG_RESULT(no)
					ac_cv_have_iobluetooth=no])
		AC_LANG_POP([Objective C])

		if test $ac_cv_have_iobluetooth = yes; then
			AC_DEFINE(HAVE_BLUETOOTH, 1, [Whether Bluetooth is available])
			AC_DEFINE(HAVE_BLUETOOTH_MACOSX,[],[Compile on Darwin / Mac OSX])
			USE_BLUETOOTH="yes"
			BLUETOOTH_LIBS="$PTHREAD_LIBS -Wl,-framework,CoreFoundation -Wl,-framework,IOBluetooth -Wl,-framework,Foundation"
			AC_SUBST(BLUETOOTH_LIBS)
		fi
	fi
	FOR_MAC=1
	;;

dnl ======================== Checks for FreeBSD/netgraph Bluetooth support
freebsd)
	if test "$enable_bluetooth" = "yes"; then
		AC_MSG_NOTICE([checking for the FreeBSD/netgraph Bluetooth support])
		AC_CACHE_CHECK(for the struct sockaddr_rfcomm from <bluetooth.h>, ac_cv_have_sockaddr_rfcomm,
			[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <bluetooth.h>]],
							[[struct sockaddr_rfcomm rc;]])],
							[ac_cv_have_sockaddr_rfcomm=yes],
							[ac_cv_have_sockaddr_rfcomm=no])])

		if test $ac_cv_have_sockaddr_rfcomm = yes; then
			AC_DEFINE(HAVE_BLUETOOTH, 1, [Whether Bluetooth is available])
			AC_DEFINE(HAVE_BLUETOOTH_NETGRAPH, [], [Compile on FreeBSD])
			USE_BLUETOOTH="yes"
			AC_CHECK_LIB(bluetooth, bt_aton,
				[BLUETOOTH_LIBS="-lbluetooth" ac_cv_have_bt_lib=yes],
				ac_cv_have_bt_lib=no)
			if test $ac_cv_have_bt_lib = yes; then
				AC_CHECK_LIB(sdp, sdp_open,
					[BLUETOOTH_LIBS="$BLUETOOTH_LIBS -lsdp"])
				AC_SUBST(BLUETOOTH_LIBS)
			fi
		fi
	fi
	;;

dnl ======================== Checks for NetBSD/netbt Bluetooth support
netbsd*)
	if test "$enable_bluetooth" = "yes"; then
		AC_MSG_NOTICE([checking for the NetBSD/netbt Bluetooth support])
		AC_CACHE_CHECK(for the struct sockaddr_bt from <bluetooth.h>, ac_cv_have_sockaddr_bt,
			[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <bluetooth.h>]],
							[[struct sockaddr_bt rc;]])],
							[ac_cv_have_sockaddr_bt=yes],
							[ac_cv_have_sockaddr_bt=no])])

		if test $ac_cv_have_sockaddr_bt = yes; then
			AC_DEFINE(HAVE_BLUETOOTH, 1, [Whether Bluetooth is available])
			AC_DEFINE(HAVE_BLUETOOTH_NETBT, [], [Compile on NetBSD])
			USE_BLUETOOTH="yes"
			CFLAGS="$CFLAGS -DCOMPAT_BLUEZ"
			AC_CHECK_LIB(bluetooth, bt_aton,
				[BLUETOOTH_LIBS="-lbluetooth" ac_cv_have_bt_lib=yes],
				ac_cv_have_bt_lib=no)
			if test $ac_cv_have_bt_lib = yes; then
				AC_CHECK_LIB(sdp, sdp_open,
					[BLUETOOTH_LIBS="$BLUETOOTH_LIBS -lsdp"])
				AC_SUBST(BLUETOOTH_LIBS)
			fi
		fi
	fi
	;;

cygwin32|cygwin|mingw32|mingw32msvc)
	AC_CHECK_HEADER(af_irda.h, [AC_DEFINE(HAVE_IRDA, 1, [Whether IrDA is available]) USE_IRDA="yes" LIBS="$LIBS -lwinmm"],, [#include <windows.h>])
	AC_CHECK_HEADER(ws2bth.h, [AC_DEFINE(HAVE_BLUETOOTH, 1, [Whether Bluetooth is available]) USE_BLUETOOTH="yes"],, [#include <windows.h>])
	if test x"$USE_IRDA" = "xyes" -o x"$USE_BLUETOOTH" = "xyes"; then
		LIBS="$LIBS -lws2_32"
	fi
	WIN32=1
	;;

esac

AM_CONDITIONAL([WIN32], [test "x$WIN32" = "x1"])
AM_CONDITIONAL([FOR_MAC], [test "x$FOR_MAC" = "x1"])

dnl ======================== Checks for X base support

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.0, found_gtk=yes, found_gtk=no)

if test "$no_x" = yes -o "$with_x" = "no"; then
   x_support="no"
   XPM_CFLAGS=""
   XPM_LIBS=""
   GTK_CFLAGS=""
   GTK_LIBS=""
else
   AC_PATH_X
   if test "x$x_libraries" = "xNONE"; then
      x_support="no"
      XPM_CFLAGS=""
      XPM_LIBS=""
      GTK_CFLAGS=""
      GTK_LIBS=""
      AC_MSG_WARN(Cannot find library libX11.)
      AC_MSG_WARN(Disabling xgnokii.)
   else
      if test "x$x_includes" != "x"; then
         XINCL="-I$x_includes"
      else
         XINCL=""
      fi
      if test "x$x_libraries" != "x"; then
         XLIBS="-L$x_libraries"
      else
         XLIBS=""
      fi
      CPPFLAGS="$CPPFLAGS $XINCL"
      AC_CHECK_HEADERS(X11/xpm.h,
          [ AC_CHECK_LIB(Xpm, XpmWriteFileFromXpmImage,
                [ XPM_CFLAGS="$XINCL"
                  XPM_LIBS="$XLIBS -lXpm -lX11"
                  AC_DEFINE(XPM, 1, [Whether the Xpm library is available]) ],
                AC_MSG_WARN(Cannot found library libXpm - disabling XPM support.),
                [ $XLIBS -lX11 ]
            )
          ]
      )
      if test "x$PTHREAD_LIBS" = xerror; then
         x_support="no"
         GTK_CFLAGS=""
         GTK_LIBS=""
      else
         if test "$found_gtk" = no; then
            x_support="no"
            GTK_CFLAGS=""
            GTK_LIBS=""
            AC_MSG_WARN(Cannot find GTK+ 2.0.)
            AC_MSG_WARN(Disabling xgnokii.)
         else
            x_support="yes"
         fi
      fi
   fi
fi

AM_CONDITIONAL(XGNOKII, test "x$x_support" = "xyes")

dnl ======================== Check for libsocket
AC_CHECK_LIB(socket, socket)

dnl ======================== Checks for glib support for libgnokii and smsd
PKG_CHECK_MODULES(GLIB, glib-2.0)

dnl ======================== Checks for gmodule support for smsd
PKG_CHECK_MODULES(GMODULE, gmodule-2.0, found_gmodule=yes, found_gmodule=no)

dnl ======================== Autodetect for possibilities to compile smsd
AC_ARG_ENABLE(smsd,
	AS_HELP_STRING([--disable-smsd],
				[disable smsd support (default is autodetected)]),,
	[enable_smsd=yes])

if test "$enable_smsd" = yes; then
	if test "$found_gmodule" = no; then
		AC_MSG_WARN(Cannot find gmodule 2.0.)
		AC_MSG_WARN(Disabling smsd.)
		enable_smsd="no"
	fi
fi

AM_CONDITIONAL(SMSD, test "x$enable_smsd" = "xyes")

dnl ======================== Checks for databases support for smsd

dnl === PostgreSQL
AC_ARG_ENABLE(postgres,
	AS_HELP_STRING([--disable-postgres],
				[disable PostgreSQL support (default is autodetected)]),
	[enable_postgres=$enableval],
	[enable_postgres=yes])
have_postgres=no
AC_PATH_PROG(PGCONFIG, pg_config, no)
if test x$enable_postgres != xno && test x$PGCONFIG != xno ; then
	have_postgres=yes
	pg_libdir=`$PGCONFIG --libdir`
	POSTGRES_LIBS="-L$pg_libdir -lpq"
	pg_includedir=`$PGCONFIG --includedir`
	POSTGRES_CFLAGS="-I$pg_includedir"
	AC_SUBST(POSTGRES_LIBS)
	AC_SUBST(POSTGRES_CFLAGS)
fi

AM_CONDITIONAL(HAVE_POSTGRES, test "x$have_postgres" = "xyes")

dnl === SQLite
AC_ARG_ENABLE(sqlite,
	AS_HELP_STRING([--disable-sqlite],
				[disable SQLite support (default is autodetected)]),
	[enable_sqlite=$enableval],
	[enable_sqlite=yes])
have_sqlite=no
PKG_CHECK_MODULES(SQLITE3, sqlite3, have_sqlite=yes, have_sqlite=no)
if test x$enable_sqlite != xno && test x$PKGCONF != xno ; then
	SQLITE_LIBS=$SQLITE3_LIBS
	SQLITE_CFLAGS=$SQLITE3_CFLAGS
	AC_SUBST(SQLITE_LIBS)
	AC_SUBST(SQLITE_CFLAGS)
fi
AM_CONDITIONAL(HAVE_SQLITE, test "x$have_sqlite" = "xyes")

dnl === MySQL
AC_ARG_ENABLE(mysql,
	AS_HELP_STRING([--disable-mysql],
				[disable MySQL support (default is autodetected)]),
	[enable_mysql=$enableval],
	[enable_mysql=yes])
have_mysql=no
dnl Prefer mysql_config5 if it is installed
AC_PATH_PROG(MYSQLCONFIG5, mysql_config5, no)
if test x$MYSQLCONFIG5 = xno ; then
	AC_PATH_PROG(MYSQLCONFIG, mysql_config, no)
else
	MYSQLCONFIG=$MYSQLCONFIG5
fi
if test x$enable_mysql != xno && test x$MYSQLCONFIG != xno ; then
	have_mysql=yes
	MYSQL_LIBS=`$MYSQLCONFIG --libs`
	MYSQL_CFLAGS=`$MYSQLCONFIG --cflags`
	AC_SUBST(MYSQL_LIBS)
	AC_SUBST(MYSQL_CFLAGS)
fi

AM_CONDITIONAL(HAVE_MYSQL, test "x$have_mysql" = "xyes")

dnl ======================== Additional switches

dnl ======================== Checks for libpcsclite
USE_LIBPCSCLITE="no"
AC_ARG_ENABLE(libpcsclite,
              AS_HELP_STRING([--disable-libpcsclite],[disable libpcsclite support (default is autodetected)
                            ]),,
              [enable_libpcsclite=yes])
if test "$enable_libpcsclite" = "yes"; then
	PKG_CHECK_MODULES(LIBPCSCLITE, libpcsclite >= 1.3.1, USE_LIBPCSCLITE=yes, USE_LIBPCSCLITE=no)
	if test "$USE_LIBPCSCLITE" = yes; then
		AC_DEFINE([HAVE_PCSC], [1], [Define if you have libpcsclite])
		OLD_CFLAGS="$CFLAGS"
		CFLAGS="$LIBPCSCLITE_CFLAGS $OLD_CFLAGS"
		AC_CHECK_TYPE([LPCSTR],
		    [AC_DEFINE([HAVE_LPCSTR], 1, 
		       [Define to 1 if libpcsclite defines the type 'LPCSTR'.])],
		    [], [#include <PCSC/wintypes.h>])
		CFLAGS="$OLD_CFLAGS"
	fi
fi

AC_ARG_ENABLE(unix98test,
   [  --enable-unix98test     if you want to disable UNIX98 test and assume to
                          use it; default is enabled],
   [ if test x$enable_unix98test = xno; then
	unix98test="no"
     else
	unix98test="yes"
     fi ],
   [ unix98test="yes"]
)

if test x$unix98test = xyes; then

AC_CHECK_FUNC(grantpt,
   [ AC_RUN_IFELSE([AC_LANG_SOURCE([[
#define  _XOPEN_SOURCE 500

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
	char *name = NULL;
        int master, err;

        master = open("/dev/ptmx", O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (master >= 0) {
                err = grantpt(master);
                err = err || unlockpt(master);
                if (!err) {
                        name = ptsname(master);
                } else {
                        exit(-1);
                }
        } else {
		exit(-1);
	}
	close(master);
	exit(0);
}
                ]])],
				[have_unix98=yes],
				[AC_MSG_WARN("No unix98ptys")],
				[have_unix98=yes])
   ]
)

fi

if test x$have_unix98 = xyes; then
	AC_DEFINE(USE_UNIX98PTYS, 1, [Whether Unix98 support is available])
fi

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#define _XOPEN_SOURCE 500
		 #include <sys/types.h>
		 #include <sys/socket.h>]],
		 [[struct msghdr msg; msg.msg_control;]])],
		 [AC_DEFINE(HAVE_MSGHDR_MSG_CONTROL, 1, Whether struct msghdr has a msg_control member)],
		 []
)

AC_PROG_EGREP

AC_HEADER_STDBOOL
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/time.h unistd.h sys/file.h)
AC_CHECK_HEADERS(string.h strings.h ctype.h stdlib.h stdarg.h stdint.h)
AC_CHECK_HEADERS(stddef.h sys/socket.h sys/modem.h termios.h sys/filio.h)
AC_CHECK_HEADERS(inttypes.h wchar.h direct.h sys/param.h dirent.h sys/types.h)
AC_CHECK_HEADERS(error.h errno.h signal.h)

dnl ======================== Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_STRUCT_TM
AC_CHECK_TYPE([u_int8_t],
    [AC_DEFINE([HAVE_U_INT8_T], 1, 
       [Define to 1 if the system has the type 'u_int8_t'.])])

dnl we don't check for long double and long long int, only
dnl ancient platforms don't have them
AC_DEFINE(HAVE_LONG_LONG, 1, [Whether the platform has a long long int type])
AC_DEFINE(HAVE_LONG_DOUBLE, 1, [Whether the platform has a long double type])

AC_CACHE_CHECK(for __ptr_t, ac_cv_c___ptr_t,
	[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]],
					[[__ptr_t foo = NULL;]])],
					[ac_cv_c___ptr_t=yes],
					[ac_cv_c___ptr_t=no])])
if test $ac_cv_c___ptr_t = yes; then
	AC_DEFINE(HAVE_PTR_T, 1, [Whether __ptr_t is available])
fi

dnl ======================== Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MEMCMP
AC_FUNC_STRFTIME
AC_FUNC_MKDIR
AC_CHECK_FUNCS(mktime timegm gettimeofday select poll wcrtomb)
AC_CHECK_FUNCS(strchr strdup strndup strstr strtol strtok strsep)
AC_CHECK_FUNCS(asprintf vasprintf snprintf vsnprintf getpass setenv)
AC_CHECK_FUNCS(getaddrinfo)
AC_CHECK_FUNCS(posix_spawn)
AM_CONDITIONAL([HAVE_POSIX_SPAWN], [test "x$ac_cv_func_posix_spawn" = xyes])
AC_CACHE_CHECK(for ISO C99 compliant snprintf,ac_cv_func_snprintf_c99,
	[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <stdio.h>

int main()
{
	char buf[] = {0, 0, 0, 0};

	snprintf(buf, 3, "ABC");
	exit ((buf[2] != 0) || (snprintf(NULL, 0, "%d", 100) != 3));
}]])],
				[ac_cv_func_snprintf_c99=yes],
				[ac_cv_func_snprintf_c99=no],
				[ac_cv_func_snprintf_c99=no])])
if test $ac_cv_func_snprintf_c99 = yes; then
	AC_DEFINE(HAVE_C99_SNPRINTF, 1, [Whether snprintf is available])
fi
AC_CACHE_CHECK(for ISO C99 compliant vsnprintf,ac_cv_func_vsnprintf_c99,
	[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int doit(char *buf, int len, const char *s, ...)
{
	va_list ap;
	int r;

	va_start(ap, s);
	r = vsnprintf(buf, len, s, ap);
	va_end(ap);

	return r;
}

int main()
{
	char buf[] = {0, 0, 0, 0};

	doit(buf, 3, "ABC");
	exit ((buf[2] != 0) || (doit(NULL, 0, "%d", 100) != 3));
}]])],
			[ac_cv_func_vsnprintf_c99=yes],
			[ac_cv_func_vsnprintf_c99=no],
			[ac_cv_func_vsnprintf_c99=no])])
if test $ac_cv_func_vsnprintf_c99 = yes; then
	AC_DEFINE(HAVE_C99_VSNPRINTF, 1, [Whether vsnprintf is available])
fi

CFLAGS="$CFLAGS $NLS_CFLAGS"
LIBS="$LIBS $NLS_LIBS"

XPACKAGE=xgnokii
AC_DEFINE_UNQUOTED(XVERSION, "$XVERSION", [The current xgnokii version])

HAVE_XGNOKII=$x_support

AC_DEFINE_UNQUOTED(SVERSION, "$SVERSION", [The current SMSD version])

case "$INSTALL" in
  'config/install-sh -c') INSTALL=`pwd`/$INSTALL
   ;;
esac

SHELL=${CONFIG_SHELL-/bin/sh}

AC_SUBST(SHELL)
case "$build_os" in
  solaris*) SHELL=/bin/ksh
   ;;
esac

AC_EXPAND_DIR(mgnokiidev_bindir, "${sbindir}")
AC_DEFINE_UNQUOTED(SBINDIR, "$mgnokiidev_bindir", [Where mgnokiidev gets installed])

AC_EXPAND_DIR(prefix_dir, "${prefix}")
AC_DEFINE_UNQUOTED(PREFIX, "$prefix_dir", [Root dir for gnokii, xgnokii and smsd])

AC_ARG_VAR([SUID_CFLAGS],
           [CFLAGS used for binaries which are usually with the suid bit])
AC_ARG_VAR([SUID_LDFLAGS],
           [LDFLAGS used for binaries which are usually with the suid bit])

AC_CONFIG_HEADERS([include/config.h])

AC_SUBST(PACKAGE)
AC_SUBST(VERSION)
AC_SUBST(XVERSION)
AC_SUBST(XPACKAGE)

AC_SUBST(exec_prefix)

AC_SUBST(GLDFLAGS)

AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

AC_SUBST(OWN_GETOPT)
AC_SUBST(USE_UNIX98PTYS)
AC_SUBST(NEED_LOCALE_CHARSET)

AC_SUBST(LIBS)

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(XPM_CFLAGS)
AC_SUBST(XPM_LIBS)
AC_SUBST(HAVE_XGNOKII)
AC_SUBST(WIN32)
AC_SUBST(WIN32_CROSS)

AC_SUBST(HAVE_LIBICAL)
AC_SUBST(HAVE_LIBUSB)
AC_SUBST(HAVE_SOCKETPHONET)

AC_CONFIG_FILES([Makefile
	  getopt/Makefile
	  Docs/Makefile
	  Docs/man/Makefile
	  Docs/protocol/Makefile
	  Docs/sample/Makefile
	  Docs/sample/logo/Makefile
	  Docs/sample/vCalendar/Makefile
	  Docs/sample/ringtone/Makefile
	  Docs/sample/ppp/Makefile
	  include/Makefile
	  include/gnokii.h
	  include/data/Makefile
	  include/devices/Makefile
	  include/phones/Makefile
	  include/links/Makefile
	  include/gnokii/Makefile
	  common/gnokii.pc
	  common/Makefile
	  common/data/Makefile
	  common/devices/Makefile
	  common/links/Makefile
	  common/phones/Makefile
	  gnokii/Makefile
	  gnokiid/Makefile
	  smsd/Makefile
	  snprintf/Makefile
	  utils/Makefile
	  xgnokii/xgnokii.pc
	  xgnokii/Makefile
	  xgnokii/xpm/Makefile
	  po/Makefile.in
	  testsuite/Makefile
	  testsuite/data/Makefile
	  Windows/Makefile
  ])
AC_OUTPUT

dnl ======================== Final report

echo "

  G N O K I I

  A toolset and driver for the mobile phones.

  Copyright (C) 1999-2011  The Gnokii Development Team.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  See file COPYING for more details.

    Host system:        $host_os
    Gnokii version:     $VERSION
    Xgnokii version:    $XVERSION
    SMSD version:       $SVERSION
    X (GTK+) support:   $x_support
    smsd support:       $enable_smsd
    Postgres support:   $have_postgres
    SQLite support:     $have_sqlite
    MySQL support:      $have_mysql
    Debug:              $debug
    XDebug:             $xdebug
    RLPDebug:           $rlpdebug
    NLS:                $USE_NLS
    IrDA:               $USE_IRDA
    ical read support:  $USE_LIBICAL
    Bluetooth:          $USE_BLUETOOTH
    libusb support:     $USE_LIBUSB
    phonet support:     $USE_SOCKETPHONET
    libpcsclite support:$USE_LIBPCSCLITE
    readline support:   $USE_READLINE
    Build Shared Lib:   $enable_shared
    Build Static Lib:   $enable_static
    Windows:            $win32 
    Prefix:             $prefix

  Type '${MAKE}' for compilation and then '${MAKE} install', to
  install gnokii.
"


