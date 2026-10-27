# Run from a configured test build directory with a fresh output directory.
ifndef TIMER_CACHE_OUT
$(error Set TIMER_CACHE_OUT to a fresh directory)
endif
.PHONY: owned-player-timer-cache
owned-player-timer-cache: $(TIMER_CACHE_OUT)/test_player_timer_cache.exe
$(TIMER_CACHE_OUT):
	mkdir -p "$@"
$(TIMER_CACHE_OUT)/test_player_timer_cache.exe: $(srcdir)/src/player/timer_cache.c $(top_srcdir)/src/player/timer.c | $(TIMER_CACHE_OUT)
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(filter-out -lssp,$(LDFLAGS)) -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src $< $(shell $(CC) -print-file-name=libssp.a) $(LIBVLCCORE) $(LIBVLC) $(LIBM) $(LIBS)
