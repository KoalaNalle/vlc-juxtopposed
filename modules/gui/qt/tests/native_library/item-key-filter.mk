# Real QApplication + production Qt Quick input filter, no native VLC window.
ifndef FILTER_OUT
$(error Set FILTER_OUT to a fresh owned output directory)
endif
FILTER_SOURCE ?= $(srcdir)/util/item_key_event_filter.cpp
FILTER_CPPFLAGS = -iquote $(FILTER_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(srcdir)/util
.PHONY: item-key-filter-native
item-key-filter-native: $(FILTER_OUT)/test_qt_item_key_event_filter.exe
$(FILTER_OUT):
	mkdir -p "$@"
$(FILTER_OUT)/test_item_key_event_filter.moc: $(srcdir)/tests/test_item_key_event_filter.cpp | $(FILTER_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(FILTER_OUT)/test_item_key_event_filter.o: $(srcdir)/tests/test_item_key_event_filter.cpp $(FILTER_OUT)/test_item_key_event_filter.moc
	$(CXX) -std=c++17 $(FILTER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(FILTER_OUT)/item_key_event_filter.o: $(FILTER_SOURCE) $(srcdir)/util/item_key_event_filter.hpp | $(FILTER_OUT)
	$(CXX) -std=c++17 $(FILTER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(FILTER_OUT)/item_key_event_filter.moc.cpp: $(srcdir)/util/item_key_event_filter.hpp | $(FILTER_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(FILTER_OUT)/item_key_event_filter_moc.o: $(FILTER_OUT)/item_key_event_filter.moc.cpp
	$(CXX) -std=c++17 $(FILTER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(FILTER_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(FILTER_OUT)
	$(CXX) -std=c++17 $(FILTER_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
FILTER_OBJECTS = $(addprefix $(FILTER_OUT)/,test_item_key_event_filter.o item_key_event_filter.o item_key_event_filter_moc.o winpthreads_shutdown.o)
$(FILTER_OUT)/test_qt_item_key_event_filter.exe: $(FILTER_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(FILTER_OBJECTS) -lqoffscreen $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
