# Opt-in actual native defaults transaction; no private code in ordinary VLC.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
PROFILE_DEFAULTS_CPPFLAGS = -iquote $(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
PROFILE_DEFAULTS_CFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src
PROFILE_DEFAULTS_OBJECTS = $(PROPOSAL_OUT)/test_player_profile_preferences.o $(PROPOSAL_OUT)/player_profile_preferences.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/general_preferences_probe.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: player-profile-preferences-native
player-profile-preferences-native: $(PROPOSAL_OUT)/test_qt_player_profile_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_player_profile_preferences.moc: $(srcdir)/tests/test_player_profile_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_player_profile_preferences.o: $(srcdir)/tests/test_player_profile_preferences.cpp $(srcdir)/maininterface/player_profile_preferences.hpp $(PROPOSAL_OUT)/test_player_profile_preferences.moc
	$(CXX) -std=c++17 $(PROFILE_DEFAULTS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_profile_preferences.o: $(srcdir)/maininterface/player_profile_preferences.cpp $(srcdir)/maininterface/player_profile_preferences.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_DEFAULTS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp $(srcdir)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_DEFAULTS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/general_preferences_probe.o: $(srcdir)/tests/general_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(PROFILE_DEFAULTS_CFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_DEFAULTS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_player_profile_preferences.exe: $(PROFILE_DEFAULTS_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PROFILE_DEFAULTS_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
