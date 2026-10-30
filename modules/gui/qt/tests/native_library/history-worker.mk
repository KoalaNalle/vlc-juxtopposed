# Opt-in actual production QObject/QThread history-edit lifecycle on real ML.
# Read after the configured Qt Makefile; one writer, fresh ignored output.
ifndef HISTORY_WORKER_OUT
$(error Set HISTORY_WORKER_OUT to a fresh owned output directory)
endif
HISTORY_WORKER_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(HISTORY_WORKER_OUT)
HISTORY_WORKER_SOURCES = tests/test_history_worker.cpp util/base_model.cpp util/vlctick.cpp \
 medialibrary/mlbasemodel.cpp medialibrary/medialib.cpp medialibrary/mlhomeactivity.cpp \
 medialibrary/mlhomeactivitydata.cpp dialogs/new_playlist_data.cpp medialibrary/mlthreadpool.cpp \
 medialibrary/mlmediamodel.cpp medialibrary/mlvideomodel.cpp medialibrary/mlrecentvideomodel.cpp \
 medialibrary/mlvideo.cpp medialibrary/mlaudio.cpp medialibrary/mlhelper.cpp \
 playlist/playlist_controller.cpp playlist/playlist_item.cpp util/checked_preferences_store.cpp \
 util/winpthreads_shutdown.cpp
HISTORY_WORKER_HEADERS = util/base_model.hpp util/listcache.hpp util/locallistcacheloader.hpp \
 util/vlctick.hpp medialibrary/mlqmltypes.hpp medialibrary/mlbasemodel.hpp \
 medialibrary/medialib.hpp medialibrary/mlhomeactivity.hpp medialibrary/mlthreadpool.hpp \
 medialibrary/mlmediamodel.hpp medialibrary/mlvideomodel.hpp medialibrary/mlrecentvideomodel.hpp \
 medialibrary/mlvideo.hpp playlist/playlist_controller.hpp playlist/playlist_item.hpp
HISTORY_WORKER_OBJECTS = $(addprefix $(HISTORY_WORKER_OUT)/,$(HISTORY_WORKER_SOURCES:.cpp=.o) $(HISTORY_WORKER_HEADERS:.hpp=.moc.o)) $(HISTORY_WORKER_OUT)/probe.o
HISTORY_WORKER_MOC_CPP = $(addprefix $(HISTORY_WORKER_OUT)/,$(HISTORY_WORKER_HEADERS:.hpp=.moc.cpp))
HISTORY_WORKER_PREDEPS = $(addprefix $(HISTORY_WORKER_OUT)/predeps/,$(HISTORY_WORKER_SOURCES:.cpp=.d) $(HISTORY_WORKER_HEADERS:.hpp=.moc.d)) $(HISTORY_WORKER_OUT)/predeps/probe.d
.SECONDARY: $(HISTORY_WORKER_MOC_CPP)
.PHONY: history-worker-native history-worker-generated history-worker-dependencies history-worker-config
history-worker-native: $(HISTORY_WORKER_OUT)/test_qt_history_worker.exe
history-worker-generated: $(HISTORY_WORKER_OUT)/test_history_worker.moc $(HISTORY_WORKER_MOC_CPP)
history-worker-dependencies: $(HISTORY_WORKER_PREDEPS)
# GNU make writes the fully expanded configuration without shell re-quoting.
# The guarded producer creates HISTORY_WORKER_OUT before requesting this target.
history-worker-config:
	$(file >$(HISTORY_WORKER_OUT)/config-cxx.txt,$(CXX))
	$(file >$(HISTORY_WORKER_OUT)/config-cc.txt,$(CC))
	$(file >$(HISTORY_WORKER_OUT)/config-moc.txt,$(MOC))
	$(file >$(HISTORY_WORKER_OUT)/config-cxxflags.txt,$(HISTORY_WORKER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS))
	$(file >$(HISTORY_WORKER_OUT)/config-cflags.txt,$(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(CFLAGS))
	$(file >$(HISTORY_WORKER_OUT)/config-link.txt,$(QT_QTEST_COMMON_ldflags) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll)
	@:
$(HISTORY_WORKER_OUT)/test_history_worker.moc: $(srcdir)/tests/test_history_worker.cpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(HISTORY_WORKER_OUT)/tests/test_history_worker.o: $(HISTORY_WORKER_OUT)/test_history_worker.moc
$(HISTORY_WORKER_OUT)/%.o: $(srcdir)/%.cpp
	mkdir -p "$(@D)"
	$(CXX) -std=c++17 $(HISTORY_WORKER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -MD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(HISTORY_WORKER_OUT)/%.moc.cpp: $(srcdir)/%.hpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(HISTORY_WORKER_OUT)/%.moc.o: $(HISTORY_WORKER_OUT)/%.moc.cpp
	$(CXX) -std=c++17 $(HISTORY_WORKER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -MD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(HISTORY_WORKER_OUT)/probe.o: $(srcdir)/tests/network_stream_probe.c
	mkdir -p "$(@D)"
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -MD -MP -MF "$@.d" -MT "$@" -c "$<" -o "$@"
$(HISTORY_WORKER_OUT)/predeps/%.d: $(srcdir)/%.cpp history-worker-generated
	mkdir -p "$(@D)"
	$(CXX) -std=c++17 $(HISTORY_WORKER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -M -MF "$@" -MT "$<" "$<"
$(HISTORY_WORKER_OUT)/predeps/%.moc.d: $(HISTORY_WORKER_OUT)/%.moc.cpp
	mkdir -p "$(@D)"
	$(CXX) -std=c++17 $(HISTORY_WORKER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -M -MF "$@" -MT "$<" "$<"
$(HISTORY_WORKER_OUT)/predeps/probe.d: $(srcdir)/tests/network_stream_probe.c
	mkdir -p "$(@D)"
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -M -MF "$@" -MT "$<" "$<"
$(HISTORY_WORKER_OUT)/test_qt_history_worker.exe: $(HISTORY_WORKER_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(HISTORY_WORKER_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
