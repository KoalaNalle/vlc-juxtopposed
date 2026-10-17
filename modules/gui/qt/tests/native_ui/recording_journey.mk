# Opt-in own-window journey using the ordinary production objects.
QA_SOURCE ?= $(srcdir)/tests/native_ui/recording_journey.cpp
include $(srcdir)/tests/native_ui/network_journey.mk
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
