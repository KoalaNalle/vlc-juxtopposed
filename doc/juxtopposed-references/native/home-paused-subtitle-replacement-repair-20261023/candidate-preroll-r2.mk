REPAIR_CORE_OBJECTS = $(filter-out input/libvlccore_la-decoder.lo input/libvlccore_la-es_out.lo player/libvlccore_la-input.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-decoder.lo input/libvlccore_la-es_out.lo player/libvlccore_la-input.lo,$(libvlccore_la_OBJECTS))),3)
$(error Expected exactly three Core objects to replace)
endif
.PHONY: owned-subtitle-preroll-core-r2
owned-subtitle-preroll-core-r2: $(REPAIR_CORE_OUT)/libvlccore.la
$(REPAIR_CORE_OUT):
	mkdir -p "$@"
$(REPAIR_CORE_OUT)/decoder.lo: $(REPAIR_SOURCE)/input/decoder.c $(REPAIR_SOURCE)/input/decoder.h | $(REPAIR_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(REPAIR_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(REPAIR_CORE_OUT)/es_out.lo: $(REPAIR_SOURCE)/input/es_out.c $(REPAIR_SOURCE)/input/decoder.h | $(REPAIR_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(REPAIR_CORE_OUT)/es_out.d -c -o "$@" "$<"
$(REPAIR_CORE_OUT)/player-input.lo: $(REPAIR_SOURCE)/player/input.c | $(REPAIR_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/player -I$(srcdir) -MD -MF $(REPAIR_CORE_OUT)/player-input.d -c -o "$@" "$<"
$(REPAIR_CORE_OUT)/libvlccore.la: $(REPAIR_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(REPAIR_CORE_OUT)/decoder.lo $(REPAIR_CORE_OUT)/es_out.lo $(REPAIR_CORE_OUT)/player-input.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(REPAIR_CORE_OBJECTS) $(REPAIR_CORE_OUT)/decoder.lo $(REPAIR_CORE_OUT)/es_out.lo $(REPAIR_CORE_OUT)/player-input.lo $(libvlccore_la_LIBADD) $(LIBS)
