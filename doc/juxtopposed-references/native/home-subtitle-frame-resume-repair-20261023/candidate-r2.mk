RESUME_CORE_OBJECTS = $(filter-out input/libvlccore_la-decoder.lo input/libvlccore_la-es_out.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-decoder.lo input/libvlccore_la-es_out.lo,$(libvlccore_la_OBJECTS))),2)
$(error Expected exactly two Core objects to replace)
endif
.PHONY: owned-frame-resume-core-r2
owned-frame-resume-core-r2: $(RESUME_CORE_OUT)/libvlccore.la
$(RESUME_CORE_OUT):
	mkdir -p "$@"
$(RESUME_CORE_OUT)/decoder.lo: $(RESUME_SOURCE)/decoder.c | $(RESUME_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(RESUME_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(RESUME_CORE_OUT)/es_out.lo: $(RESUME_SOURCE)/es_out.c | $(RESUME_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(RESUME_CORE_OUT)/es_out.d -c -o "$@" "$<"
$(RESUME_CORE_OUT)/libvlccore.la: $(RESUME_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(RESUME_CORE_OUT)/decoder.lo $(RESUME_CORE_OUT)/es_out.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(RESUME_CORE_OBJECTS) $(RESUME_CORE_OUT)/decoder.lo $(RESUME_CORE_OUT)/es_out.lo $(libvlccore_la_LIBADD) $(LIBS)
