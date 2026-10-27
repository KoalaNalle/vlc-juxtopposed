# Exact production key routing with real Qt events and native key callbacks.
ifndef ROUTING_OUT
$(error Set ROUTING_OUT to a fresh owned output directory)
endif
ROUTING_SOURCE_ROOT ?= $(top_srcdir)
ROUTING_CPPFLAGS = -iquote $(ROUTING_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: key-routing-native
key-routing-native: $(ROUTING_OUT)/test_qt_shared_modal_hotkeys.exe
$(ROUTING_OUT):
	mkdir -p "$@"
$(ROUTING_OUT)/shared_modal_hotkey_production.inc: $(srcdir)/tests/generate_shared_modal_hotkey_test.py $(ROUTING_SOURCE_ROOT)/modules/gui/qt/maininterface/interface_window_handler.cpp $(ROUTING_SOURCE_ROOT)/modules/gui/qt/maininterface/mainctx.cpp | $(ROUTING_OUT)
	$(PYTHON3) $(srcdir)/tests/generate_shared_modal_hotkey_test.py --repo-root "$(ROUTING_SOURCE_ROOT)" --output "$@"
$(ROUTING_OUT)/test_shared_modal_hotkeys.moc: $(srcdir)/tests/test_shared_modal_hotkeys.cpp | $(ROUTING_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(ROUTING_OUT)/test_shared_modal_hotkeys.o: $(srcdir)/tests/test_shared_modal_hotkeys.cpp $(srcdir)/util/queue_find_key_scope.hpp $(ROUTING_OUT)/test_shared_modal_hotkeys.moc $(ROUTING_OUT)/shared_modal_hotkey_production.inc
	$(CXX) -std=c++17 $(ROUTING_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ROUTING_OUT)/vlchotkeyconverter.o: $(srcdir)/util/vlchotkeyconverter.cpp | $(ROUTING_OUT)
	$(CXX) -std=c++17 $(ROUTING_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ROUTING_OUT)/vlchotkeyconverter.moc.cpp: $(srcdir)/util/vlchotkeyconverter.hpp | $(ROUTING_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(ROUTING_OUT)/vlchotkeyconverter_moc.o: $(ROUTING_OUT)/vlchotkeyconverter.moc.cpp
	$(CXX) -std=c++17 $(ROUTING_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ROUTING_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(ROUTING_OUT)
	$(CXX) -std=c++17 $(ROUTING_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(ROUTING_OUT)/media_player_probe.o: $(srcdir)/tests/media_player_probe.c | $(ROUTING_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c "$<" -o "$@"
ROUTING_OBJECTS = $(addprefix $(ROUTING_OUT)/,test_shared_modal_hotkeys.o vlchotkeyconverter.o vlchotkeyconverter_moc.o media_player_probe.o winpthreads_shutdown.o)
$(ROUTING_OUT)/test_qt_shared_modal_hotkeys.exe: $(ROUTING_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $(ROUTING_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
