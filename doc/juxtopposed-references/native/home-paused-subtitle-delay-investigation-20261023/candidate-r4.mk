DELAY_REPLACED = input/libvlccore_la-decoder.lo input/libvlccore_la-es_out.lo input/libvlccore_la-input.lo player/libvlccore_la-input.lo player/libvlccore_la-player.lo
DELAY_CORE_OBJECTS = $(filter-out $(DELAY_REPLACED),$(libvlccore_la_OBJECTS))
ifneq ($(words $(filter $(DELAY_REPLACED),$(libvlccore_la_OBJECTS))),5)
$(error Expected exactly five Core objects to replace)
endif
DELAY_PRIVATE_OBJECTS = $(DELAY_CORE_OUT)/decoder.lo $(DELAY_CORE_OUT)/es_out.lo $(DELAY_CORE_OUT)/input.lo $(DELAY_CORE_OUT)/player_input.lo $(DELAY_CORE_OUT)/player.lo
.PHONY: owned-subtitle-delay-core-r4
owned-subtitle-delay-core-r4: $(DELAY_CORE_OUT)/libvlccore.la
$(DELAY_CORE_OUT):
	mkdir -p "$@"
$(DELAY_CORE_OUT)/decoder.lo: $(DELAY_SOURCE)/src/input/decoder.c | $(DELAY_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(DELAY_CORE_OUT)/decoder.d -c -o "$@" "$<"
$(DELAY_CORE_OUT)/es_out.lo: $(DELAY_SOURCE)/src/input/es_out.c | $(DELAY_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(DELAY_CORE_OUT)/es_out.d -c -o "$@" "$<"
$(DELAY_CORE_OUT)/input.lo: $(DELAY_SOURCE)/src/input/input.c | $(DELAY_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/input -I$(srcdir) -MD -MF $(DELAY_CORE_OUT)/input.d -c -o "$@" "$<"
$(DELAY_CORE_OUT)/player_input.lo: $(DELAY_SOURCE)/src/player/input.c | $(DELAY_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/player -I$(srcdir) -MD -MF $(DELAY_CORE_OUT)/player_input.d -c -o "$@" "$<"
$(DELAY_CORE_OUT)/player.lo: $(DELAY_SOURCE)/src/player/player.c | $(DELAY_CORE_OUT)
	$(LIBTOOL) --tag=CC --mode=compile $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libvlccore_la_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -iquote $(srcdir)/player -I$(srcdir) -MD -MF $(DELAY_CORE_OUT)/player.d -c -o "$@" "$<"
$(DELAY_CORE_OUT)/libvlccore.la: $(DELAY_CORE_OBJECTS) $(libvlccore_la_DEPENDENCIES) $(EXTRA_libvlccore_la_DEPENDENCIES) $(DELAY_PRIVATE_OBJECTS)
	$(libvlccore_la_LINK) -rpath $(libdir) $(DELAY_CORE_OBJECTS) $(DELAY_PRIVATE_OBJECTS) $(libvlccore_la_LIBADD) $(LIBS)
