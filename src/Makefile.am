plugindir = $(IRSSI2_MODULEDIR)

plugin_LTLIBRARIES = irssi2_silc.la

INCLUDES = \
	$(IRSSI2_CFLAGS) \
	$(SILC_CFLAGS)

irssi2_silc_la_LDFLAGS = \
	-avoid-version -module \
	$(SILC_LIBS)

irssi2_silc_la_SOURCES = \
	silc.c silc.h \
	silc-client.c silc-client.h \
	silc-gateway.c silc-gateway.h \
	silc-gateway-connection.c silc-gateway-connection.h \
	silc-channel.c silc-channel.h \
	silc-message.c silc-message.h \
	silc-presence.c silc-presence.h \
	clientops.c clientops.h \
	clientops-notify.c
