noinst_LTLIBRARIES = libDEVICES.la

gnokii_devices	= tekram.c
if LIBUSB
gnokii_devices	+= dku2libusb.c
endif
if !WIN32
gnokii_devices	+= tcp.c
endif
if SOCKETPHONET
gnokii_devices	+= socketphonet.c
endif
if OSXBLUETOOTH
gnokii_devices	+= osxbluetooth.m
endif
if UNIXBLUETOOTH
gnokii_devices	+= unixbluetooth.c
endif
if UNIXIRDA
gnokii_devices	+= unixirda.c
endif
if !WIN32
gnokii_devices	+= unixserial.c
endif
if WINBLUETOOTH
gnokii_devices	+= winbluetooth.c
endif
if WINIRDA
gnokii_devices	+= winirda.c
endif
if WIN32
gnokii_devices	+= winserial.c
endif

libDEVICES_la_SOURCES = $(gnokii_devices)

libDEVICES_la_CFLAGS =		\
	-I$(top_srcdir)/include

libDEVICES_la_LIBADD =		\
	$(USB_LIBS)		\
	$(BLUETOOTH_LIBS)	\
	$(TCP_LIBS)
