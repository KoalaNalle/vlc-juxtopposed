ifndef CANDIDATE_CORE_OUT
$(error Set CANDIDATE_CORE_OUT)
endif
ifndef CANDIDATE_INPUT_SOURCE
$(error Set CANDIDATE_INPUT_SOURCE)
endif
CANDIDATE_CORE_OBJECTS = $(filter-out player/libvlccore_la-input.lo,$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter player/libvlccore_la-input.lo,$(libvlccore_la_OBJECTS))),1)
$(error Expected exactly one original decoder object to replace)
endif
.PHONY: owned-subtitle-refresh-core-r4
owned-subtitle-refresh-core-r4: $(CANDIDATE_CORE_OUT)/libvlccore.la
$(CANDIDATE_CORE_OUT):
	mkdir -p "$@"
$(CANDIDATE_CORE_OUT)/player-input.lo: $(CANDIDATE_INPUT_SOURCE) | $(CANDIDATE_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/player -I$(srcdir) -MD -MF $(CANDIDATE_CORE_OUT)/player-input.d -c -o "$@" "$<"
$(CANDIDATE_CORE_OUT)/libvlccore.la: $(CANDIDATE_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(CANDIDATE_CORE_OUT)/player-input.lo
	$(libvlccore_la_LINK) -rpath $(libdir) $(CANDIDATE_CORE_OBJECTS) $(CANDIDATE_CORE_OUT)/player-input.lo $(libvlccore_la_LIBADD) $(LIBS)
