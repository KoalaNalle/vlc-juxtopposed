ifndef PROCESS_OUT
$(error Set PROCESS_OUT)
endif
PROCESS_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROCESS_OUT)
.PHONY: library-process-native
library-process-native: $(PROCESS_OUT)/test_library_opening_process.exe
$(PROCESS_OUT):
	mkdir -p "$@"
$(PROCESS_OUT)/test_library_opening_process.moc: $(srcdir)/tests/test_library_opening_process.cpp | $(PROCESS_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROCESS_OUT)/process.o: $(srcdir)/tests/test_library_opening_process.cpp $(PROCESS_OUT)/test_library_opening_process.moc
	$(CXX) $(PROCESS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROCESS_OUT)/probe.o: $(srcdir)/tests/library_preferences_probe.c | $(PROCESS_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
$(PROCESS_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROCESS_OUT)
	$(CXX) $(PROCESS_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROCESS_OUT)/test_library_opening_process.exe: $(PROCESS_OUT)/process.o $(PROCESS_OUT)/probe.o $(PROCESS_OUT)/shutdown.o
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $^ $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
