QA_SOURCE := $(srcdir)/tests/native_ui/sidebar_pins_journey.cpp
include $(srcdir)/tests/native_ui/network_journey.mk
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
$(QA_OUT)/color-svg-diagnostic.o: $(DIAGNOSTIC_SOURCE) | $(QA_OUT)
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(libqt_plugin_la_CPPFLAGS) -I$(srcdir)/util $(libqt_plugin_la_CXXFLAGS) $(CXXFLAGS) -c $< -o $@
libqt_plugin_la_OBJECTS := $(filter-out %color_svg_image_provider.lo,$(libqt_plugin_la_OBJECTS)) $(QA_OUT)/color-svg-diagnostic.o

$(QA_OUT)/libqt_plugin.la: $(QA_OUT)/color-svg-diagnostic.o
