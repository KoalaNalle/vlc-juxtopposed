# Read after the configured Qt Makefile. Real owned widget integration tests.
ifndef CONTROLS_MODEL_OUT
$(error Set CONTROLS_MODEL_OUT to the freshly built Controls controller objects)
endif
ifndef DIALOGS_OUT
$(error Set DIALOGS_OUT to a fresh private directory)
endif
CONTROLS_DIALOG_NAMES = tests/test_controls_dialogs dialogs/preferences/controls_shortcuts dialogs/preferences/preferences_widgets widgets/native/searchlineedit widgets/native/customwidgets widgets/native/animators util/vlchotkeyconverter util/imagehelper util/modal_input_guard
CONTROLS_DIALOG_MOC_NAMES = dialogs/preferences/preferences_widgets widgets/native/searchlineedit widgets/native/customwidgets widgets/native/animators util/vlchotkeyconverter
CONTROLS_DIALOG_OBJECTS = $(addprefix $(DIALOGS_OUT)/,$(addsuffix .o,$(CONTROLS_DIALOG_NAMES))) $(addprefix $(DIALOGS_OUT)/,$(addsuffix .moc.o,$(CONTROLS_DIALOG_MOC_NAMES)))
CONTROLS_DIALOG_MODEL_OBJECTS = $(addprefix $(CONTROLS_MODEL_OUT)/,controls_preferences_model.o controls_preferences_model_moc.o checked_preferences_store.o library_preferences_probe.o winpthreads_shutdown.o)
CONTROLS_DIALOG_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src -I$(DIALOGS_OUT)
.SECONDARY: $(addprefix $(DIALOGS_OUT)/,$(addsuffix .moc.cpp,$(CONTROLS_DIALOG_MOC_NAMES)))
.PHONY: controls-dialogs-native
controls-dialogs-native: $(DIALOGS_OUT)/test_qt_controls_dialogs.exe
$(DIALOGS_OUT)/test_controls_dialogs.moc: $(srcdir)/tests/test_controls_dialogs.cpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(DIALOGS_OUT)/%.moc.cpp: $(srcdir)/%.hpp
	mkdir -p "$(@D)"
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(DIALOGS_OUT)/%.moc.o: $(DIALOGS_OUT)/%.moc.cpp
	$(CXX) $(CONTROLS_DIALOG_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(DIALOGS_OUT)/tests/test_controls_dialogs.o: $(DIALOGS_OUT)/test_controls_dialogs.moc $(srcdir)/util/modal_input_guard.hpp
$(DIALOGS_OUT)/util/modal_input_guard.o: $(srcdir)/util/modal_input_guard.hpp
$(DIALOGS_OUT)/%.o: $(srcdir)/%.cpp
	mkdir -p "$(@D)"
	$(CXX) $(CONTROLS_DIALOG_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(DIALOGS_OUT)/test_qt_controls_dialogs.exe: $(CONTROLS_DIALOG_OBJECTS) $(CONTROLS_DIALOG_MODEL_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -Wl,--gc-sections -o $@ $(CONTROLS_DIALOG_OBJECTS) $(CONTROLS_DIALOG_MODEL_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
