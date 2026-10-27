# Opt-in native ControlbarProfile layout/draft checks.
ifndef PROPOSAL_OUT
$(error Set PROPOSAL_OUT to a fresh owned output directory)
endif
PROFILE_LAYOUT_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(QT_QTEST_COMMON_cppflags) -I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src -I$(PROPOSAL_OUT)
.PHONY: player-profile-layout-native
player-profile-layout-native: $(PROPOSAL_OUT)/test_qt_player_profile_layout.exe
$(PROPOSAL_OUT):
	mkdir -p "$@"
$(PROPOSAL_OUT)/test_player_profile_layout.moc: $(srcdir)/tests/test_player_profile_layout.cpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/player_profile_layout.moc.cpp: $(srcdir)/maininterface/player_profile_layout.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/controlbar_profile.moc.cpp: $(srcdir)/dialogs/toolbar/controlbar_profile.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/player_controlbar_model.moc.cpp: $(srcdir)/player/player_controlbar_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/control_list_model.moc.cpp: $(srcdir)/player/control_list_model.hpp | $(PROPOSAL_OUT)
	$(MOC) $(MOC_CPPFLAGS) -o $@ $<
$(PROPOSAL_OUT)/test_player_profile_layout.o: $(srcdir)/tests/test_player_profile_layout.cpp $(PROPOSAL_OUT)/test_player_profile_layout.moc | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_profile_layout.o: $(srcdir)/maininterface/player_profile_layout.cpp $(srcdir)/maininterface/player_profile_layout.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/controlbar_profile.o: $(srcdir)/dialogs/toolbar/controlbar_profile.cpp $(srcdir)/dialogs/toolbar/controlbar_profile.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_controlbar_model.o: $(srcdir)/player/player_controlbar_model.cpp $(srcdir)/player/player_controlbar_model.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/control_list_model.o: $(srcdir)/player/control_list_model.cpp $(srcdir)/player/control_list_model.hpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/winpthreads_shutdown.o: $(srcdir)/util/winpthreads_shutdown.cpp | $(PROPOSAL_OUT)
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_profile_layout_moc.o: $(PROPOSAL_OUT)/player_profile_layout.moc.cpp
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/controlbar_profile_moc.o: $(PROPOSAL_OUT)/controlbar_profile.moc.cpp
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/player_controlbar_model_moc.o: $(PROPOSAL_OUT)/player_controlbar_model.moc.cpp
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
$(PROPOSAL_OUT)/control_list_model_moc.o: $(PROPOSAL_OUT)/control_list_model.moc.cpp
	$(CXX) -std=c++17 $(PROFILE_LAYOUT_CPPFLAGS) $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) -c $< -o $@
PROFILE_LAYOUT_OBJECTS = $(PROPOSAL_OUT)/test_player_profile_layout.o $(PROPOSAL_OUT)/player_profile_layout.o $(PROPOSAL_OUT)/controlbar_profile.o $(PROPOSAL_OUT)/player_controlbar_model.o $(PROPOSAL_OUT)/control_list_model.o $(PROPOSAL_OUT)/winpthreads_shutdown.o $(PROPOSAL_OUT)/player_profile_layout_moc.o $(PROPOSAL_OUT)/controlbar_profile_moc.o $(PROPOSAL_OUT)/player_controlbar_model_moc.o $(PROPOSAL_OUT)/control_list_model_moc.o
$(PROPOSAL_OUT)/test_qt_player_profile_layout.exe: $(PROFILE_LAYOUT_OBJECTS)
	$(LIBTOOL) --tag=CXX --mode=link $(CXX) -std=c++17 $(QT_QTEST_COMMON_cxxflags) $(CXXFLAGS) $(QT_QTEST_COMMON_ldflags) -o $@ $(PROFILE_LAYOUT_OBJECTS) $(QT_QTEST_COMMON_ldadd) -Wl,--wrap=pthread_key_delete -lntdll -Wl,--strip-debug
