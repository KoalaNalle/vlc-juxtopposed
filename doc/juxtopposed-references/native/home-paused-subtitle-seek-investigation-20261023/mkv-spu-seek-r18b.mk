ifndef MKV_PRIVATE_OUT
$(error Set MKV_PRIVATE_OUT)
endif
ifndef MKV_PRIVATE_SOURCE
$(error Set MKV_PRIVATE_SOURCE)
endif
MKV_PRIVATE_OBJECTS = $(filter-out demux/mkv/libmkv_plugin_la-matroska_segment.lo,$(libmkv_plugin_la_OBJECTS))
ifneq ($(words $(filter demux/mkv/libmkv_plugin_la-matroska_segment.lo,$(libmkv_plugin_la_OBJECTS))),1)
$(error Expected exactly one Matroska segment object to replace)
endif
.PHONY: owned-mkv-spu-seek-r18b
owned-mkv-spu-seek-r18b: $(MKV_PRIVATE_OUT)/libmkv_plugin.la
$(MKV_PRIVATE_OUT):
	mkdir -p "$@"
$(MKV_PRIVATE_OUT)/matroska_segment.lo: $(MKV_PRIVATE_SOURCE) | $(MKV_PRIVATE_OUT)
	$(LIBTOOL) --tag=CXX --mode=compile $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libmkv_plugin_la_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -iquote $(srcdir)/demux/mkv -MD -MF $(MKV_PRIVATE_OUT)/matroska_segment.d -c -o "$@" "$<"
$(MKV_PRIVATE_OUT)/libmkv_plugin.la: $(MKV_PRIVATE_OBJECTS) $(libmkv_plugin_la_DEPENDENCIES) $(EXTRA_libmkv_plugin_la_DEPENDENCIES) $(MKV_PRIVATE_OUT)/matroska_segment.lo
	$(libmkv_plugin_la_LINK) $(MKV_PRIVATE_OBJECTS) $(MKV_PRIVATE_OUT)/matroska_segment.lo $(libmkv_plugin_la_LIBADD) $(LIBS)
