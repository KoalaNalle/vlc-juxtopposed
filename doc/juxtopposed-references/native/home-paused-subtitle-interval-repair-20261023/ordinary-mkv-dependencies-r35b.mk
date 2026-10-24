# Keep the configured ordinary compilation/link rules and flags, adding only
# dependency output for provenance because this build has dummy .Plo files.
demux/mkv/libmkv_plugin_la-%.lo: CXXFLAGS += -MD -MF $(@:.lo=.ordinary-r35b.d)
