# Compile the production record module's start-date calculation directly.
ifndef RECORD_START_OUT
$(error Set RECORD_START_OUT to a fresh directory)
endif
.PHONY: owned-record-start
owned-record-start: $(RECORD_START_OUT)/test_record_start.exe
$(RECORD_START_OUT):
	mkdir -p "$@"
$(RECORD_START_OUT)/test_record_start.exe: $(srcdir)/modules/stream_out/record_start.c $(top_srcdir)/modules/stream_out/record.c | $(RECORD_START_OUT)
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(filter-out -lssp,$(LDFLAGS)) -o $@ $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/src -I$(top_builddir)/src $< $(shell $(CC) -print-file-name=libssp.a) $(LIBVLCCORE) $(LIBVLC) $(LIBM) $(LIBS)
