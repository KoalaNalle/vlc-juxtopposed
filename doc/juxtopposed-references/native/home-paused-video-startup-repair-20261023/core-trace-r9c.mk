ifndef TRACE_CORE_OUT
$(error Set TRACE_CORE_OUT)
endif
ifndef TRACE_DECODER_SOURCE
$(error Set TRACE_DECODER_SOURCE)
endif
TRACE_CORE_OBJECTS = $(filter-out input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))),1)
$(error Expected exactly one original decoder object to replace)
endif
.PHONY: owned-decoder-trace-core-r9c
owned-decoder-trace-core-r9c: $(TRACE_CORE_OUT)/libvlccore.la
$(TRACE_CORE_OUT):
	mkdir -p "$@"
$(TRACE_CORE_OUT)/decoder.lo: $(TRACE_DECODER_SOURCE) | $(TRACE_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(TRACE_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(TRACE_CORE_OUT)/libvlccore.la: $(TRACE_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(TRACE_CORE_OUT)/decoder.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(TRACE_CORE_OBJECTS) $(TRACE_CORE_OUT)/decoder.lo $(libvlccore_la_LIBADD) $(LIBS)
