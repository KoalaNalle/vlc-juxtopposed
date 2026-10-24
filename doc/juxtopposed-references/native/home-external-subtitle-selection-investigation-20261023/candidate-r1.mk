EXTERNAL_CORE_OBJECTS = $(filter-out input/libvlccore_la-input.lo input/libvlccore_la-es_out.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-input.lo input/libvlccore_la-es_out.lo,$(libvlccore_la_OBJECTS))),2)
$(error Expected exactly two Core objects to replace)
endif
.PHONY: owned-external-subtitle-core-r1
owned-external-subtitle-core-r1: $(EXTERNAL_CORE_OUT)/libvlccore.la
$(EXTERNAL_CORE_OUT):
	mkdir -p "$@"
$(EXTERNAL_CORE_OUT)/input.lo: $(EXTERNAL_SOURCE)/input.c $(EXTERNAL_SOURCE)/es_out.h | $(EXTERNAL_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(EXTERNAL_CORE_OUT)/input.d -c -o "$@" "$<"
$(EXTERNAL_CORE_OUT)/es_out.lo: $(EXTERNAL_SOURCE)/es_out.c $(EXTERNAL_SOURCE)/es_out.h | $(EXTERNAL_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(EXTERNAL_CORE_OUT)/es_out.d -c -o "$@" "$<"
$(EXTERNAL_CORE_OUT)/libvlccore.la: $(EXTERNAL_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(EXTERNAL_CORE_OUT)/input.lo $(EXTERNAL_CORE_OUT)/es_out.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(EXTERNAL_CORE_OBJECTS) $(EXTERNAL_CORE_OUT)/input.lo $(EXTERNAL_CORE_OUT)/es_out.lo $(libvlccore_la_LIBADD) $(LIBS)
