# Maintained real-controller/WASAPI fixture. Reuse the existing native objects,
# excluding that executable's test main; no private controller/trace object.
ifndef AUDIO_OUT
$(error Set AUDIO_OUT)
endif
PAUSED_AUDIO_OBJECTS = $(filter-out tests/test_qt_native_bookmarks_chapters-test_native_bookmarks_chapters.$(OBJEXT),$(test_qt_native_bookmarks_chapters_OBJECTS))
.PHONY: native-paused-audio
native-paused-audio: $(AUDIO_OUT)/test_native_paused_audio_seek.exe
$(AUDIO_OUT):
	mkdir -p "$@"
$(AUDIO_OUT)/test_native_paused_audio_seek.moc: $(srcdir)/tests/test_native_paused_audio_seek.cpp | $(AUDIO_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(AUDIO_OUT)/test.o: $(srcdir)/tests/test_native_paused_audio_seek.cpp $(AUDIO_OUT)/test_native_paused_audio_seek.moc
	$(CXX) -std=c++17 $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(test_qt_native_bookmarks_chapters_CPPFLAGS) $(test_qt_native_bookmarks_chapters_CXXFLAGS) $(CXXFLAGS) -I$(AUDIO_OUT) -c "$<" -o "$@"
$(AUDIO_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(AUDIO_OUT)
	$(CXX) -std=c++17 $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(test_qt_native_bookmarks_chapters_CPPFLAGS) $(test_qt_native_bookmarks_chapters_CXXFLAGS) $(CXXFLAGS) -c "$<" -o "$@"
$(AUDIO_OUT)/test_native_paused_audio_seek.exe: $(PAUSED_AUDIO_OBJECTS) $(test_qt_native_bookmarks_chapters_DEPENDENCIES) $(AUDIO_OUT)/test.o $(AUDIO_OUT)/shutdown.o
	$(test_qt_native_bookmarks_chapters_LINK) $(PAUSED_AUDIO_OBJECTS) $(AUDIO_OUT)/test.o $(AUDIO_OUT)/shutdown.o $(test_qt_native_bookmarks_chapters_LDADD) $(LIBS) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
