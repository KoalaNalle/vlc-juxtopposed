ifndef MMDEVICE_OUT
$(error Set MMDEVICE_OUT to an owned native output directory)
endif
MMDEVICE_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src -DVLC_DYNAMIC_PLUGIN -DMODULE_STRING=\"mmdevice_stop_test\"
.PHONY: mmdevice-stop-native
mmdevice-stop-native: $(MMDEVICE_OUT)/test_mmdevice_stop.exe
$(MMDEVICE_OUT):
	mkdir -p "$@"
$(MMDEVICE_OUT)/mmdevice_stop.o: $(srcdir)/modules/audio_output/mmdevice_stop.c $(top_srcdir)/modules/audio_output/mmdevice.c $(top_srcdir)/modules/audio_output/mmdevice.h | $(MMDEVICE_OUT)
	$(CC) $(MMDEVICE_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(MMDEVICE_OUT)/test_mmdevice_stop.exe: $(MMDEVICE_OUT)/mmdevice_stop.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE) -lole32 -luuid
