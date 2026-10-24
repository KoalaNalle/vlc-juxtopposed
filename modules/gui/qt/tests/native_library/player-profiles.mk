# Opt-in owned profile persistence checks; no private code enters ordinary VLC.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
PROFILES_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
PROFILES_OBJECTS = $(PROPOSAL_OUT)/test_player_profiles_store.o $(PROPOSAL_OUT)/player_profiles_store.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
.PHONY: player-profiles-store-native
player-profiles-store-native: $(PROPOSAL_OUT)/test_qt_player_profiles_store.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_player_profiles_store.moc: $(srcdir)/tests/test_player_profiles_store.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_player_profiles_store.o: $(srcdir)/tests/test_player_profiles_store.cpp $(PROPOSAL_OUT)/test_player_profiles_store.moc
	$(CXX) -std=c++17 $(PROFILES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_profiles_store.o: $(srcdir)/maininterface/player_profiles_store.cpp $(srcdir)/maininterface/player_profiles_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp $(srcdir)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_qt_player_profiles_store.exe: $(PROFILES_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PROFILES_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
