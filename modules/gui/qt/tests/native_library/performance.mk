# Read after the configured native Qt Makefile. Opt-in owned controller tests.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
PERFORMANCE_CPPFLAGS = -I$(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
PERFORMANCE_OBJECTS = $(PROPOSAL_OUT)/test_performance_preferences.o $(PROPOSAL_OUT)/performance_preferences_model.o $(PROPOSAL_OUT)/performance_preferences_model_moc.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/plugin_cache_rescan.o $(PROPOSAL_OUT)/plugin_cache_rescan_moc.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: performance-preferences-native
performance-preferences-native: $(PROPOSAL_OUT)/test_qt_performance_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_performance_preferences.moc: $(srcdir)/tests/test_performance_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/performance_preferences_model.moc.cpp: $(srcdir)/maininterface/performance_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/performance_preferences_model_moc.o: $(PROPOSAL_OUT)/performance_preferences_model.moc.cpp
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/performance_preferences_model.o: $(srcdir)/maininterface/performance_preferences_model.cpp $(srcdir)/maininterface/performance_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_performance_preferences.o: $(srcdir)/tests/test_performance_preferences.cpp $(PROPOSAL_OUT)/test_performance_preferences.moc
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_probe.o: $(srcdir)/tests/library_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(PERFORMANCE_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_performance_preferences.exe: $(PERFORMANCE_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PERFORMANCE_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug

$(PROPOSAL_OUT)/plugin_cache_rescan.moc.cpp: $(srcdir)/util/plugin_cache_rescan.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/plugin_cache_rescan.o: $(srcdir)/util/plugin_cache_rescan.cpp $(srcdir)/util/plugin_cache_rescan.hpp | $(PROPOSAL_OUT)
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/plugin_cache_rescan_moc.o: $(PROPOSAL_OUT)/plugin_cache_rescan.moc.cpp
	$(CXX) $(PERFORMANCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
