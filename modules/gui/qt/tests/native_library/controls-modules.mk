# Read after the configured modules/Makefile. Never install this provider.
ifndef CONTROLS_OUT
$(error Set CONTROLS_OUT to a fresh private directory)
endif
CONTROLS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir)
.PHONY: controls-input-module
controls-input-module: $(CONTROLS_OUT)/libcontrols_fixture_plugin.la
$(CONTROLS_OUT):
	mkdir -p "$@"
$(CONTROLS_OUT)/controls_fixture.lo: $(top_srcdir)/modules/gui/qt/tests/controls_input_fixture.c | $(CONTROLS_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(CONTROLS_CPPFLAGS) -UMODULE_STRING -DMODULE_STRING=\"controls_fixture\" $(CFLAGS) -c $< -o $@
$(CONTROLS_OUT)/libcontrols_fixture_plugin.la: $(CONTROLS_OUT)/controls_fixture.lo
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -rpath $(abs_builddir)/controls-fixture -o $@ $<
