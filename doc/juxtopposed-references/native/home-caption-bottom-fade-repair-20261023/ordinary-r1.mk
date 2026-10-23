# Ordinary plugin: no private journey translation unit or QtTest linkage.
.PHONY: owned-ordinary-caption-layout-r1
owned-ordinary-caption-layout-r1: $(QA_OUT)/libqt_plugin.la
$(QA_OUT):
	mkdir -p "$@"
$(QA_OUT)/libqt_plugin.la: $(libqt_plugin_la_OBJECTS) $(libqt_plugin_la_DEPENDENCIES) | $(QA_OUT)
	$(libqt_plugin_la_LINK) $(am_libqt_plugin_la_rpath) $(libqt_plugin_la_OBJECTS) $(libqt_plugin_la_LIBADD) $(LIBS)
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
