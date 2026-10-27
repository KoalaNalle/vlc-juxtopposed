ifndef FPS_NATIVE_OUT
$(error Set FPS_NATIVE_OUT to a fresh owned directory)
endif
.PHONY: owned-subtitle-fps-native
owned-subtitle-fps-native: $(FPS_NATIVE_OUT)/test_qt_player_subtitle_delay.exe
$(FPS_NATIVE_OUT):
	mkdir -p "$@"
$(FPS_NATIVE_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(FPS_NATIVE_OUT)
	$(CXX) $(QT_QTEST_COMMON_cppflags) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -I$(srcdir) -c "$<" -o "$@"
$(FPS_NATIVE_OUT)/test_qt_player_subtitle_delay.exe: $(test_qt_player_subtitle_delay_OBJECTS) $(test_qt_player_subtitle_delay_DEPENDENCIES) $(FPS_NATIVE_OUT)/shutdown.o
	$(test_qt_player_subtitle_delay_LINK) $(test_qt_player_subtitle_delay_OBJECTS) $(FPS_NATIVE_OUT)/shutdown.o $(test_qt_player_subtitle_delay_LDADD) $(LIBS) -Wl,--wrap=pthread_key_delete -lntdll
test_qt_player_subtitle_delay_LDFLAGS += -Wl,--strip-debug
