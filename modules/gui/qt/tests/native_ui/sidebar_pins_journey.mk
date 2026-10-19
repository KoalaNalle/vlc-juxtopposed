QA_SOURCE ?= $(srcdir)/tests/native_ui/sidebar_pins_journey.cpp
include $(srcdir)/tests/native_ui/network_journey.mk
libqt_plugin_la_LDFLAGS += -Wl,--strip-debug
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/movable_dialog_journey.hpp \
    $(srcdir)/tests/native_ui/navigation_recovery_journey.hpp \
    $(srcdir)/tests/native_ui/dialog_busy_journey.hpp \
    $(srcdir)/tests/native_ui/setup_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/profiles_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/transport_defaults_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/customizer_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/favorite_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/playlist_origin_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/fullscreen_controls_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/settings_activation_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/tooltip_edge_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/audio_device_settings_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/interface_title_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/auto_hide_journey.hpp
$(QA_OUT)/native-ui-journey.o: $(srcdir)/tests/native_ui/fullscreen_screen_journey.hpp
