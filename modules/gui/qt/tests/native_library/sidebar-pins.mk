# Opt-in Windows native persistence test, after the configured Qt Makefile.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
PINS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
PINS_OBJECTS = $(PROPOSAL_OUT)/test_sidebar_pins_store.o $(PROPOSAL_OUT)/sidebar_pins_store.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: sidebar-pins-store-native
sidebar-pins-store-native: $(PROPOSAL_OUT)/test_qt_sidebar_pins_store.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_sidebar_pins_store.moc: $(srcdir)/tests/test_sidebar_pins_store.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_sidebar_pins_store.o: $(srcdir)/tests/test_sidebar_pins_store.cpp $(PROPOSAL_OUT)/test_sidebar_pins_store.moc
	$(CXX) -std=c++17 -DVLC_TEST_WRAP_SETTINGS_SYNC $(PINS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/sidebar_pins_store.o: $(srcdir)/maininterface/sidebar_pins_store.cpp $(srcdir)/maininterface/sidebar_pins_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PINS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp $(srcdir)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PINS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PINS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_sidebar_pins_store.exe: $(PINS_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PINS_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -Wl,--wrap=_ZN9QSettings4syncEv -lntdll -Wl,--strip-debug
