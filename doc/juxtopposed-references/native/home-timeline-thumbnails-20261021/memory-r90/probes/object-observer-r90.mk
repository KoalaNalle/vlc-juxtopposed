# Private diagnostic; intentionally does not link QtTest.
.PHONY: owned-object-diagnostic
owned-object-diagnostic: $(QA_OUT)/libqt_plugin.la
$(QA_OUT):
	mkdir -p "$@"
$(QA_OUT)/object-observer.o: $(QA_SOURCE) | $(QA_OUT)
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(libqt_plugin_la_CPPFLAGS) -I$(srcdir) $(libqt_plugin_la_CXXFLAGS) $(CXXFLAGS) -c $< -o $@
$(QA_OUT)/libqt_plugin.la: $(QA_OUT)/object-observer.o $(libqt_plugin_la_OBJECTS) $(libqt_plugin_la_DEPENDENCIES)
	$(libqt_plugin_la_LINK) $(am_libqt_plugin_la_rpath) $(QA_OUT)/object-observer.o $(libqt_plugin_la_OBJECTS) $(libqt_plugin_la_LIBADD) $(LIBS)
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug

libqt_plugin_la_LDFLAGS += -Wl,--wrap=_ZN22QPlatformAccessibility9setActiveEb
