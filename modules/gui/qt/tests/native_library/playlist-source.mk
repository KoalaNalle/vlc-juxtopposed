# Opt-in actual native queue-entry origin tests. No windows or media playback.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
SOURCE_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
.PHONY: playlist-source-native
playlist-source-native: $(PROPOSAL_OUT)/test_qt_playlist_source.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_playlist_source.moc: $(srcdir)/tests/test_playlist_source.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/playlist_controller.moc.cpp: $(srcdir)/playlist/playlist_controller.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/playlist_item.moc.cpp: $(srcdir)/playlist/playlist_item.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/vlctick.moc.cpp: $(srcdir)/util/vlctick.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_playlist_source.o: $(srcdir)/tests/test_playlist_source.cpp $(PROPOSAL_OUT)/test_playlist_source.moc | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playlist_controller.o: $(srcdir)/playlist/playlist_controller.cpp $(srcdir)/playlist/playlist_controller.hpp $(srcdir)/playlist/playlist_controller_p.hpp $(srcdir)/playlist/media.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playlist_item.o: $(srcdir)/playlist/playlist_item.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/vlctick.o: $(srcdir)/util/vlctick.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playback_preferences_probe.o: $(srcdir)/tests/playback_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playlist_controller_moc.o: $(PROPOSAL_OUT)/playlist_controller.moc.cpp
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/playlist_item_moc.o: $(PROPOSAL_OUT)/playlist_item.moc.cpp
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/vlctick_moc.o: $(PROPOSAL_OUT)/vlctick.moc.cpp
	$(CXX) -std=c++17 $(SOURCE_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
SOURCE_OBJECTS = $(addprefix $(PROPOSAL_OUT)/,test_playlist_source.o playlist_controller.o playlist_item.o vlctick.o winpthreads_shutdown.o playback_preferences_probe.o playlist_controller_moc.o playlist_item_moc.o vlctick_moc.o)
$(PROPOSAL_OUT)/test_qt_playlist_source.exe: $(SOURCE_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(SOURCE_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
