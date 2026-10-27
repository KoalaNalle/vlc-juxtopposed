# Opt-in console tests of saved defaults on actual native input/decoder tracks.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
PLAYBACK_CONSUMER_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
.PHONY: playback-consumers-native
playback-consumers-native: $(PROPOSAL_OUT)/test_qt_playback_track_consumers.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_playback_track_consumers.moc: $(srcdir)/tests/test_playback_track_consumers.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/playback_preferences_model.moc.cpp: $(srcdir)/maininterface/playback_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_playback_track_consumers.o: $(srcdir)/tests/test_playback_track_consumers.cpp $(PROPOSAL_OUT)/test_playback_track_consumers.moc | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playback_preferences_model.o: $(srcdir)/maininterface/playback_preferences_model.cpp $(srcdir)/maininterface/playback_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playback_preferences_model_moc.o: $(PROPOSAL_OUT)/playback_preferences_model.moc.cpp
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp $(srcdir)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/general_preferences_probe.o: $(srcdir)/tests/general_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playback_main_playlist_probe.o: $(srcdir)/tests/playback_main_playlist_probe.c | $(PROPOSAL_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_preferences_model.moc.cpp: $(srcdir)/maininterface/audio_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/audio_preferences_model.o: $(srcdir)/maininterface/audio_preferences_model.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_preferences_model_moc.o: $(PROPOSAL_OUT)/audio_preferences_model.moc.cpp
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_windows_devices.o: $(srcdir)/util/audio_windows_devices.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PLAYBACK_CONSUMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
PLAYBACK_CONSUMER_OBJECTS = $(addprefix $(PROPOSAL_OUT)/,test_playback_track_consumers.o playback_preferences_model.o playback_preferences_model_moc.o checked_preferences_store.o winpthreads_shutdown.o general_preferences_probe.o playback_main_playlist_probe.o audio_preferences_model.o audio_preferences_model_moc.o audio_windows_devices.o)
$(PROPOSAL_OUT)/test_qt_playback_track_consumers.exe: $(PLAYBACK_CONSUMER_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PLAYBACK_CONSUMER_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll $(LIBCOM) -luuid -Wl,--strip-debug
