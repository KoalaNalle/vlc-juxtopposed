# Exact pending-seek producer with supplied native fields/recording callback.
ifndef SEEK_TIMER_OUT
$(error Set SEEK_TIMER_OUT)
endif
SEEK_TIMER_SOURCE ?= $(top_srcdir)/src/player/timer.c
SEEK_TIMER_CPPFLAGS = -iquote $(SEEK_TIMER_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: native-seek-timer-point
native-seek-timer-point: $(SEEK_TIMER_OUT)/test_player_seek_timer_point.exe
$(SEEK_TIMER_OUT):
	mkdir -p "$@"
$(SEEK_TIMER_OUT)/player_seek_timer_production.inc: $(srcdir)/tests/generate_player_seek_timer_test.py $(SEEK_TIMER_SOURCE) | $(SEEK_TIMER_OUT)
	$(PYTHON3) $(srcdir)/tests/generate_player_seek_timer_test.py --source "$(SEEK_TIMER_SOURCE)" --output "$@"
$(SEEK_TIMER_OUT)/test_player_seek_timer_point.moc: $(srcdir)/tests/test_player_seek_timer_point.cpp | $(SEEK_TIMER_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(SEEK_TIMER_OUT)/test.o: $(srcdir)/tests/test_player_seek_timer_point.cpp $(SEEK_TIMER_OUT)/test_player_seek_timer_point.moc
	$(CXX) -std=c++17 $(SEEK_TIMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(SEEK_TIMER_OUT)/probe.o: $(srcdir)/tests/player_seek_timer_probe.c $(SEEK_TIMER_OUT)/player_seek_timer_production.inc
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -iquote $(SEEK_TIMER_OUT) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c "$<" -o "$@"
$(SEEK_TIMER_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(SEEK_TIMER_OUT)
	$(CXX) -std=c++17 $(SEEK_TIMER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(SEEK_TIMER_OUT)/test_player_seek_timer_point.exe: $(SEEK_TIMER_OUT)/test.o $(SEEK_TIMER_OUT)/probe.o $(SEEK_TIMER_OUT)/shutdown.o
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $^ $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
