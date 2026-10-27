# Read after the real configured modules/gui/qt Makefile. Private test build only.
PENDING ?= $(abs_srcdir)
SHARED_STORE ?= $(abs_srcdir)
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh private output directory)
endif
LIBRARY_CPPFLAGS = -I$(PENDING) -I$(SHARED_STORE) -I$(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(srcdir)/medialibrary -I$(top_srcdir) -I$(top_srcdir)/src
LIBRARY_CFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: library-preferences-native
library-preferences-native: $(PROPOSAL_OUT)/test_qt_library_preferences.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/library_preferences_model.moc.cpp: $(PENDING)/maininterface/library_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -I$(PENDING) -o $@ $<
$(PROPOSAL_OUT)/test_library_preferences.moc: $(PENDING)/tests/test_library_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -I$(PENDING) -o $@ $<
$(PROPOSAL_OUT)/mlqmltypes.moc.cpp: $(srcdir)/medialibrary/mlqmltypes.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/library_preferences_model.o: $(PENDING)/maininterface/library_preferences_model.cpp $(PENDING)/maininterface/library_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/test_library_preferences.o: $(PENDING)/tests/test_library_preferences.cpp $(PROPOSAL_OUT)/test_library_preferences.moc | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_model_moc.o: $(PROPOSAL_OUT)/library_preferences_model.moc.cpp
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(SHARED_STORE)/util/checked_preferences_store.cpp $(SHARED_STORE)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/library_preferences_probe.o: $(PENDING)/tests/library_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(LIBRARY_CFLAGS) $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/mlhomeactivitydata.o: $(srcdir)/medialibrary/mlhomeactivitydata.cpp | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/new_playlist_data.o: $(srcdir)/dialogs/new_playlist_data.cpp | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/mlqmltypes_moc.o: $(PROPOSAL_OUT)/mlqmltypes.moc.cpp
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) $(LIBRARY_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
LIBRARY_OBJECTS = $(PROPOSAL_OUT)/test_library_preferences.o $(PROPOSAL_OUT)/library_preferences_model.o $(PROPOSAL_OUT)/library_preferences_model_moc.o $(PROPOSAL_OUT)/checked_preferences_store.o $(PROPOSAL_OUT)/library_preferences_probe.o $(PROPOSAL_OUT)/mlhomeactivitydata.o $(PROPOSAL_OUT)/new_playlist_data.o $(PROPOSAL_OUT)/mlqmltypes_moc.o $(PROPOSAL_OUT)/winpthreads_shutdown.o
$(PROPOSAL_OUT)/test_qt_library_preferences.exe: $(LIBRARY_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(LIBRARY_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug

LIBRARY_OBJECTS += $(PROPOSAL_OUT)/library_policies.o
$(PROPOSAL_OUT)/test_qt_library_preferences.exe: $(PROPOSAL_OUT)/library_policies.o
$(PROPOSAL_OUT)/library_policies.o: $(PENDING)/tests/library_policies.c | $(PROPOSAL_OUT)
	$(CC) $(LIBRARY_CFLAGS) $(CFLAGS) -c $< -o $@
