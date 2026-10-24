ifndef ADDONS_OUT
$(error Set ADDONS_OUT to a fresh owned native output directory)
endif
ADDONS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: addons-native
addons-native: $(ADDONS_OUT)/test_addons.exe
$(ADDONS_OUT):
	mkdir -p "$@"
$(ADDONS_OUT)/addons.o: $(srcdir)/src/misc/addons.c | $(ADDONS_OUT)
	$(CC) $(ADDONS_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(ADDONS_OUT)/test_addons.exe: $(ADDONS_OUT)/addons.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE)
