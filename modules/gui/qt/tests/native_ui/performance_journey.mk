# Opt-in own-window journey using the ordinary production objects.
# Keep the huge debug sections out of the owned test DLL on the travel HDD.
QA_SOURCE ?= $(srcdir)/tests/native_ui/performance_journey.cpp
include $(srcdir)/tests/native_ui/network_journey.mk
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
