.PHONY: owned-player-modules
owned-player-modules:
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -I$(srcdir)/src/player -c $(QA_SOURCE) -o $(QA_OUT)/player_modules.lo
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -module -avoid-version -no-undefined -rpath $(QA_OUT) $(QA_OUT)/player_modules.lo $(LIBVLCCORE) $(LIBVLC) $(LIBM) -o $(QA_OUT)/libplayer_fixture_plugin.la
