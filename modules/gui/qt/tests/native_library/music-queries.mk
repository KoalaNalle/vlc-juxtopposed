# Opt-in real native media-library queries; no desktop or private GUI hooks.
ifndef MUSIC_QUERIES_OUT
$(error Set MUSIC_QUERIES_OUT to a fresh owned output directory)
endif
MUSIC_QUERIES_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(MUSIC_QUERIES_OUT)
.PHONY: music-queries-native
music-queries-native: $(MUSIC_QUERIES_OUT)/test_qt_music_library_queries.exe
$(MUSIC_QUERIES_OUT):
	mkdir -p "$@"
$(MUSIC_QUERIES_OUT)/test_music_library_queries.moc: $(srcdir)/tests/test_music_library_queries.cpp | $(MUSIC_QUERIES_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(MUSIC_QUERIES_OUT)/queries.o: $(srcdir)/tests/test_music_library_queries.cpp $(MUSIC_QUERIES_OUT)/test_music_library_queries.moc
	$(CXX) -std=c++17 $(MUSIC_QUERIES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(MUSIC_QUERIES_OUT)/probe.o: $(srcdir)/tests/network_stream_probe.c | $(MUSIC_QUERIES_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c "$<" -o "$@"
$(MUSIC_QUERIES_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(MUSIC_QUERIES_OUT)
	$(CXX) -std=c++17 $(MUSIC_QUERIES_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
MUSIC_QUERIES_OBJECTS = $(addprefix $(MUSIC_QUERIES_OUT)/,queries.o probe.o shutdown.o)
$(MUSIC_QUERIES_OUT)/test_qt_music_library_queries.exe: $(MUSIC_QUERIES_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(MUSIC_QUERIES_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
