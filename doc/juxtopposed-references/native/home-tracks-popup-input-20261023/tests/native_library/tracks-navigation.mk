ifndef TRACKS_NAV_OUT
$(error Set TRACKS_NAV_OUT)
endif
TRACKS_NAV_FLAGS = -I$(TRACKS_NAV_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir)
.PHONY: native-tracks-navigation
native-tracks-navigation: $(TRACKS_NAV_OUT)/test_tracks_cancel_navigation.exe
$(TRACKS_NAV_OUT):
	mkdir -p "$@"
$(TRACKS_NAV_OUT)/test_tracks_cancel_navigation.moc: $(srcdir)/tests/test_tracks_cancel_navigation.cpp | $(TRACKS_NAV_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(TRACKS_NAV_OUT)/navigation.moc.cpp: $(srcdir)/widgets/native/navigation_attached.hpp | $(TRACKS_NAV_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o "$@" "$<"
$(TRACKS_NAV_OUT)/test.o: $(srcdir)/tests/test_tracks_cancel_navigation.cpp $(TRACKS_NAV_OUT)/test_tracks_cancel_navigation.moc
	$(CXX) -std=c++17 $(TRACKS_NAV_FLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(TRACKS_NAV_OUT)/navigation.o: $(srcdir)/widgets/native/navigation_attached.cpp | $(TRACKS_NAV_OUT)
	$(CXX) -std=c++17 $(TRACKS_NAV_FLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(TRACKS_NAV_OUT)/navigation.moc.o: $(TRACKS_NAV_OUT)/navigation.moc.cpp
	$(CXX) -std=c++17 $(TRACKS_NAV_FLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(TRACKS_NAV_OUT)/shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(TRACKS_NAV_OUT)
	$(CXX) -std=c++17 $(TRACKS_NAV_FLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c "$<" -o "$@"
$(TRACKS_NAV_OUT)/test_tracks_cancel_navigation.exe: $(TRACKS_NAV_OUT)/test.o $(TRACKS_NAV_OUT)/navigation.o $(TRACKS_NAV_OUT)/navigation.moc.o $(TRACKS_NAV_OUT)/shutdown.o
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o "$@" $^ -lqoffscreen $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
