# Read after the configured native Qt Makefile. Opt-in owned controller tests.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
NETWORK_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
NETWORK_OBJECTS = $(PROPOSAL_OUT)/test_network_preferences.o $(PROPOSAL_OUT)/network_preferences_model.o $(PROPOSAL_OUT)/network_preferences_model_moc.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: network-preferences-native
network-preferences-native: $(PROPOSAL_OUT)/test_qt_network_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_network_preferences.moc: $(srcdir)/tests/test_network_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/network_preferences_model.moc.cpp: $(srcdir)/maininterface/network_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/network_preferences_model_moc.o: $(PROPOSAL_OUT)/network_preferences_model.moc.cpp
	$(CXX) $(NETWORK_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/network_preferences_model.o: $(srcdir)/maininterface/network_preferences_model.cpp $(srcdir)/maininterface/network_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) $(NETWORK_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_network_preferences.o: $(srcdir)/tests/test_network_preferences.cpp $(PROPOSAL_OUT)/test_network_preferences.moc
	$(CXX) $(NETWORK_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp | $(PROPOSAL_OUT)
	$(CXX) $(NETWORK_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_probe.o: $(srcdir)/tests/library_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(NETWORK_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(NETWORK_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_network_preferences.exe: $(NETWORK_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(NETWORK_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
