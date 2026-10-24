ifndef PRIORITY_OUT
$(error Set PRIORITY_OUT to an owned native output directory)
endif
PRIORITY_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: priority-startup-native
priority-startup-native: $(PRIORITY_OUT)/test_priority_startup.exe
$(PRIORITY_OUT):
	mkdir -p "$@"
$(PRIORITY_OUT)/priority_startup.o: $(srcdir)/src/misc/priority_startup.c | $(PRIORITY_OUT)
	$(CC) $(PRIORITY_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(PRIORITY_OUT)/test_priority_startup.exe: $(PRIORITY_OUT)/priority_startup.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE)
