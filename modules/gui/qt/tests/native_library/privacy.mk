# Read after the configured native Qt Makefile. Opt-in private native fixture.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh output directory)
endif
PRIVACY_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
PRIVACY_OBJECTS = $(PROPOSAL_OUT)/recent_network_history.o $(PROPOSAL_OUT)/test_privacy_preferences.o $(PROPOSAL_OUT)/privacy_preferences_model.o $(PROPOSAL_OUT)/privacy_preferences_model_moc.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/library_preferences_model.o $(PROPOSAL_OUT)/library_preferences_model_moc.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/privacy_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: privacy-preferences-native
privacy-preferences-native: $(PROPOSAL_OUT)/test_qt_privacy_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_privacy_preferences.moc: $(srcdir)/tests/test_privacy_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/%_preferences_model.moc.cpp: $(srcdir)/maininterface/%_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/%_preferences_model_moc.o: $(PROPOSAL_OUT)/%_preferences_model.moc.cpp
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/%_preferences_model.o: $(srcdir)/maininterface/%_preferences_model.cpp $(srcdir)/maininterface/%_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_privacy_preferences.o: $(srcdir)/tests/test_privacy_preferences.cpp $(PROPOSAL_OUT)/test_privacy_preferences.moc
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/%_preferences_probe.o: $(srcdir)/tests/%_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(PRIVACY_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_privacy_preferences.exe: $(PRIVACY_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PRIVACY_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug

$(PROPOSAL_OUT)/recent_network_history.o: $(srcdir)/util/recent_network_history.cpp $(srcdir)/util/recent_network_history.hpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@

# Real Home persistence used by the Library-off local clear callback.
PRIVACY_OBJECTS += $(PROPOSAL_OUT)/mlhomeactivitydata.o $(PROPOSAL_OUT)/new_playlist_data.o $(PROPOSAL_OUT)/mlqmltypes_moc.o
$(PROPOSAL_OUT)/test_qt_privacy_preferences.exe: $(PROPOSAL_OUT)/mlhomeactivitydata.o $(PROPOSAL_OUT)/new_playlist_data.o $(PROPOSAL_OUT)/mlqmltypes_moc.o
$(PROPOSAL_OUT)/mlhomeactivitydata.o: $(srcdir)/medialibrary/mlhomeactivitydata.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/new_playlist_data.o: $(srcdir)/dialogs/new_playlist_data.cpp | $(PROPOSAL_OUT)
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/mlqmltypes.moc.cpp: $(srcdir)/medialibrary/mlqmltypes.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/mlqmltypes_moc.o: $(PROPOSAL_OUT)/mlqmltypes.moc.cpp
	$(CXX) $(PRIVACY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
