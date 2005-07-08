plugindir = $(IRSSI2_MODULEDIR)

plugin_LTLIBRARIES = irssi2_silc.la

INCLUDES = \
	$(IRSSI2_CFLAGS) \
	$(SILC_CFLAGS) \
	-Wall

irssi2_silc_la_LDFLAGS = \
	-avoid-version \
	-module \
	$(SILC_LIBS)

irssi2_silc_la_SOURCES = \
	base64.c base64.h \
	clientops.c clientops.h \
	clientops-notify.c \
	silc.c silc.h \
	silc-client.c silc-client.h \
	silc-channel.c silc-channel.h \
	silc-channel-connection.c silc-channel-connection.h \
	silc-gateway.c silc-gateway.h \
	silc-gateway-connection.c silc-gateway-connection.h \
	silc-local-presence.c silc-local-presence.h \
	silc-message.c silc-message.h \
	silc-presence.c silc-presence.h \
	support.c support.h
