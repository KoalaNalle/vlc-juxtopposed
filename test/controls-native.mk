# Read after the actual configured test/Makefile. Owned opt-in native fixture.
ifndef CONTROLS_OUT
$(error Set CONTROLS_OUT to a fresh private directory)
endif
CONTROLS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: controls-input-native
controls-input-native: $(CONTROLS_OUT)/test_controls_input.exe
.PHONY: controls-http-native
controls-http-native: $(CONTROLS_OUT)/test_controls_http.exe
$(CONTROLS_OUT):
	mkdir -p "$@"
$(CONTROLS_OUT)/controls_input.o: $(top_srcdir)/test/src/interface/controls.c | $(CONTROLS_OUT)
	$(CC) $(CONTROLS_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(CONTROLS_OUT)/test_controls_input.exe: $(CONTROLS_OUT)/controls_input.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLCCORE) $(LIBVLC)
$(CONTROLS_OUT)/controls_http.o: $(top_srcdir)/test/src/interface/controls_http.c | $(CONTROLS_OUT)
	$(CC) $(CONTROLS_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(CONTROLS_OUT)/test_controls_http.exe: $(CONTROLS_OUT)/controls_http.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLCCORE) $(LIBVLC)
