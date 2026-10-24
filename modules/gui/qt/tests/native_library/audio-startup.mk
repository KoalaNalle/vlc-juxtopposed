# Opt-in console test: real preference writer and MMDevice/WASAPI defaults.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
# Prefer the generated test metadata for this owned build over a stale moc in
# the main Qt build directory. Source-relative includes keep their usual order.
AUDIO_STARTUP_CPPFLAGS = -iquote $(PROPOSAL_OUT) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
.PHONY: audio-startup-native
audio-startup-native: $(PROPOSAL_OUT)/test_qt_audio_startup_consumer.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_audio_startup_consumer.moc: $(srcdir)/tests/test_audio_startup_consumer.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/audio_preferences_model.moc.cpp: $(srcdir)/maininterface/audio_preferences_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_audio_startup_consumer.o: $(srcdir)/tests/test_audio_startup_consumer.cpp $(PROPOSAL_OUT)/test_audio_startup_consumer.moc | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_preferences_model.o: $(srcdir)/maininterface/audio_preferences_model.cpp $(srcdir)/maininterface/audio_preferences_model.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_preferences_model_moc.o: $(PROPOSAL_OUT)/audio_preferences_model.moc.cpp
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/checked_preferences_store.o: $(srcdir)/util/checked_preferences_store.cpp $(srcdir)/util/checked_preferences_store.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_windows_devices.o: $(srcdir)/util/audio_windows_devices.cpp $(srcdir)/util/audio_windows_devices.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/general_preferences_probe.o: $(srcdir)/tests/general_preferences_probe.c | $(PROPOSAL_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
AUDIO_STARTUP_OBJECTS = $(addprefix $(PROPOSAL_OUT)/,test_audio_startup_consumer.o audio_preferences_model.o audio_preferences_model_moc.o checked_preferences_store.o audio_windows_devices.o winpthreads_shutdown.o general_preferences_probe.o)
$(PROPOSAL_OUT)/test_qt_audio_startup_consumer.exe: $(AUDIO_STARTUP_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(AUDIO_STARTUP_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll $(LIBCOM) -luuid -Wl,--strip-debug

# The established defaults/PCM suite can reuse these exact model/store objects.
.PHONY: audio-defaults-native
audio-defaults-native: $(PROPOSAL_OUT)/test_qt_audio_preferences.exe
$(PROPOSAL_OUT)/test_audio_preferences.moc: $(srcdir)/tests/test_audio_preferences.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_audio_preferences.o: $(srcdir)/tests/test_audio_preferences.cpp $(PROPOSAL_OUT)/test_audio_preferences.moc | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_preferences_pcm_probe.o: $(srcdir)/tests/audio_preferences_pcm_probe.c | $(PROPOSAL_OUT)
	$(CC) $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src $(CFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_equalizer.moc.cpp: $(srcdir)/player/audio_equalizer.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/variables.moc.cpp: $(srcdir)/util/variables.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/audio_equalizer.o: $(srcdir)/player/audio_equalizer.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/variables.o: $(srcdir)/util/variables.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/audio_equalizer_moc.o: $(PROPOSAL_OUT)/audio_equalizer.moc.cpp
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/variables_moc.o: $(PROPOSAL_OUT)/variables.moc.cpp
	$(CXX) -std=c++17 $(AUDIO_STARTUP_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
AUDIO_DEFAULTS_OBJECTS = $(filter-out %/test_audio_startup_consumer.o,$(AUDIO_STARTUP_OBJECTS)) $(addprefix $(PROPOSAL_OUT)/,test_audio_preferences.o audio_preferences_pcm_probe.o audio_equalizer.o variables.o audio_equalizer_moc.o variables_moc.o)
$(PROPOSAL_OUT)/test_qt_audio_preferences.exe: $(AUDIO_DEFAULTS_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(AUDIO_DEFAULTS_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll $(LIBCOM) -luuid -Wl,--strip-debug
