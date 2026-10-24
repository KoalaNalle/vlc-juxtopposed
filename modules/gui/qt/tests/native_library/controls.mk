# Read after the configured native Qt Makefile. Opt-in owned controller tests.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
CONTROLS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
CONTROLS_OBJECTS = $(PROPOSAL_OUT)/test_controls_preferences.o $(PROPOSAL_OUT)/controls_preferences_model.o $(PROPOSAL_OUT)/controls_preferences_model_moc.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: controls-preferences-native
controls-preferences-native: $(PROPOSAL_OUT)/test_qt_controls_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_controls_preferences.moc: $(srcdir)/tests/test_controls_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/controls_preferences_model.moc.cpp: $(srcdir)/maininterface/controls_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/controls_preferences_model_moc.o: $(PROPOSAL_OUT)/controls_preferences_model.moc.cpp
	$(CXX) $(CONTROLS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/controls_preferences_model.o: $(srcdir)/maininterface/controls_preferences_model.cpp $(srcdir)/maininterface/controls_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) $(CONTROLS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_controls_preferences.o: $(srcdir)/tests/test_controls_preferences.cpp $(PROPOSAL_OUT)/test_controls_preferences.moc
	$(CXX) $(CONTROLS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp | $(PROPOSAL_OUT)
	$(CXX) $(CONTROLS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_probe.o: $(srcdir)/tests/library_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(CONTROLS_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(CONTROLS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_controls_preferences.exe: $(CONTROLS_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(CONTROLS_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
