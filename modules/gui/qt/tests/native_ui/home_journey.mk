QA_SOURCE ?= $(srcdir)/tests/native_ui/home_journey.cpp
include $(srcdir)/tests/native_ui/network_journey.mk
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
