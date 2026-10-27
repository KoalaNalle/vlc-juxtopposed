# Opt-in production model/service command lifetime and row-range regressions.
ifndef ML_REQUEST_OUT
$(error Set ML_REQUEST_OUT to a fresh owned output directory)
endif
ML_REQUEST_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(ML_REQUEST_OUT)
ML_REQUEST_SOURCES = tests/test_ml_play_request.cpp util/base_model.cpp util/vlctick.cpp \
 medialibrary/mlbasemodel.cpp medialibrary/medialib.cpp medialibrary/mlhomeactivity.cpp \
 medialibrary/mlhomeactivitydata.cpp dialogs/new_playlist_data.cpp medialibrary/mlthreadpool.cpp \
 playlist/playlist_controller.cpp playlist/playlist_item.cpp util/checked_preferences_store.cpp \
 util/winpthreads_shutdown.cpp
ML_REQUEST_HEADERS = util/base_model.hpp util/listcache.hpp util/locallistcacheloader.hpp \
 util/vlctick.hpp medialibrary/mlqmltypes.hpp medialibrary/mlbasemodel.hpp \
 medialibrary/medialib.hpp medialibrary/mlhomeactivity.hpp medialibrary/mlthreadpool.hpp \
 playlist/playlist_controller.hpp playlist/playlist_item.hpp
ML_REQUEST_OBJECTS = $(addprefix $(ML_REQUEST_OUT)/,$(ML_REQUEST_SOURCES:.cpp=.o) $(ML_REQUEST_HEADERS:.hpp=.moc.o)) $(ML_REQUEST_OUT)/probe.o
.PHONY: ml-play-request-native
ml-play-request-native: $(ML_REQUEST_OUT)/test_qt_ml_play_request.exe
$(ML_REQUEST_OUT)/test_ml_play_request.moc: $(srcdir)/tests/test_ml_play_request.cpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(ML_REQUEST_OUT)/tests/test_ml_play_request.o: $(ML_REQUEST_OUT)/test_ml_play_request.moc
$(ML_REQUEST_OUT)/%.o: $(srcdir)/%.cpp
	mkdir -p "$(@D)"
	$(CXX) -std=c++17 $(ML_REQUEST_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ML_REQUEST_OUT)/%.moc.cpp: $(srcdir)/%.hpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(ML_REQUEST_OUT)/%.moc.o: $(ML_REQUEST_OUT)/%.moc.cpp
	$(CXX) -std=c++17 $(ML_REQUEST_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ML_REQUEST_OUT)/probe.o: $(srcdir)/tests/network_stream_probe.c
	mkdir -p "$(@D)"
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c "$<" -o "$@"
$(ML_REQUEST_OUT)/test_qt_ml_play_request.exe: $(ML_REQUEST_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(ML_REQUEST_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
