DEFS = -DCOMPILING_LIBGNOKII

noinst_LTLIBRARIES = libDEVICES.la

WIN32_FILES = \
	winserial.c \
	winirda.c \
	winbluetooth.c

UNIX_FILES = \
	unixserial.c \
	unixbluetooth.c \
	osxbluetooth.c \
	unixirda.c \
	tcp.c \
	socketphonet.c

if WIN32
PLATFORM_FILES = $(WIN32_FILES)
else
PLATFORM_FILES = $(UNIX_FILES)
endif

libDEVICES_la_SOURCES = \
	$(PLATFORM_FILES) \
	tekram.c \
	irda.c \
	dku2libusb.c \
	bluetooth.c

libDEVICES_la_CFLAGS =		\
	-I$(top_srcdir)/include

libDEVICES_la_LIBADD =		\
	$(USB_LIBS)		\
	$(BLUETOOTH_LIBS)	\
	$(TCP_LIBS)

EXTRA_DIST = $(WIN32_FILES) $(UNIX_FILES)

