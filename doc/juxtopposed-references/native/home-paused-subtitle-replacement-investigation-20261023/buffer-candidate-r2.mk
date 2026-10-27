ifndef CANDIDATE_CORE_OUT
$(error Set CANDIDATE_CORE_OUT)
endif
ifndef CANDIDATE_DECODER_SOURCE
$(error Set CANDIDATE_DECODER_SOURCE)
endif
CANDIDATE_CORE_OBJECTS = $(filter-out input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))),1)
$(error Expected exactly one original decoder object to replace)
endif
.PHONY: owned-subtitle-buffer-core-r2
owned-subtitle-buffer-core-r2: $(CANDIDATE_CORE_OUT)/libvlccore.la
$(CANDIDATE_CORE_OUT):
	mkdir -p "$@"
$(CANDIDATE_CORE_OUT)/decoder.lo: $(CANDIDATE_DECODER_SOURCE) | $(CANDIDATE_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(CANDIDATE_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(CANDIDATE_CORE_OUT)/libvlccore.la: $(CANDIDATE_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(CANDIDATE_CORE_OUT)/decoder.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(CANDIDATE_CORE_OBJECTS) $(CANDIDATE_CORE_OUT)/decoder.lo $(libvlccore_la_LIBADD) $(LIBS)
