FEEDBACK_CORE_OBJECTS = $(filter-out input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter input/libvlccore_la-decoder.lo,$(libvlccore_la_OBJECTS))),1)
$(error Expected exactly one Core decoder object to replace)
endif
.PHONY: owned-frame-feedback-core-r2
owned-frame-feedback-core-r2: $(FEEDBACK_CORE_OUT)/libvlccore.la
$(FEEDBACK_CORE_OUT):
	mkdir -p "$@"
$(FEEDBACK_CORE_OUT)/decoder.lo: $(FEEDBACK_SOURCE)/decoder.c | $(FEEDBACK_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(FEEDBACK_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(FEEDBACK_CORE_OUT)/libvlccore.la: $(FEEDBACK_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(FEEDBACK_CORE_OUT)/decoder.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(FEEDBACK_CORE_OBJECTS) $(FEEDBACK_CORE_OUT)/decoder.lo $(libvlccore_la_LIBADD) $(LIBS)
