# Read after the configured Qt Makefile. Private owned rescan integration.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
RESCAN_CPPFLAGS = -I$(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
RESCAN_OBJECTS = $(PROPOSAL_OUT)/test_plugin_cache_rescan.o $(PROPOSAL_OUT)/plugin_cache_rescan.o $(PROPOSAL_OUT)/plugin_cache_rescan_moc.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: plugin-cache-rescan-native
plugin-cache-rescan-native: $(PROPOSAL_OUT)/test_qt_plugin_cache_rescan.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_plugin_cache_rescan.moc: $(srcdir)/tests/test_plugin_cache_rescan.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/plugin_cache_rescan.moc.cpp: $(srcdir)/util/plugin_cache_rescan.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_plugin_cache_rescan.o: $(srcdir)/tests/test_plugin_cache_rescan.cpp $(PROPOSAL_OUT)/test_plugin_cache_rescan.moc
	$(CXX) $(RESCAN_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/plugin_cache_rescan.o: $(srcdir)/util/plugin_cache_rescan.cpp $(srcdir)/util/plugin_cache_rescan.hpp | $(PROPOSAL_OUT)
	$(CXX) $(RESCAN_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/plugin_cache_rescan_moc.o: $(PROPOSAL_OUT)/plugin_cache_rescan.moc.cpp
	$(CXX) $(RESCAN_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_probe.o: $(srcdir)/tests/library_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(RESCAN_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(RESCAN_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_plugin_cache_rescan.exe: $(RESCAN_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(RESCAN_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
