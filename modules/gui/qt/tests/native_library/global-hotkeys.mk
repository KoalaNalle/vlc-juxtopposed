# Actual Windows global-hotkey module tests, loaded from the staged runtime.
ifndef HOTKEYS_OUT
$(error Set HOTKEYS_OUT to a fresh private output directory)
endif
HOTKEYS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(HOTKEYS_OUT)
HOTKEYS_OBJECTS = $(HOTKEYS_OUT)/test_global_hotkey_registration.o $(HOTKEYS_OUT)/library_preferences_probe.o $(HOTKEYS_OUT)/winpthreads_shutdown.o
.PHONY: global-hotkeys-native
global-hotkeys-native: $(HOTKEYS_OUT)/test_qt_global_hotkey_registration.exe
$(HOTKEYS_OUT):
	mkdir -p "$@"
$(HOTKEYS_OUT)/test_global_hotkey_registration.moc: $(srcdir)/tests/test_global_hotkey_registration.cpp | $(HOTKEYS_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(HOTKEYS_OUT)/test_global_hotkey_registration.o: $(srcdir)/tests/test_global_hotkey_registration.cpp $(HOTKEYS_OUT)/test_global_hotkey_registration.moc
	$(CXX) $(HOTKEYS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(HOTKEYS_OUT)/library_preferences_probe.o: $(srcdir)/tests/library_preferences_probe.c | $(HOTKEYS_OUT)
	$(CC) $(HOTKEYS_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(HOTKEYS_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(HOTKEYS_OUT)
	$(CXX) $(HOTKEYS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(HOTKEYS_OUT)/test_qt_global_hotkey_registration.exe: $(HOTKEYS_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(HOTKEYS_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -luser32 -Wl,--strip-debug
