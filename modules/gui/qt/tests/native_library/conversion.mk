# Read after the configured Qt Makefile. Opt-in real conversion-controller test.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
CONVERSION_CPPFLAGS = -I$(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
CONVERSION_OBJECTS = $(PROPOSAL_OUT)/test_conversion_controller.o $(PROPOSAL_OUT)/conversion_controller.o $(PROPOSAL_OUT)/conversion_controller_moc.o $(PROPOSAL_OUT)/soutchain.o $(PROPOSAL_OUT)/media_player_probe.o $(PROPOSAL_OUT)/network_stream_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: conversion-controller-native
conversion-controller-native: $(PROPOSAL_OUT)/test_qt_conversion_controller.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_conversion_controller.moc: $(srcdir)/tests/test_conversion_controller.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/conversion_controller.moc.cpp: $(srcdir)/dialogs/conversion_controller.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_conversion_controller.o: $(srcdir)/tests/test_conversion_controller.cpp $(PROPOSAL_OUT)/test_conversion_controller.moc
	$(CXX) $(CONVERSION_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/conversion_controller.o: $(srcdir)/dialogs/conversion_controller.cpp $(srcdir)/dialogs/conversion_controller.hpp | $(PROPOSAL_OUT)
	$(CXX) $(CONVERSION_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/conversion_controller_moc.o: $(PROPOSAL_OUT)/conversion_controller.moc.cpp
	$(CXX) $(CONVERSION_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/soutchain.o: $(srcdir)/util/soutchain.cpp | $(PROPOSAL_OUT)
	$(CXX) $(CONVERSION_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/media_player_probe.o: $(srcdir)/tests/media_player_probe.c | $(PROPOSAL_OUT)
	$(CC) $(CONVERSION_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/network_stream_probe.o: $(srcdir)/tests/network_stream_probe.c | $(PROPOSAL_OUT)
	$(CC) $(CONVERSION_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(CONVERSION_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_conversion_controller.exe: $(CONVERSION_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(CONVERSION_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
