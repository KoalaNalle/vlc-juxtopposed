# Read AFTER the actual configured modules/Makefile (not Qt's Makefile).
# Build only inside an owned private test directory.
PENDING ?= $(top_srcdir)/modules/gui/qt
ifndef LIBRARY_MODULE_OUT
$(error Set LIBRARY_MODULE_OUT to a fresh private output directory)
endif
LIBRARY_MODULE_INCLUDES = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir)
# Keep the actual configured target/plugin runtime; broaden export regex ONLY
# for the private WinMsg diagnostic symbol. Never install these diagnostics.
LIBRARY_MODULE_LDFLAGS = $(filter-out -export-symbols-regex ^vlc_entry,$(AM_LDFLAGS))
.PHONY: library-private-fixture-modules
library-private-fixture-modules: $(LIBRARY_MODULE_OUT)/liblibrary_fixture_fetcher_plugin.la $(LIBRARY_MODULE_OUT)/liblibrary_winmsg_probe_plugin.la
$(LIBRARY_MODULE_OUT):
	mkdir -p "$@"
$(LIBRARY_MODULE_OUT)/library_fixture_fetcher_plugin.lo: $(PENDING)/tests/library_fixture_fetcher.c | $(LIBRARY_MODULE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(LIBRARY_MODULE_INCLUDES) -UMODULE_STRING -DMODULE_STRING=\"library_fixture_fetcher\" $(CFLAGS) -c $< -o $@
$(LIBRARY_MODULE_OUT)/liblibrary_fixture_fetcher_plugin.la: $(LIBRARY_MODULE_OUT)/library_fixture_fetcher_plugin.lo
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(LIBRARY_MODULE_LDFLAGS) -export-symbols-regex '^vlc_entry' -rpath $(abs_builddir)/library-fixture -o $@ $<
# WINMSG_SOURCE is the test-only source prepared from the current receiver.
$(LIBRARY_MODULE_OUT)/library_winmsg_probe_plugin.lo: $(WINMSG_SOURCE) | $(LIBRARY_MODULE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(LIBRARY_MODULE_INCLUDES) -UMODULE_STRING -DMODULE_STRING=\"win_msg\" $(CFLAGS) -c $< -o $@
$(LIBRARY_MODULE_OUT)/liblibrary_winmsg_probe_plugin.la: $(LIBRARY_MODULE_OUT)/library_winmsg_probe_plugin.lo
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(LIBRARY_MODULE_LDFLAGS) -export-symbols-regex '^(vlc_entry|vlc_test_library_copydata)' -rpath $(abs_builddir)/library-fixture -o $@ $< -luser32
