# Opt-in private GUI test. Never add its TU to libqt_plugin sources.
QA_SOURCE ?= $(srcdir)/tests/native_ui/network_journey.cpp
ifndef QA_OUT
$(error Set QA_OUT to an owned private output directory)
endif
.PHONY: owned-native-ui
owned-native-ui: $(QA_OUT)/libqt_plugin.la
$(QA_OUT):
	mkdir -p "$@"
$(QA_OUT)/native-ui-journey.o: $(QA_SOURCE) | $(QA_OUT)
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(libqt_plugin_la_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) $(libqt_plugin_la_CXXFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(QA_OUT)/libqt_plugin.la: $(QA_OUT)/native-ui-journey.o $(libqt_plugin_la_OBJECTS) $(libqt_plugin_la_DEPENDENCIES)
	$(libqt_plugin_la_LINK) $(am_libqt_plugin_la_rpath) $(QA_OUT)/native-ui-journey.o $(libqt_plugin_la_OBJECTS) $(QT_QTEST_LIBS) $(libqt_plugin_la_LIBADD) $(LIBS)
