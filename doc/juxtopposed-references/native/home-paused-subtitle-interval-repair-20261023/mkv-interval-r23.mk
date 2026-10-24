ifndef MKV_INTERVAL_OUT
$(error Set MKV_INTERVAL_OUT)
endif
ifndef MKV_INTERVAL_SOURCE
$(error Set MKV_INTERVAL_SOURCE)
endif
MKV_INTERVAL_CXX_NAMES = util virtual_segment matroska_segment matroska_segment_parse matroska_segment_seeker demux events Ebml_parser chapters chapter_command chapter_command_dvd chapter_command_script chapter_command_script_common stream_io_callback lzokay mkv
MKV_INTERVAL_ORIGINAL_CXX = $(addprefix demux/mkv/libmkv_plugin_la-,$(addsuffix .lo,$(MKV_INTERVAL_CXX_NAMES)))
MKV_INTERVAL_C_OBJECTS = $(filter-out $(MKV_INTERVAL_ORIGINAL_CXX),$(libmkv_plugin_la_OBJECTS))
ifneq ($(words $(filter $(MKV_INTERVAL_ORIGINAL_CXX),$(libmkv_plugin_la_OBJECTS))),16)
$(error Expected sixteen original C++ objects)
endif
ifneq ($(words $(MKV_INTERVAL_C_OBJECTS)),2)
$(error Expected only the two unchanged C objects)
endif
MKV_INTERVAL_OBJECTS = $(addprefix $(MKV_INTERVAL_OUT)/,$(addsuffix .lo,$(MKV_INTERVAL_CXX_NAMES)))
.PHONY: owned-mkv-interval-r23
owned-mkv-interval-r23: $(MKV_INTERVAL_OUT)/libmkv_plugin.la
$(MKV_INTERVAL_OUT):
	mkdir -p "$@"
$(MKV_INTERVAL_OUT)/%.lo: $(MKV_INTERVAL_SOURCE)/%.cpp | $(MKV_INTERVAL_OUT)
	$(LIBTOOL) --tag=CXX --mode=compile $(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(libmkv_plugin_la_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS) -iquote $(MKV_INTERVAL_SOURCE) -iquote $(srcdir)/demux/mkv -MD -MF $(MKV_INTERVAL_OUT)/$*.d -c -o "$@" "$<"
$(MKV_INTERVAL_OUT)/libmkv_plugin.la: $(MKV_INTERVAL_OBJECTS) $(MKV_INTERVAL_C_OBJECTS) $(libmkv_plugin_la_DEPENDENCIES) $(EXTRA_libmkv_plugin_la_DEPENDENCIES)
	$(libmkv_plugin_la_LINK) $(MKV_INTERVAL_OBJECTS) $(MKV_INTERVAL_C_OBJECTS) $(libmkv_plugin_la_LIBADD) $(LIBS)
