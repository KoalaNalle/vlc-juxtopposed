# Native Windows QtCore geometry policy checks; no desktop/DPI acceptance.
ifndef PLACEMENT_OUT
$(error Set PLACEMENT_OUT to a fresh owned output directory)
endif
PLACEMENT_CPPFLAGS = -iquote $(PLACEMENT_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(srcdir)/util
.PHONY: detached-placement-native
detached-placement-native: $(PLACEMENT_OUT)/test_qt_detached_window_placement.exe
$(PLACEMENT_OUT):
	mkdir -p "$@"
$(PLACEMENT_OUT)/test_detached_window_placement.moc: $(srcdir)/tests/test_detached_window_placement.cpp | $(PLACEMENT_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(PLACEMENT_OUT)/test_detached_window_placement.o: $(srcdir)/tests/test_detached_window_placement.cpp $(srcdir)/util/detached_window_placement.hpp $(PLACEMENT_OUT)/test_detached_window_placement.moc
	$(CXX) -std=c++17 $(PLACEMENT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(PLACEMENT_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PLACEMENT_OUT)
	$(CXX) -std=c++17 $(PLACEMENT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
PLACEMENT_OBJECTS = $(addprefix $(PLACEMENT_OUT)/,test_detached_window_placement.o winpthreads_shutdown.o)
$(PLACEMENT_OUT)/test_qt_detached_window_placement.exe: $(PLACEMENT_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(PLACEMENT_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
