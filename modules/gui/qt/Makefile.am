guidir = $(pluginsdir)/gui
gui_LTLIBRARIES =
gui_PLUGINS =
gui_RPATH =

if HAVE_PARTIAL_LINKING
noinst_LTLIBRARIES = $(gui_PLUGINS)
else
gui_LTLIBRARIES += $(gui_PLUGINS)
gui_RPATH += -rpath '$(guidir)'
endif


## Howto
# For each Q_OBJECT:
#   - Add *.moc.cpp to nodist_..._SOURCES
#   - Add *.cpp to ..._SOURCES
#   - Add *.hpp to ..._SOURCES
# For each UI
#   - Add *.ui to ..._UI
#   - Add *.h to nodist_..._SOURCES
# For each resource (icon, png, ...)
#   - Add it to DEPS_res
# For each qml and js resource
#   - Add it to ..._QML

CLEANFILES =

include ../../common.am

# Generate vlc_modules_list for Qt plugins
include ../../modulelist.mk

SUFFIXES += .ui .h .hpp .moc.cpp .qml .js .mjs .moc
TEST_EXTENSIONS = .qml
EXTRA_DIST = qt6.pro qtest.pro quicktest.pro scripts
vlc_qt_libs = $(QT_LIBS)
RCC_EXTRA_PARAMS =

all-local: $(VLC_QT_CHECK_DEP) $(QML_MODULE_STAMPS)

if !HAVE_QT_ZSTD
RCC_EXTRA_PARAMS += --no-zstd
endif

vlc_qt_check_SOURCES = vlc-qt-check.cpp plugins.hpp
vlc_qt_check_CXXFLAGS = $(AM_CXXFLAGS) $(QT_CFLAGS) -fPIC $(CXXFLAGS_qt)
vlc_qt_check_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS_qt)
vlc_qt_check_LDADD = $(vlc_qt_libs)
vlc_qt_check_LDFLAGS = $(AM_LDFLAGS) $(QT_LDFLAGS)

libqt_plugin_la_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS_qt) \
	-DQT_STRICT_ITERATORS \
	-DQT_USE_QSTRINGBUILDER \
	-DQT_NO_CAST_TO_ASCII \
	-DQT_NO_CAST_FROM_BYTEARRAY \
	-I$(srcdir) -I$(builddir) \
	-I$(builddir)/dialogs/extended \
	-I$(builddir)/dialogs/fingerprint \
	-I$(builddir)/dialogs/firstrun \
	-I$(builddir)/dialogs/help \
	-I$(builddir)/dialogs/messages \
	-I$(builddir)/dialogs/open \
	-I$(builddir)/dialogs/podcast \
	-I$(builddir)/dialogs/preferences \
	-I$(builddir)/dialogs/sout \
	-I$(builddir)/dialogs/vlm

libqt_plugin_la_CXXFLAGS = $(AM_CXXFLAGS) $(QT_CFLAGS) $(CXXFLAGS_qt) -fPIC
libqt_plugin_la_LIBADD = $(vlc_qt_libs)
if HAVE_LIBATOMIC
libqt_plugin_la_CXXFLAGS += -DQT_HAS_LIBATOMIC
libqt_plugin_la_LIBADD += -latomic
endif
libqt_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(QT_LDFLAGS) $(GC_SECTIONS_LDFLAGS)
if HAVE_OSX
libqt_plugin_la_LDFLAGS += -Wl,-framework,Cocoa
endif

if HAVE_X
if HAVE_XCB
libqt_plugin_la_CXXFLAGS += $(XCB_CFLAGS) -DQT_HAS_X11
libqt_plugin_la_LIBADD += $(XCB_LIBS) $(X_LIBS) $(X_PRE_LIB) -lX11
vlc_qt_check_CXXFLAGS += $(XCB_CFLAGS) -DQT_HAS_X11
endif
endif

if HAVE_WIN32
libqt_plugin_la_CXXFLAGS += $(LIBCOMCXXFLAGS)
libqt_plugin_la_LIBADD += $(LIBCOM) -lcomctl32 -luuid -ldwmapi
libqt_plugin_la_LIBADD += -lntdll
libqt_plugin_la_LDFLAGS += -Wl,--wrap=pthread_key_delete
endif
if UPDATE_CHECK
libqt_plugin_la_CPPFLAGS += -DUPDATE_CHECK
endif
if QT_QML_DEBUG
libqt_plugin_la_CPPFLAGS += -DQT_QML_DEBUG
endif
if HAVE_QT_GTK
libqt_plugin_la_CPPFLAGS += -DQT_HAS_GTK
endif

libqt_plugin_la_SOURCES = \
	qt.cpp qt.hpp plugins.hpp \
	dialogs/bookmarks/bookmarks.cpp dialogs/bookmarks/bookmarks.hpp \
	dialogs/dialogs/dialogmodel.cpp \
	dialogs/dialogs/dialogmodel.hpp \
	dialogs/dialogs_provider.cpp dialogs/dialogs_provider.hpp \
	dialogs/epg/EPGChannels.cpp \
	dialogs/epg/EPGChannels.hpp \
	dialogs/epg/EPGItem.cpp \
	dialogs/epg/EPGItem.hpp \
	dialogs/epg/EPGProgram.cpp \
	dialogs/epg/EPGProgram.hpp \
	dialogs/epg/EPGRuler.cpp dialogs/epg/EPGRuler.hpp \
	dialogs/epg/EPGView.cpp dialogs/epg/EPGView.hpp \
	dialogs/epg/EPGWidget.cpp \
	dialogs/epg/EPGWidget.hpp \
	dialogs/epg/epg.cpp dialogs/epg/epg.hpp \
	dialogs/extended/extended.cpp dialogs/extended/extended.hpp \
	dialogs/extended/extended_panels.cpp \
	dialogs/extended/extended_panels.hpp \
	dialogs/extensions/extensions.cpp dialogs/extensions/extensions.hpp \
	dialogs/extensions/extensions_manager.cpp dialogs/extensions/extensions_manager.hpp \
	dialogs/fingerprint/chromaprint.cpp \
	dialogs/fingerprint/chromaprint.hpp \
	dialogs/fingerprint/fingerprintdialog.cpp \
	dialogs/fingerprint/fingerprintdialog.hpp \
	dialogs/firstrun/firstrunwizard.cpp dialogs/firstrun/firstrunwizard.hpp \
	dialogs/gototime/gototime.cpp dialogs/gototime/gototime.hpp \
	dialogs/help/aboutmodel.hpp \
	dialogs/help/help.cpp dialogs/help/help.hpp \
	dialogs/mediainfo/info_panels.cpp dialogs/mediainfo/info_panels.hpp \
	dialogs/mediainfo/info_widgets.cpp dialogs/mediainfo/info_widgets.hpp \
	dialogs/mediainfo/mediainfo.cpp dialogs/mediainfo/mediainfo.hpp \
	dialogs/messages/messages.cpp dialogs/messages/messages.hpp \
	dialogs/open/open.cpp dialogs/open/open.hpp \
	dialogs/open/open_panels.cpp dialogs/open/open_panels.hpp \
	dialogs/open/openurl.cpp dialogs/open/openurl.hpp \
	dialogs/plugins/plugins.cpp dialogs/plugins/plugins.hpp \
	dialogs/podcast/podcast_configuration.cpp \
	dialogs/podcast/podcast_configuration.hpp \
	dialogs/preferences/complete_preferences.cpp \
	dialogs/preferences/complete_preferences.hpp \
	dialogs/preferences/expert_model.cpp \
	dialogs/preferences/expert_model.hpp \
	dialogs/preferences/expert_view.cpp \
	dialogs/preferences/expert_view.hpp \
	dialogs/preferences/preferences.cpp \
	dialogs/preferences/preferences.hpp \
	dialogs/preferences/preferences_widgets.cpp \
	dialogs/preferences/preferences_widgets.hpp \
	dialogs/preferences/simple_preferences.cpp \
	dialogs/preferences/simple_preferences.hpp \
	dialogs/sout/convert.cpp dialogs/sout/convert.hpp \
	dialogs/sout/profile_selector.cpp \
	dialogs/sout/profile_selector.hpp \
	dialogs/sout/profiles.hpp \
	dialogs/sout/sout.cpp dialogs/sout/sout.hpp \
	dialogs/sout/sout_widgets.cpp \
	dialogs/sout/sout_widgets.hpp \
	dialogs/systray/systray.cpp \
	dialogs/systray/systray.hpp \
	dialogs/toolbar/controlbar_profile.hpp \
	dialogs/toolbar/controlbar_profile.cpp \
	dialogs/toolbar/controlbar_profile_model.cpp \
	dialogs/toolbar/controlbar_profile_model.hpp \
	dialogs/vlm/vlm.cpp dialogs/vlm/vlm.hpp \
	dialogs/playlists/playlists.cpp dialogs/playlists/playlists.hpp \
	maininterface/compositor.hpp \
	maininterface/compositor.cpp \
	maininterface/compositor_common.hpp \
	maininterface/compositor_common.cpp \
	maininterface/compositor_accessibility.cpp \
	maininterface/compositor_accessibility.hpp \
	maininterface/compositor_dummy.hpp \
	maininterface/compositor_dummy.cpp \
	maininterface/compositor_platform.hpp \
	maininterface/compositor_platform.cpp \
	maininterface/interface_window_handler.cpp \
	maininterface/interface_window_handler.hpp \
	maininterface/mainctx.cpp \
	maininterface/general_preferences_model.cpp maininterface/general_preferences_model.hpp \
	maininterface/interface_preferences_model.cpp maininterface/interface_preferences_model.hpp \
	maininterface/playback_preferences_model.cpp maininterface/playback_preferences_model.hpp \
	maininterface/audio_preferences_model.cpp maininterface/audio_preferences_model.hpp \
	maininterface/subtitles_preferences_model.cpp maininterface/subtitles_preferences_model.hpp \
	maininterface/library_preferences_model.cpp maininterface/library_preferences_model.hpp \
	maininterface/video_preferences_model.cpp maininterface/video_preferences_model.hpp \
	util/audio_defaults_policy.hpp util/audio_windows_devices.cpp util/audio_windows_devices.hpp \
	util/playback_seek_policy.hpp \
	util/checked_preferences_store.cpp util/checked_preferences_store.hpp \
	util/fullscreen_screen_policy.hpp \
	util/interface_language_map.hpp \
	maininterface/mainctx.hpp \
	maininterface/mainctx_submodels.cpp maininterface/mainctx_submodels.hpp \
	maininterface/mainui.cpp \
	maininterface/mainui.hpp \
	maininterface/navigationmodel.cpp maininterface/navigationmodel.hpp \
	maininterface/videosurface.cpp \
	maininterface/videosurface.hpp \
	maininterface/video_window_handler.cpp \
	maininterface/video_window_handler.hpp \
	maininterface/windoweffects_module.hpp \
	medialibrary/medialib.cpp \
	medialibrary/medialib.hpp \
	medialibrary/mlmediastore.cpp \
	medialibrary/mlmediastore.hpp \
	medialibrary/mlalbum.cpp \
	medialibrary/mlalbum.hpp \
	medialibrary/mlalbummodel.cpp \
	medialibrary/mlalbummodel.hpp \
	medialibrary/mlaudio.cpp \
	medialibrary/mlaudio.hpp \
	medialibrary/mlaudiomodel.cpp \
	medialibrary/mlaudiomodel.hpp \
	medialibrary/mlartist.cpp \
	medialibrary/mlartist.hpp \
	medialibrary/mlartistmodel.cpp \
	medialibrary/mlartistmodel.hpp \
	medialibrary/mlbasemodel.cpp \
	medialibrary/mlbasemodel.hpp \
	medialibrary/mlbookmarkmodel.cpp \
	medialibrary/mlbookmarkmodel.hpp \
	medialibrary/mlevent.hpp \
	medialibrary/mlfolder.cpp \
	medialibrary/mlfolder.hpp \
	medialibrary/mlfoldersmodel.cpp \
	medialibrary/mlfoldersmodel.hpp \
	medialibrary/mlgenre.cpp \
	medialibrary/mlgenre.hpp \
	medialibrary/mlgenremodel.cpp \
	medialibrary/mlgenremodel.hpp \
	medialibrary/mlgroup.cpp \
	medialibrary/mlgroup.hpp \
	medialibrary/mlhelper.cpp \
	medialibrary/mlhelper.hpp \
	medialibrary/mlthreadpool.cpp \
	medialibrary/mlthreadpool.hpp \
	medialibrary/mlqmltypes.hpp \
	medialibrary/mlmedia.hpp \
	medialibrary/mlqueryparams.cpp \
	medialibrary/mlqueryparams.hpp \
	medialibrary/mlrecentmediamodel.cpp \
	medialibrary/mlrecentmediamodel.hpp \
	medialibrary/mlrecentvideomodel.cpp \
	medialibrary/mlrecentvideomodel.hpp \
	medialibrary/mlrecentaudiomodel.cpp \
	medialibrary/mlrecentaudiomodel.hpp \
	medialibrary/mlhomeactivity.cpp \
	medialibrary/mlhomeactivity.hpp \
	medialibrary/mlhomeactivitydata.cpp \
	medialibrary/mlhomeactivitydata.hpp \
	medialibrary/mlurlmodel.cpp \
	medialibrary/mlurlmodel.hpp \
	medialibrary/mlvideo.cpp \
	medialibrary/mlvideo.hpp \
	medialibrary/mlvideofoldersmodel.cpp \
	medialibrary/mlvideofoldersmodel.hpp \
	medialibrary/mlvideogroupsmodel.cpp \
	medialibrary/mlvideogroupsmodel.hpp \
	medialibrary/mlmediamodel.cpp \
	medialibrary/mlmediamodel.hpp \
	medialibrary/mlvideomodel.cpp \
	medialibrary/mlvideomodel.hpp \
	medialibrary/mlplaylist.cpp \
	medialibrary/mlplaylist.hpp \
	medialibrary/mlplaylistlistmodel.cpp \
	medialibrary/mlplaylistlistmodel.hpp \
	medialibrary/mlplaylistmedia.cpp \
	medialibrary/mlplaylistmedia.hpp \
	medialibrary/mlplaylistmodel.cpp \
	medialibrary/mlplaylistmodel.hpp \
	medialibrary/thumbnailcollector.hpp \
	medialibrary/thumbnailcollector.cpp \
	medialibrary/mlcustomcover.hpp \
	medialibrary/mlcustomcover.cpp \
	menus/custom_menus.cpp \
	menus/custom_menus.hpp \
	menus/qml_menu_wrapper.cpp \
	menus/qml_menu_wrapper.hpp \
	menus/menus.cpp menus/menus.hpp \
	network/addonsmodel.cpp \
	network/addonsmodel.hpp \
	network/mediatreelistener.cpp \
	network/mediatreelistener.hpp \
	network/devicesourceprovider.cpp \
	network/devicesourceprovider.hpp \
	network/networkbasemodel.cpp \
	network/networkbasemodel.hpp \
	network/networkdevicemodel.cpp \
	network/networkdevicemodel.hpp \
	network/networksourcesmodel.cpp \
	network/networksourcesmodel.hpp \
	network/networkmediamodel.cpp \
	network/networkmediamodel.hpp \
	network/networktreeitem.hpp \
	network/standardpathmodel.cpp \
	network/standardpathmodel.hpp \
	network/vlcmediasourcewrapper.hpp \
	style/qtthemeprovider.hpp \
	style/colorcontext.cpp \
	style/colorcontext.hpp \
	style/systempalette.cpp \
	style/systempalette.hpp \
	style/vlcicons.cpp 	style/vlcicons.hpp \
	style/defaultthemeproviders.hpp \
	style/systempalettethemeprovider.cpp \
	player/input_models.cpp player/input_models.hpp \
	player/delay_estimator.cpp \
	player/delay_estimator.hpp \
	player/player_controller.cpp player/player_controller.hpp player/player_controller_p.hpp \
	player/player_timer_point.hpp \
	player/audio_equalizer.cpp player/audio_equalizer.hpp \
	player/audio_waveform.cpp player/audio_waveform.hpp \
	player/local_music_folder.cpp player/local_music_folder.hpp \
	dialogs/network_stream_model.cpp dialogs/network_stream_model.hpp \
	dialogs/conversion_controller.cpp dialogs/conversion_controller.hpp \
	dialogs/new_playlist_model.cpp \
	dialogs/new_playlist_model.hpp \
	dialogs/new_playlist_data.cpp \
	dialogs/new_playlist_data.hpp \
	player/video_transform.cpp player/video_transform.hpp \
	player/subtitle_appearance.cpp player/subtitle_appearance.hpp \
	player/subtitle_cue_data.hpp \
	player/subtitle_cue_data.cpp \
	player/subtitle_cue_timeline.hpp \
	player/subtitle_cue_timeline.cpp \
	maininterface/subtitle_preset_model.cpp maininterface/subtitle_preset_model.hpp \
	util/filterchain.hpp \
	player/player_controlbar_model.cpp player/player_controlbar_model.hpp \
	player/control_list_model.cpp player/control_list_model.hpp \
	player/control_list_filter.cpp \
	player/control_list_filter.hpp \
	playlist/media.hpp \
	playlist/playlist_common.hpp \
	playlist/playlist_controller.cpp \
	playlist/playlist_controller.hpp \
	playlist/playlist_controller_p.hpp \
	playlist/playlist_item.cpp \
	playlist/playlist_item.hpp \
	playlist/playlist_model.cpp \
	playlist/playlist_model.hpp \
	playlist/playlist_model_p.hpp \
	util/asynctask.hpp \
	util/accessible_compat_attached.cpp util/accessible_compat_attached.hpp \
	util/audio_device_model.cpp  \
	util/audio_device_model.hpp \
	util/workerthreadset.hpp util/workerthreadset.cpp \
	util/base_model.hpp util/base_model_p.hpp util/base_model.cpp \
	util/color_scheme_model.cpp util/color_scheme_model.hpp \
	util/ui_notifier.cpp util/ui_notifier.hpp \
	util/color_svg_image_provider.cpp util/color_svg_image_provider.hpp \
	util/vlcaccess_image_provider.cpp util/vlcaccess_image_provider.hpp \
	util/covergenerator.cpp \
	util/covergenerator.hpp \
	util/imageluminanceextractor.cpp util/imageluminanceextractor.hpp \
	util/csdbuttonmodel.cpp util/csdbuttonmodel.hpp \
	util/csdmenu.cpp util/csdmenu.hpp \
	util/imagehelper.cpp util/imagehelper.hpp \
	util/keyhelper.cpp util/keyhelper.hpp \
	util/listcache.hxx util/listcache.hpp \
	util/listcacheloader.hpp \
	util/locallistcacheloader.hpp \
	util/locallistbasemodel.hpp \
	util/navigation_history.cpp util/navigation_history.hpp \
	util/item_key_event_filter.cpp \
	util/item_key_event_filter.hpp \
	util/flickable_scroll_handler.cpp \
	util/flickable_scroll_handler.hpp \
	util/qt_dirs.cpp util/qt_dirs.hpp \
	util/proxycolumnmodel.hpp \
	util/registry.cpp util/registry.hpp \
	util/renderer_manager.cpp util/renderer_manager.hpp \
	util/singleton.hpp \
	util/soutchain.cpp util/soutchain.hpp \
	util/validators.cpp util/validators.hpp \
	util/varcommon_p.hpp \
	util/varchoicemodel.cpp  util/varchoicemodel.hpp \
	util/variables.cpp util/variables.hpp \
	util/vlchotkeyconverter.cpp util/vlchotkeyconverter.hpp \
	util/vlctick.cpp \
	util/vlctick.hpp \
	util/shared_input_item.hpp \
	util/fast_gaussian_blur_template.h \
	util/effects_image_provider.cpp \
	util/effects_image_provider.hpp \
	util/qsgtextureview.cpp \
	util/qsgtextureview.hpp \
	util/dismiss_popup_event_filter.cpp \
	util/dismiss_popup_event_filter.hpp \
	util/list_selection_model.cpp \
	util/list_selection_model.hpp \
	util/model_recovery_agent.hpp \
	util/vlcqtmessagehandler.cpp \
	util/vlcqtmessagehandler.hpp \
	util/colorizedsvgicon.cpp \
	util/colorizedsvgicon.hpp \
	util/textureproviderobserver.cpp \
	util/textureproviderobserver.hpp \
	util/kirigamiwheelhandler.cpp \
	util/kirigamiwheelhandler.hpp \
	util/textureproviderindirection.cpp \
	util/textureproviderindirection.hpp \
	util/sgmanipulator.cpp \
	util/sgmanipulator.hpp \
	util/pointingtooltipattached.hpp \
	widgets/native/animators.cpp \
	widgets/native/animators.hpp \
	widgets/native/customwidgets.cpp widgets/native/customwidgets.hpp \
	widgets/native/csdthemeimage.cpp \
	widgets/native/csdthemeimage.hpp \
	widgets/native/interface_widgets.cpp \
	widgets/native/interface_widgets.hpp \
	widgets/native/navigation_attached.cpp \
	widgets/native/navigation_attached.hpp \
	widgets/native/mlfolderseditor.cpp \
	widgets/native/mlfolderseditor.hpp \
	widgets/native/qvlcframe.cpp \
	widgets/native/qvlcframe.hpp \
	widgets/native/searchlineedit.cpp widgets/native/searchlineedit.hpp \
	widgets/native/viewblockingrectangle.cpp widgets/native/viewblockingrectangle.hpp \
	widgets/native/doubleclickignoringitem.hpp

# Meta-object compilation

moc_verbose = $(moc_verbose_$(V))
moc_verbose_ = $(moc_verbose__$(AM_DEFAULT_VERBOSITY))
moc_verbose_0 = @echo "  MOC     " $@;
moc_verbose__0 = $(moc_verbose_0)

.hpp.moc.cpp:
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<

.cpp.moc:
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<

# These MOC files depend on the configure settings:
dialogs/dialogs_provider.moc.cpp dialogs/vlm/vlm.moc.cpp: $(top_builddir)/config.h

nodist_libqt_plugin_la_SOURCES = \
	dialogs/bookmarks/bookmarks.moc.cpp \
	dialogs/dialogs/dialogmodel.moc.cpp \
	dialogs/dialogs_provider.moc.cpp \
	dialogs/epg/EPGChannels.moc.cpp \
	dialogs/epg/EPGProgram.moc.cpp \
	dialogs/epg/EPGRuler.moc.cpp \
	dialogs/epg/EPGView.moc.cpp \
	dialogs/epg/EPGWidget.moc.cpp \
	dialogs/epg/epg.moc.cpp \
	dialogs/extended/extended.moc.cpp \
	dialogs/extended/extended_panels.moc.cpp \
	dialogs/extensions/extensions.moc.cpp \
	dialogs/extensions/extensions_manager.moc.cpp \
	dialogs/fingerprint/chromaprint.moc.cpp \
	dialogs/fingerprint/fingerprintdialog.moc.cpp \
	dialogs/firstrun/firstrunwizard.moc.cpp \
	dialogs/gototime/gototime.moc.cpp \
	dialogs/help/aboutmodel.moc.cpp \
	dialogs/help/help.moc.cpp \
	dialogs/mediainfo/info_panels.moc.cpp \
	dialogs/mediainfo/info_widgets.moc.cpp \
	dialogs/mediainfo/mediainfo.moc.cpp \
	dialogs/messages/messages.moc.cpp \
	dialogs/open/open.moc.cpp \
	dialogs/open/open_panels.moc.cpp \
	dialogs/open/openurl.moc.cpp \
	dialogs/plugins/plugins.moc.cpp \
	dialogs/podcast/podcast_configuration.moc.cpp \
	dialogs/preferences/complete_preferences.moc.cpp \
	dialogs/preferences/expert_model.moc.cpp \
	dialogs/preferences/expert_view.moc.cpp \
	dialogs/preferences/preferences.moc.cpp \
	dialogs/preferences/preferences_widgets.moc.cpp \
	dialogs/preferences/simple_preferences.moc.cpp \
	dialogs/sout/convert.moc.cpp \
	dialogs/sout/profile_selector.moc.cpp \
	dialogs/sout/sout.moc.cpp \
	dialogs/sout/sout_widgets.moc.cpp \
	dialogs/systray/systray.moc.cpp \
	dialogs/toolbar/controlbar_profile.moc.cpp \
	dialogs/toolbar/controlbar_profile_model.moc.cpp \
	dialogs/playlists/playlists.moc.cpp \
	maininterface/compositor.moc.cpp \
	maininterface/compositor_common.moc.cpp \
	maininterface/compositor_dummy.moc.cpp \
	maininterface/compositor_platform.moc.cpp \
	maininterface/interface_window_handler.moc.cpp \
	maininterface/mainctx.moc.cpp \
	maininterface/general_preferences_model.moc.cpp \
	maininterface/interface_preferences_model.moc.cpp \
	maininterface/playback_preferences_model.moc.cpp \
	maininterface/audio_preferences_model.moc.cpp \
	maininterface/subtitles_preferences_model.moc.cpp \
	maininterface/library_preferences_model.moc.cpp \
	maininterface/video_preferences_model.moc.cpp \
	maininterface/mainctx_submodels.moc.cpp \
	maininterface/mainui.moc.cpp \
	maininterface/navigationmodel.moc.cpp \
	maininterface/videosurface.moc.cpp \
	maininterface/video_window_handler.moc.cpp \
	medialibrary/medialib.moc.cpp \
	medialibrary/mlmediastore.moc.cpp \
	medialibrary/mlalbummodel.moc.cpp \
	medialibrary/mlaudiomodel.moc.cpp \
	medialibrary/mlartistmodel.moc.cpp \
	medialibrary/mlbookmarkmodel.moc.cpp \
	medialibrary/mlbasemodel.moc.cpp \
	medialibrary/mlfoldersmodel.moc.cpp \
	medialibrary/mlgenremodel.moc.cpp \
	medialibrary/mlthreadpool.moc.cpp \
	medialibrary/mlqmltypes.moc.cpp \
	medialibrary/mlrecentmediamodel.moc.cpp \
	medialibrary/mlrecentvideomodel.moc.cpp \
	medialibrary/mlrecentaudiomodel.moc.cpp \
	medialibrary/mlhomeactivity.moc.cpp \
	medialibrary/mlurlmodel.moc.cpp \
	medialibrary/mlvideo.moc.cpp \
	medialibrary/mlmediamodel.moc.cpp \
	medialibrary/mlvideomodel.moc.cpp \
	medialibrary/mlplaylistlistmodel.moc.cpp \
	medialibrary/mlplaylistmodel.moc.cpp \
	medialibrary/mlvideofoldersmodel.moc.cpp \
	medialibrary/mlvideogroupsmodel.moc.cpp \
	medialibrary/thumbnailcollector.moc.cpp \
	menus/custom_menus.moc.cpp \
	menus/qml_menu_wrapper.moc.cpp \
	menus/menus.moc.cpp \
	network/addonsmodel.moc.cpp \
	network/devicesourceprovider.moc.cpp \
	network/networkdevicemodel.moc.cpp \
	network/networkbasemodel.moc.cpp \
	network/networksourcesmodel.moc.cpp \
	network/networkmediamodel.moc.cpp \
	network/networktreeitem.moc.cpp \
	network/standardpathmodel.moc.cpp \
	style/colorcontext.moc.cpp \
	style/systempalette.moc.cpp \
	style/vlcicons.moc.cpp \
	player/delay_estimator.moc.cpp \
	player/input_models.moc.cpp \
	player/player_controller.moc.cpp \
	player/audio_equalizer.moc.cpp \
	player/audio_waveform.moc.cpp \
	player/local_music_folder.moc.cpp \
	dialogs/network_stream_model.moc.cpp \
	dialogs/conversion_controller.moc.cpp \
	dialogs/new_playlist_model.moc.cpp \
	player/video_transform.moc.cpp \
	player/subtitle_appearance.moc.cpp \
	player/subtitle_cue_timeline.moc.cpp \
	maininterface/subtitle_preset_model.moc.cpp \
	player/player_controlbar_model.moc.cpp \
	player/control_list_model.moc.cpp \
	player/control_list_filter.moc.cpp \
	playlist/playlist_controller.moc.cpp \
	playlist/playlist_item.moc.cpp \
	playlist/playlist_model.moc.cpp \
	util/asynctask.moc.cpp \
	util/accessible_compat_attached.moc.cpp \
	util/audio_device_model.moc.cpp \
	util/workerthreadset.moc.cpp \
	util/base_model.moc.cpp \
	util/color_scheme_model.moc.cpp \
	util/ui_notifier.moc.cpp \
	util/color_svg_image_provider.moc.cpp \
	util/vlcaccess_image_provider.moc.cpp \
	util/imageluminanceextractor.moc.cpp \
	util/csdbuttonmodel.moc.cpp \
	util/csdmenu.moc.cpp \
	util/keyhelper.moc.cpp \
	util/listcache.moc.cpp \
	util/locallistcacheloader.moc.cpp \
	util/navigation_history.moc.cpp \
	util/item_key_event_filter.moc.cpp \
	util/effects_image_provider.moc.cpp \
	util/flickable_scroll_handler.moc.cpp \
	util/renderer_manager.moc.cpp \
	util/validators.moc.cpp \
	util/varchoicemodel.moc.cpp \
	util/variables.moc.cpp \
	util/vlctick.moc.cpp \
	util/dismiss_popup_event_filter.moc.cpp \
	util/list_selection_model.moc.cpp \
	util/vlchotkeyconverter.moc.cpp \
	util/qsgtextureview.moc.cpp \
	util/textureproviderobserver.moc.cpp \
	util/kirigamiwheelhandler.moc.cpp \
	util/textureproviderindirection.moc.cpp \
	util/sgmanipulator.moc.cpp \
	util/pointingtooltipattached.moc.cpp \
	widgets/native/animators.moc.cpp \
	widgets/native/csdthemeimage.moc.cpp \
	widgets/native/customwidgets.moc.cpp \
	widgets/native/interface_widgets.moc.cpp \
	widgets/native/navigation_attached.moc.cpp \
	widgets/native/mlfolderseditor.moc.cpp \
	widgets/native/searchlineedit.moc.cpp \
	widgets/native/viewblockingrectangle.moc.cpp

nodist_libqt_plugin_la_SOURCES += \
	dialogs/extended/ui_equalizer.h \
	dialogs/extended/ui_video_effects.h \
	dialogs/fingerprint/ui_fingerprintdialog.h \
	dialogs/firstrun/ui_firstrunwizard.h \
	dialogs/help/ui_about.h \
	dialogs/help/ui_update.h \
	dialogs/messages/ui_messages_panel.h \
	dialogs/open/ui_open.h \
	dialogs/open/ui_open_capture.h \
	dialogs/open/ui_open_disk.h \
	dialogs/open/ui_open_file.h \
	dialogs/open/ui_open_net.h \
	dialogs/podcast/ui_podcast_configuration.h \
	dialogs/preferences/ui_sprefs_audio.h \
	dialogs/preferences/ui_sprefs_input.h \
	dialogs/preferences/ui_sprefs_interface.h \
	dialogs/preferences/ui_sprefs_medialibrary.h \
	dialogs/preferences/ui_sprefs_subtitles.h \
	dialogs/preferences/ui_sprefs_video.h \
	dialogs/sout/ui_profiles.h \
	dialogs/sout/ui_sout.h \
	dialogs/vlm/ui_vlm.h


# windows compositor

if HAVE_WIN32

libqt_plugin_la_SOURCES += \
	maininterface/mainctx_win32.cpp \
	maininterface/mainctx_win32.hpp \
	maininterface/compositor_win7.cpp \
	maininterface/compositor_win7.hpp \
	style/windowsthemeprovider.cpp \
	maininterface/win32windoweffects_module.cpp \
	maininterface/win32windoweffects_module.hpp \
	util/csdmenu_win32.c \
	util/csdmenu_win32.h \
	util/winpthreads_shutdown.cpp

nodist_libqt_plugin_la_SOURCES += maininterface/mainctx_win32.moc.cpp \
	maininterface/compositor_win7.moc.cpp

libqt_plugin_la_SOURCES += \
	maininterface/compositor_dcomp.cpp \
	maininterface/compositor_dcomp.hpp \
	maininterface/compositor_dcomp_error.hpp \
	maininterface/compositor_dcomp_acrylicsurface.hpp \
	maininterface/compositor_dcomp_acrylicsurface.cpp

nodist_libqt_plugin_la_SOURCES += \
	maininterface/compositor_dcomp_acrylicsurface.moc.cpp \
	maininterface/compositor_dcomp.moc.cpp

endif

# X11 compositor

if HAVE_X
if HAVE_QT_XCB
if HAVE_XCB
if HAVE_XCB_DAMAGE
if HAVE_XCB_XFIXES

libqt_plugin_la_CXXFLAGS += -DQT_HAS_X11_COMPOSITOR \
	$(QT_X11_CFLAGS) $(XCB_RENDER_CFLAGS) $(XCB_COMPOSITE_CFLAGS) $(XCB_DAMAGE_CFLAGS) $(XCB_XFIXES_CFLAGS)

libqt_plugin_la_LIBADD += $(QT_X11_LIBS) $(XCB_RENDER_LIBS) $(XCB_COMPOSITE_LIBS) $(XCB_DAMAGE_LIBS) $(XCB_XFIXES_LIBS)

libqt_plugin_la_SOURCES += \
	maininterface/compositor_x11.cpp \
	maininterface/compositor_x11.hpp \
	maininterface/compositor_x11_renderclient.cpp \
	maininterface/compositor_x11_renderclient.hpp \
	maininterface/compositor_x11_renderwindow.cpp \
	maininterface/compositor_x11_renderwindow.hpp \
	maininterface/compositor_x11_uisurface.cpp \
	maininterface/compositor_x11_uisurface.hpp \
	maininterface/compositor_x11_utils.cpp \
	maininterface/compositor_x11_utils.hpp

nodist_libqt_plugin_la_SOURCES += \
	maininterface/compositor_x11.moc.cpp \
	maininterface/compositor_x11_renderclient.moc.cpp \
	maininterface/compositor_x11_renderwindow.moc.cpp \
	maininterface/compositor_x11_uisurface.moc.cpp

#extra X11 features

libqt_x11_plugin_la_CXXFLAGS = $(AM_CXXFLAGS) $(XCB_CFLAGS) $(XCB_RENDER_CFLAGS) $(XCB_COMPOSITE_CFLAGS) $(XCB_DAMAGE_CFLAGS) $(XCB_XFIXES_CFLAGS)
libqt_x11_plugin_la_LIBADD = $(XCB_LIBS) $(XCB_RENDER_LIBS) $(XCB_COMPOSITE_LIBS) $(XCB_DAMAGE_LIBS) $(XCB_XFIXES_LIBS)
libqt_x11_plugin_la_SOURCES = \
	maininterface/qt_x11_plugin.c \
	maininterface/compositor_x11_utils.hpp \
	maininterface/compositor_x11_utils.cpp \
	util/csdmenu_module.h \
	util/csdmenu_x11.cpp \
	util/csdmenu_x11.h

if ENABLE_QT
gui_PLUGINS += libqt_x11_plugin.la
endif

endif
endif
endif
endif
endif

# wayland compositor

if HAVE_WAYLAND
if HAVE_QT_GUI_PRIVATE

libqt_plugin_la_CPPFLAGS += -DQT_HAS_WAYLAND_COMPOSITOR

libqt_plugin_la_SOURCES += \
	maininterface/compositor_wayland.cpp \
	maininterface/compositor_wayland.hpp


nodist_libqt_plugin_la_SOURCES += \
	maininterface/compositor_wayland.moc.cpp

libqt_wayland_plugin_la_CFLAGS = $(AM_CFLAGS) $(WAYLAND_CLIENT_CFLAGS)
libqt_wayland_plugin_la_LIBADD =  $(WAYLAND_CLIENT_LIBS) $(LIBM)
libqt_wayland_plugin_la_SOURCES = \
	maininterface/qt_wayland_plugin.c \
	maininterface/compositor_wayland_module.c \
	maininterface/compositor_wayland_module.h

#if WAYLAND_PROTOCOLS
#if WAYLAND_SCANNER
libqt_wayland_plugin_la_CFLAGS += -DQT_HAS_WAYLAND_PROTOCOLS -I$(builddir)/maininterface

libqt_wayland_plugin_la_SOURCES += \
	util/csdmenu_wayland.c \
	util/csdmenu_wayland.h \
	util/csdmenu_module.h

nodist_libqt_wayland_plugin_la_SOURCES = \
	maininterface/xdg-shell-client-protocol.h \
	maininterface/xdg-shell-protocol.c

if HAVE_QT65
libqt_wayland_plugin_la_CFLAGS += -DQT_HAS_WAYLAND_FRACTIONAL_SCALING
nodist_libqt_wayland_plugin_la_SOURCES += \
	maininterface/viewporter-client-protocol.h \
	maininterface/viewporter-protocol.c

maininterface/viewporter-client-protocol.h: \
		$(WAYLAND_PROTOCOLS)/stable/viewporter/viewporter.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header $< $@

maininterface/viewporter-protocol.c: \
		$(WAYLAND_PROTOCOLS)/stable/viewporter/viewporter.xml \
		maininterface/viewporter-client-protocol.h
	$(AM_V_GEN)$(WAYLAND_SCANNER) private-code $< $@
endif

BUILT_SOURCES += $(nodist_libqt_wayland_plugin_la_SOURCES)

maininterface/xdg-shell-client-protocol.h: $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header $< $@

maininterface/xdg-shell-protocol.c: $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) private-code $< $@

#endif
#endif

if ENABLE_QT
gui_PLUGINS += libqt_wayland_plugin.la
endif

endif
endif

if !HAVE_WIN32
if HAVE_KF6WINDOWSYSTEM

libqt_plugin_la_CPPFLAGS += -DQT_HAS_KWINDOWSYSTEM_PLUGIN

libqt_kwindowsystem_plugin_la_CXXFLAGS = $(AM_CXXFLAGS) $(KF6WINDOWSYSTEM_CFLAGS)
libqt_kwindowsystem_plugin_la_LIBADD =  $(KF6WINDOWSYSTEM_LIBS) $(QT_LIBS)
libqt_kwindowsystem_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(QT_LDFLAGS)
libqt_kwindowsystem_plugin_la_SOURCES = \
	maininterface/kwindowsystem_module.cpp

if ENABLE_QT
gui_PLUGINS += libqt_kwindowsystem_plugin.la
endif
endif
endif

# User interface compilation

uic_verbose = $(uic_verbose_$(V))
uic_verbose_ = $(uic_verbose__$(AM_DEFAULT_VERBOSITY))
uic_verbose_0 = @echo "  UIC     " $@;
uic_verbose__0 = $(uic_verbose_0)

MOC_CPPFLAGS = $(DEFS) \
	-I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_builddir) \
	$(QT_MOC_INCLUDE_DIRS) \
	$(QT_CFLAGS) \
	$(libqt_plugin_la_CPPFLAGS)
if HAVE_DARWIN
MOC_CPPFLAGS += -DQ_OS_MAC
endif
if HAVE_WIN32
MOC_CPPFLAGS += -D_WIN32
endif

libqt_plugin_la_UI = \
	dialogs/extended/equalizer.ui \
	dialogs/extended/video_effects.ui \
	dialogs/fingerprint/fingerprintdialog.ui \
	dialogs/firstrun/firstrunwizard.ui \
	dialogs/help/about.ui \
	dialogs/help/update.ui \
	dialogs/messages/messages_panel.ui \
	dialogs/open/open.ui \
	dialogs/open/open_capture.ui \
	dialogs/open/open_disk.ui \
	dialogs/open/open_file.ui \
	dialogs/open/open_net.ui \
	dialogs/podcast/podcast_configuration.ui \
	dialogs/preferences/sprefs_audio.ui \
	dialogs/preferences/sprefs_input.ui \
	dialogs/preferences/sprefs_interface.ui \
	dialogs/preferences/sprefs_medialibrary.ui \
	dialogs/preferences/sprefs_subtitles.ui \
	dialogs/preferences/sprefs_video.ui \
	dialogs/sout/profiles.ui \
	dialogs/sout/sout.ui \
	dialogs/vlm/vlm.ui
EXTRA_DIST += $(libqt_plugin_la_UI)

ui_%.h: %.ui
	$(AM_V_at)$(MKDIR_P) "ui"
	$(AM_V_at)rm -f $@ $@.tmp
	$(AM_V_at)echo "#define Q_(a,b) QString::fromUtf8(_(a))" > $@.tmp
	$(uic_verbose)$(UIC) -tr "Q_" $< >> $@.tmp
	$(AM_V_at)sed -e 's/Q_(\"_(\\\"\(.*\)\\\")"/Q_("\1"/' $@.tmp >$@
	$(AM_V_at)rm -f $@.tmp

# Resources

libqt_plugin_la_RES = \
	pixmaps/juxtopposed/ATTRIBUTION.md \
	pixmaps/juxtopposed/library-settings-f12f7.svg \
	pixmaps/juxtopposed/subtitles-settings-1ab84.svg \
	pixmaps/juxtopposed/subtitles-settings-1f9fe.svg \
	pixmaps/juxtopposed/subtitles-settings-4d76e.svg \
	pixmaps/juxtopposed/subtitles-settings-764c9.svg \
	pixmaps/juxtopposed/subtitles-settings-8ba8e.svg \
	pixmaps/juxtopposed/subtitles-settings-97863.svg \
	pixmaps/juxtopposed/audio-settings-1ab84.svg \
	pixmaps/juxtopposed/audio-settings-1f9fe.svg \
	pixmaps/juxtopposed/audio-settings-5f3f8.svg \
	pixmaps/juxtopposed/audio-settings-764c9.svg \
	pixmaps/juxtopposed/audio-settings-8ba8e.svg \
	pixmaps/juxtopposed/audio-settings-97863.svg \
	pixmaps/juxtopposed/video-settings-1ab84.svg \
	pixmaps/juxtopposed/video-settings-1f9fe.svg \
	pixmaps/juxtopposed/video-settings-764c9.svg \
	pixmaps/juxtopposed/video-settings-7aced.svg \
	pixmaps/juxtopposed/video-settings-8ba8e.svg \
	pixmaps/juxtopposed/video-settings-97863.svg \
	pixmaps/juxtopposed/playback-settings-1ab84.svg \
	pixmaps/juxtopposed/playback-settings-1f9fe.svg \
	pixmaps/juxtopposed/playback-settings-764c9.svg \
	pixmaps/juxtopposed/playback-settings-8ba8e.svg \
	pixmaps/juxtopposed/playback-settings-97863.svg \
	pixmaps/juxtopposed/playback-settings-bc944.svg \
	pixmaps/juxtopposed/interface-settings-1ab84.svg \
	pixmaps/juxtopposed/interface-settings-8ba8e.svg \
	pixmaps/juxtopposed/volume-background-light.svg \
	pixmaps/juxtopposed/volume-fill-light.svg \
	pixmaps/juxtopposed/global-settings-05bc9.svg \
	pixmaps/juxtopposed/global-settings-08f95.svg \
	pixmaps/juxtopposed/global-settings-1b0f7.svg \
	pixmaps/juxtopposed/global-settings-1bf8a.svg \
	pixmaps/juxtopposed/global-settings-1f9fe.svg \
	pixmaps/juxtopposed/global-settings-36bfa.svg \
	pixmaps/juxtopposed/global-settings-4dae7.svg \
	pixmaps/juxtopposed/global-settings-66cd9.svg \
	pixmaps/juxtopposed/global-settings-6db55.svg \
	pixmaps/juxtopposed/global-settings-764c9.svg \
	pixmaps/juxtopposed/global-settings-7d179.svg \
	pixmaps/juxtopposed/global-settings-80d5a.svg \
	pixmaps/juxtopposed/global-settings-97863.svg \
	pixmaps/juxtopposed/global-settings-ad68b.svg \
	pixmaps/juxtopposed/global-settings-cf6e1.svg \
	pixmaps/juxtopposed/global-settings-ec3f6.svg \
	pixmaps/juxtopposed/global-settings-f1a4a.svg \
	pixmaps/juxtopposed/audio.svg \
	pixmaps/juxtopposed/audio-light.svg \
	pixmaps/juxtopposed/fullscreen.svg \
	pixmaps/juxtopposed/fullscreen-light.svg \
	pixmaps/juxtopposed/Inter-LICENSE.txt \
	pixmaps/juxtopposed/InterVariable.ttf \
	pixmaps/juxtopposed/next.svg \
	pixmaps/juxtopposed/pause.svg \
	pixmaps/juxtopposed/play.svg \
	pixmaps/juxtopposed/previous.svg \
	pixmaps/juxtopposed/queue.svg \
	pixmaps/juxtopposed/queue-light.svg \
	pixmaps/juxtopposed/queue-add.svg \
	pixmaps/juxtopposed/queue-bookmarks.svg \
	pixmaps/juxtopposed/queue-bookmarks-active.svg \
	pixmaps/juxtopposed/queue-chapters.svg \
	pixmaps/juxtopposed/queue-chapters-active.svg \
	pixmaps/juxtopposed/queue-move.svg \
	pixmaps/juxtopposed/queue-remove.svg \
	pixmaps/juxtopposed/queue-repeat.svg \
	pixmaps/juxtopposed/queue-shuffle.svg \
	pixmaps/juxtopposed/queue-sort.svg \
	pixmaps/juxtopposed/queue-tab-active.svg \
	pixmaps/juxtopposed/queue-tab-inactive.svg \
	pixmaps/juxtopposed/new-playlist-add.svg \
	pixmaps/juxtopposed/new-playlist-close.svg \
	pixmaps/juxtopposed/new-playlist-divider.svg \
	pixmaps/juxtopposed/settings.svg \
	pixmaps/juxtopposed/subtitles.svg \
	pixmaps/juxtopposed/subtitles-light.svg \
	pixmaps/juxtopposed/timeline-chevron-left.svg \
	pixmaps/juxtopposed/timeline-chevron-right.svg \
	pixmaps/juxtopposed/timeline-minus.svg \
	pixmaps/juxtopposed/timeline-plus.svg \
	pixmaps/juxtopposed/volume-background.svg \
	pixmaps/juxtopposed/volume-fill.svg \
	pixmaps/juxtopposed/volume-mask.svg \
	pixmaps/juxtopposed/volume-muted.svg \
	pixmaps/juxtopposed/volume.svg \
	pixmaps/juxtopposed/volume-light.svg \
	pixmaps/VLCIcons.ttf \
	pixmaps/addons/addon_default.svg \
	pixmaps/addons/addon_broken.svg \
	pixmaps/addons/addon_blue.svg \
	pixmaps/addons/addon_cyan.svg \
	pixmaps/addons/addon_green.svg \
	pixmaps/addons/addon_magenta.svg \
	pixmaps/addons/addon_red.svg \
	pixmaps/addons/addon_yellow.svg \
	pixmaps/addons/addon_score.svg \
	pixmaps/icons/active_indicator.svg \
	pixmaps/icons/add.svg \
	pixmaps/icons/ic_album_cover.svg \
	pixmaps/icons/ic_aspect_ratio.svg \
	pixmaps/icons/atob.svg \
	pixmaps/icons/ic_audiosub.svg \
	pixmaps/icons/back.svg \
	pixmaps/icons/ic_fluent_bookmark.svg \
	pixmaps/icons/breadcrumb_prev.svg \
	pixmaps/icons/breadcrumb_sep.svg \
	pixmaps/icons/check.svg \
	pixmaps/icons/clear.svg \
	pixmaps/icons/close.svg \
	pixmaps/icons/ic_fluent_full_screen_minimize.svg \
	pixmaps/icons/del.svg \
	pixmaps/icons/download.svg \
	pixmaps/icons/dropzone.svg \
	pixmaps/icons/dvd_menu.svg \
	pixmaps/icons/dvd_next.svg \
	pixmaps/icons/dvd_prev.svg \
	pixmaps/icons/ic_fluent_arrow_eject.svg \
	pixmaps/icons/ellipsis.svg \
	pixmaps/icons/ic_playlist_add.svg \
	pixmaps/icons/expand_inverted.svg \
	pixmaps/icons/expand.svg \
	pixmaps/icons/ic_fluent_options.svg \
	pixmaps/icons/ic_fluent_fast_forward.svg \
	pixmaps/icons/ic_fluent_next_frame.svg \
	pixmaps/icons/ic_fluent_previous_frame.svg \
	pixmaps/icons/ic_fluent_full_screen_maximize.svg \
	pixmaps/icons/ic_fluent_grid.svg \
	pixmaps/icons/history.svg \
	pixmaps/icons/ic_fluent_home.svg \
	pixmaps/icons/ic_fluent_info.svg \
	pixmaps/icons/ic_fluent_apps_list.svg \
	pixmaps/icons/ic_fluent_more_vertical.svg \
	pixmaps/icons/multiselect.svg \
	pixmaps/icons/ok.svg \
	pixmaps/icons/ic_fluent_next.svg \
	pixmaps/icons/ic_pause_filled.svg \
	pixmaps/icons/pip.svg \
	pixmaps/icons/ic_playlist_clear.svg \
	pixmaps/icons/ic_playlist.svg \
	pixmaps/icons/ic_fluent_play.svg \
	pixmaps/icons/ic_fluent_play_reverse_filled.svg \
	pixmaps/icons/ic_fluent_arrow_move.svg \
	pixmaps/icons/ic_fluent_chevron_down_24.svg \
	pixmaps/icons/ic_fluent_chevron_left_24.svg \
	pixmaps/icons/ic_fluent_chevron_right_24.svg \
	pixmaps/icons/ic_fluent_chevron_up_24.svg \
	pixmaps/icons/ic_fluent_play_filled.svg \
	pixmaps/icons/podcast.svg \
	pixmaps/icons/ic_fluent_previous.svg \
	pixmaps/icons/profile_new.svg \
	pixmaps/icons/ic_fluent_record.svg \
	pixmaps/icons/remove.svg \
	pixmaps/icons/ic_fluent_cast.svg \
	pixmaps/icons/ic_fluent_arrow_repeat_all.svg \
	pixmaps/icons/ic_fluent_arrow_repeat_1.svg \
	pixmaps/icons/ic_fluent_rewind.svg \
	pixmaps/icons/ic_fluent_search.svg \
	pixmaps/icons/ic_fluent_arrow_shuffle.svg \
	pixmaps/icons/ic_fluent_skip_back_10.svg \
	pixmaps/icons/ic_fluent_skip_forward_10.svg \
	pixmaps/icons/ic_fluent_camera.svg \
	pixmaps/icons/ic_fluent_spacebar.svg \
	pixmaps/icons/ic_fluent_stop.svg \
	pixmaps/icons/stream.svg \
	pixmaps/icons/sync.svg \
	pixmaps/icons/ic_fluent_clock.svg \
	pixmaps/icons/ic_fluent_globe.svg \
	pixmaps/icons/topbar_filter.svg \
	pixmaps/icons/ic_fluent_music_note_2.svg \
	pixmaps/icons/ic_fluent_wifi_1.svg \
	pixmaps/icons/ic_fluent_arrow_sort.svg \
	pixmaps/icons/ic_fluent_filmstrip.svg \
	pixmaps/icons/ic_fluent_chevron_up.svg \
	pixmaps/icons/ic_fluent_chevron_down.svg \
	pixmaps/icons/ic_fluent_tv.svg \
	pixmaps/icons/ic_tvtelx.svg \
	pixmaps/icons/visualization.svg \
	pixmaps/icons/volume_high.svg \
	pixmaps/icons/volume_low.svg \
	pixmaps/icons/volume_medium.svg \
	pixmaps/icons/volume_muted.svg \
	pixmaps/icons/volume_zero.svg \
	pixmaps/icons/window_close.svg \
	pixmaps/icons/window_maximize.svg \
	pixmaps/icons/window_minimize.svg \
	pixmaps/icons/window_restore.svg \
	pixmaps/icons/ic_fluent_arrow_download_24_regular.svg \
	pixmaps/icons/ic_fluent_arrow_sync_24_regular.svg \
	pixmaps/icons/regex.svg \
	pixmaps/icons/ic_fluent_text_change_case_24_regular.svg \
	pixmaps/menu/add.svg \
	pixmaps/menu/capture-card.svg \
	pixmaps/menu/clear.svg \
	pixmaps/menu/disc.svg \
	pixmaps/menu/download.svg \
	pixmaps/menu/ic_fluent_arrow_eject.svg \
	pixmaps/menu/exit.svg \
	pixmaps/menu/ic_fluent_fast_forward.svg \
	pixmaps/menu/file.svg \
	pixmaps/menu/folder.svg \
	pixmaps/menu/help.svg \
	pixmaps/menu/info.svg \
	pixmaps/menu/media_library.svg \
	pixmaps/menu/messages.svg \
	pixmaps/menu/movie.svg \
	pixmaps/menu/music.svg \
	pixmaps/menu/network.svg \
	pixmaps/menu/ic_playlist.svg \
	pixmaps/menu/ic_fluent_next.svg \
	pixmaps/menu/ic_pause_filled.svg \
	pixmaps/menu/ic_fluent_play_filled.svg \
	pixmaps/menu/preferences.svg \
	pixmaps/menu/ic_fluent_previous.svg \
	pixmaps/menu/profile_new.svg \
	pixmaps/menu/ic_fluent_record.svg \
	pixmaps/menu/remove.svg \
	pixmaps/menu/ic_fluent_arrow_repeat_all.svg \
	pixmaps/menu/repeat_off.svg \
	pixmaps/menu/ic_fluent_arrow_repeat_1.svg \
	pixmaps/menu/ic_fluent_rewind.svg \
	pixmaps/menu/search_clear.svg \
	pixmaps/menu/ic_fluent_options.svg \
	pixmaps/menu/ic_fluent_skip_back_10.svg \
	pixmaps/menu/ic_fluent_skip_forward_10.svg \
	pixmaps/menu/ic_fluent_arrow_shuffle.svg \
	pixmaps/menu/ic_fluent_stop.svg \
	pixmaps/menu/stream.svg \
	pixmaps/menu/sync.svg \
	pixmaps/menu/update.svg \
	pixmaps/menu/volume-high.svg \
	pixmaps/menu/volume-low.svg \
	pixmaps/menu/volume-muted.svg \
	pixmaps/misc/cone.svg \
	pixmaps/misc/new_indicator.svg \
	pixmaps/misc/theme_dark.svg \
	pixmaps/misc/theme_daynight.svg \
	pixmaps/misc/theme_light.svg \
	pixmaps/misc/wait1.svg \
	pixmaps/misc/wait2.svg \
	pixmaps/misc/wait3.svg \
	pixmaps/misc/wait4.svg \
	pixmaps/placeholder/add_service.svg \
	pixmaps/placeholder/noart.png \
	pixmaps/placeholder/noart_album.svg \
	pixmaps/placeholder/noart_albumCover.svg \
	pixmaps/placeholder/noart_artist.svg \
	pixmaps/placeholder/noart_artistCover.svg \
	pixmaps/placeholder/noart_artist_small.svg \
	pixmaps/placeholder/noart_videoCover.svg \
	pixmaps/prefs/advprefs_audio.svg \
	pixmaps/prefs/advprefs_codec.svg \
	pixmaps/prefs/advprefs_extended.svg \
	pixmaps/prefs/advprefs_intf.svg \
	pixmaps/prefs/advprefs_playlist.svg \
	pixmaps/prefs/advprefs_sout.svg \
	pixmaps/prefs/advprefs_video.svg \
	pixmaps/prefs/sample_complete.png \
	pixmaps/prefs/sample_minimal.png \
	pixmaps/prefs/sample_skins.png \
	pixmaps/prefs/spref_audio.png \
	pixmaps/prefs/spref_hotkeys.png \
	pixmaps/prefs/spref_input.png \
	pixmaps/prefs/spref_interface.png \
	pixmaps/prefs/spref_medialibrary.png \
	pixmaps/prefs/spref_subtitles.png \
	pixmaps/prefs/spref_video.png \
	pixmaps/sd/assembleenationale.svg \
	pixmaps/sd/capture-card.svg \
	pixmaps/sd/directory.svg \
	pixmaps/sd/disc.svg \
	pixmaps/sd/file.svg \
	pixmaps/sd/fmc.svg \
	pixmaps/sd/frenchtv.svg \
	pixmaps/sd/harddisk.svg \
	pixmaps/sd/icecast.svg \
	pixmaps/sd/jamendo.svg \
	pixmaps/sd/katsomo.svg \
	pixmaps/sd/lan.svg \
	pixmaps/sd/library.svg \
	pixmaps/sd/metachannels.svg \
	pixmaps/sd/mtp.svg \
	pixmaps/sd/network.svg \
	pixmaps/sd/node.svg \
	pixmaps/sd/pictures.svg \
	pixmaps/sd/playlist.svg \
	pixmaps/sd/podcast.svg \
	pixmaps/sd/radio.svg \
	pixmaps/sd/screen.svg \
	pixmaps/sd/stream.svg \
	pixmaps/sd/unknown.svg

if HAVE_WIN32
libqt_plugin_la_RES += \
	pixmaps/win7/pause.svg \
	pixmaps/win7/prev.svg \
	pixmaps/win7/next.svg \
	pixmaps/win7/play.svg
endif

# QML modules
#
# Each QML module has a small section below. To add a new module "foo", append
# a section following this template:
#
#   libqml_module_foo_a_Module     = VLC.Foo
#   libqml_module_foo_a_ModulePath = VLC/Foo
#   libqml_module_foo_a_CXXFLAGS   = $(libqt_plugin_la_CXXFLAGS)
#   libqml_module_foo_a_CPPFLAGS   = $(libqt_plugin_la_CPPFLAGS)
#   libqml_module_foo_a_SOURCES    = $(QML_MODULE_foo_SRC)
#   libqml_module_foo_a_QML 			 = foo/qml/Bar.qml # ...
#   libqml_module_foo_a_Singleton_QML  = foo/qml/SingletonBar.qml # only when there are Singletons
#   nodist_libqml_module_foo_a_SOURCES = foo/qmlassets.cpp $(QML_MODULE_foo_NODIST_EXTRA)
#   QML_MODULE_LIBS += libqml_module_foo.a
#
# When exposing singletons, list those QML files in the proper variable so that
# they are declared as singletons in the generated qmldir.
#
# Then:
#   1. Add "foo" to libqt_plugin_la_QML_MODULES.
#   2. Add the QML file lists to libqt_plugin_la_ALL_QML.
#   3. Add a QML_MODULE_foo_SRC / QML_MODULE_foo_NODIST_EXTRA pair to the
#      "if HAVE_QMLCACHEGEN" block below.
#
noinst_LIBRARIES =
QML_MODULE_LIBS =

libqml_module_dialogs_a_Module = VLC.Dialogs
libqml_module_dialogs_a_ModulePath = VLC/Dialogs
libqml_module_dialogs_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_dialogs_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_dialogs_a_QML = \
	dialogs/dialogs/qml/CustomDialog.qml \
	dialogs/dialogs/qml/JuxtopposedNetworkDialog.qml \
	dialogs/dialogs/qml/JuxtopposedDialogsHost.qml \
	dialogs/dialogs/qml/JuxtopposedGlobalSettings.qml \
	dialogs/dialogs/qml/JuxtopposedInterfaceSettings.qml \
	dialogs/dialogs/qml/JuxtopposedPlaybackSettings.qml \
	dialogs/dialogs/qml/JuxtopposedAudioSettings.qml \
	dialogs/dialogs/qml/JuxtopposedSubtitlesSettings.qml \
	dialogs/dialogs/qml/JuxtopposedLibrarySettings.qml \
	dialogs/dialogs/qml/JuxtopposedVideoSettings.qml \
	dialogs/dialogs/qml/JuxtopposedSettingsBackdrop.qml \
	dialogs/dialogs/qml/JuxtopposedSettingsBackdropEffect.qml \
	dialogs/dialogs/qml/JuxtopposedSettingsBackdropShadow.qml \
	dialogs/dialogs/qml/JuxtopposedGeneralSettings.qml \
	dialogs/dialogs/qml/JuxtopposedNewPlaylistDialog.qml \
	dialogs/dialogs/qml/JuxtopposedConversionDialog.qml \
	dialogs/dialogs/qml/Dialogs.qml \
	dialogs/dialogs/qml/ModalDialog.qml \
	dialogs/dialogs/qml/WindowDialog.qml \
	dialogs/toolbar/qml/EditorDNDDelegate.qml \
	dialogs/toolbar/qml/EditorDNDView.qml\
	dialogs/toolbar/qml/EditorDummyButton.qml \
	dialogs/toolbar/qml/EditorTabButton.qml \
	dialogs/toolbar/qml/ToolbarEditor.qml \
	dialogs/toolbar/qml/ToolbarEditorDialog.qml \
	dialogs/toolbar/qml/ToolbarEditorButtonList.qml
libqml_module_dialogs_a_Singleton_QML =
libqml_module_dialogs_a_SOURCES = $(QML_MODULE_dialogs_SRC)
nodist_libqml_module_dialogs_a_SOURCES = dialogs/qmlassets.cpp $(QML_MODULE_dialogs_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_dialogs.a

libqml_module_maininterface_a_Module = VLC.MainInterface
libqml_module_maininterface_a_ModulePath = VLC/MainInterface
libqml_module_maininterface_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_maininterface_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_maininterface_a_QML = \
	maininterface/qml/MenuTopbar.qml \
	maininterface/qml/LocalTopbar.qml \
	maininterface/qml/MainInterface.qml \
	maininterface/qml/MainViewLoader.qml \
	maininterface/qml/MainDisplay.qml \
	maininterface/qml/JuxtopposedNavigation.qml \
	maininterface/qml/SideNavigationPane.qml \
	maininterface/qml/SideNavigationDelegate.qml \
	maininterface/qml/NoMedialibHome.qml

if UPDATE_CHECK
libqml_module_maininterface_a_QML += maininterface/qml/UpdatePane.qml
endif

libqml_module_maininterface_a_Singleton_QML =
libqml_module_maininterface_a_SOURCES = $(QML_MODULE_maininterface_SRC)
nodist_libqml_module_maininterface_a_SOURCES = maininterface/qmlassets.cpp $(QML_MODULE_maininterface_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_maininterface.a

libqml_module_medialibrary_a_Module = VLC.MediaLibrary
libqml_module_medialibrary_a_ModulePath = VLC/MediaLibrary
libqml_module_medialibrary_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_medialibrary_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_medialibrary_a_QML = \
	medialibrary/qml/ArtistTopBanner.qml \
	medialibrary/qml/AudioGridItem.qml \
	medialibrary/qml/HomeDisplay.qml \
	medialibrary/qml/HomePage.qml \
	medialibrary/qml/JuxtopposedHomeCard.qml \
	medialibrary/qml/JuxtopposedHomeDashboard.qml \
	medialibrary/qml/JuxtopposedHomeCollectionView.qml \
	medialibrary/qml/JuxtopposedHomePage.qml \
	medialibrary/qml/JuxtopposedHomeHistoryPage.qml \
	medialibrary/qml/JuxtopposedHomeCollectionPage.qml \
	medialibrary/qml/JuxtopposedHomeFolderPage.qml \
	medialibrary/qml/JuxtopposedHomeSorting.js \
	medialibrary/qml/MediaView.qml \
	medialibrary/qml/VideoGroupDisplay.qml \
	medialibrary/qml/MusicAlbums.qml \
	medialibrary/qml/MusicAlbumsDisplay.qml \
	medialibrary/qml/MusicAlbumsGridExpandDelegate.qml \
	medialibrary/qml/MusicArtist.qml \
	medialibrary/qml/MusicArtistDelegate.qml \
	medialibrary/qml/MusicArtistsAlbums.qml \
	medialibrary/qml/MusicAllArtists.qml \
	medialibrary/qml/MusicArtistsDisplay.qml \
	medialibrary/qml/MusicDisplay.qml \
	medialibrary/qml/MusicCategoriesDisplay.qml \
	medialibrary/qml/MusicGenres.qml \
	medialibrary/qml/MusicGenresDisplay.qml \
	medialibrary/qml/MusicPlaylistsDisplay.qml \
	medialibrary/qml/MusicTrackListDisplay.qml \
	medialibrary/qml/MusicTracksDisplay.qml \
	medialibrary/qml/UrlListDisplay.qml \
	medialibrary/qml/VideoDisplay.qml \
	medialibrary/qml/VideoAll.qml \
	medialibrary/qml/VideoAllDisplay.qml \
	medialibrary/qml/VideoAllSubDisplay.qml \
	medialibrary/qml/PlaylistMediaList.qml \
	medialibrary/qml/PlaylistMedia.qml \
	medialibrary/qml/PlaylistMediaDisplay.qml \
	medialibrary/qml/VideoPlaylistsDisplay.qml \
	medialibrary/qml/JuxtopposedPlaylistsDisplay.qml \
	medialibrary/qml/VideoGridItem.qml \
	medialibrary/qml/VideoInfoExpandPanel.qml \
	medialibrary/qml/VideoListDisplay.qml \
	medialibrary/qml/VideoGridDisplay.qml \
	medialibrary/qml/JuxtopposedVideoGrid.qml \
	medialibrary/qml/MusicAlbumSectionDelegate.qml \
	medialibrary/qml/MLGlobalShortcuts.qml \
	medialibrary/qml/Bookmarks.qml \
	medialibrary/qml/MLContextMenu.qml \
	medialibrary/qml/MLDragItem.qml \
	medialibrary/qml/MLTableColumns.qml \
	medialibrary/qml/ScanProgressBar.qml
libqml_module_medialibrary_a_Singleton_QML =
libqml_module_medialibrary_a_SOURCES = $(QML_MODULE_medialibrary_SRC)
nodist_libqml_module_medialibrary_a_SOURCES = medialibrary/qmlassets.cpp $(QML_MODULE_medialibrary_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_medialibrary.a

libqml_module_menus_a_Module = VLC.Menus
libqml_module_menus_a_ModulePath = VLC/Menus
libqml_module_menus_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_menus_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_menus_a_QML = \
	menus/qml/GlobalShortcuts.qml \
	menus/qml/Menubar.qml \
	menus/qml/ShortcutExt.qml
libqml_module_menus_a_Singleton_QML =
libqml_module_menus_a_SOURCES = $(QML_MODULE_menus_SRC)
nodist_libqml_module_menus_a_SOURCES = menus/qmlassets.cpp $(QML_MODULE_menus_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_menus.a

libqml_module_network_a_Module = VLC.Network
libqml_module_network_a_ModulePath = VLC/Network
libqml_module_network_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_network_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_network_a_QML = \
	network/qml/AddressbarButton.qml \
	network/qml/BrowseDeviceView.qml \
	network/qml/BrowseDisplay.qml \
	network/qml/BrowseHomeDisplay.qml \
	network/qml/BrowseTreeDisplay.qml \
	network/qml/BrowseTreeHeader.qml \
	network/qml/DiscoverDisplay.qml \
	network/qml/DiscoverUrlDisplay.qml \
	network/qml/NetworkAddressbar.qml \
	network/qml/NetworkGridItem.qml \
	network/qml/NetworkThumbnailItem.qml \
	network/qml/ServicesHomeDisplay.qml \
	network/qml/ServicesManage.qml \
	network/qml/ServicesSources.qml
libqml_module_network_a_Singleton_QML =
libqml_module_network_a_SOURCES = $(QML_MODULE_network_SRC)
nodist_libqml_module_network_a_SOURCES = network/qmlassets.cpp $(QML_MODULE_network_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_network.a

libqml_module_player_a_Module = VLC.Player
libqml_module_player_a_ModulePath = VLC/Player
libqml_module_player_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_player_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_player_a_QML = \
	player/qml/JuxtopposedButton.qml \
	player/qml/JuxtopposedSeekBackground.qml \
	player/qml/JuxtopposedPlaybackBar.qml \
	player/qml/JuxtopposedSubtitleCueTimeline.qml \
	player/qml/JuxtopposedSubtitleTimelineAdapter.qml \
	player/qml/JuxtopposedSubtitleTimelineToggle.qml \
	player/qml/JuxtopposedMusicPlayer.qml \
	player/qml/PlaybackSettingsMenu.qml \
	player/qml/JuxtopposedSettingsPanel.qml \
	player/qml/JuxtopposedSubtitlePresets.qml \
	player/qml/ControlBar.qml \
	player/qml/PlaybackSpeed.qml \
	player/qml/LyricsFlickable.qml \
	player/qml/MiniPlayer.qml \
	player/qml/PIPPlayer.qml \
	player/qml/Player.qml \
	player/qml/PlayerControlLayout.qml \
	player/qml/PlayerPlaylistVisibilityFSM.qml \
	player/qml/PlayerToolbarVisibilityFSM.qml \
	player/qml/MinimalView.qml \
	player/qml/ResumeDialog.qml \
	player/qml/SliderBar.qml \
	player/qml/TopBar.qml \
	player/qml/TracksMenu.qml \
	player/qml/TracksPage.qml \
	player/qml/TracksPageAudio.qml \
	player/qml/TracksPageSpeed.qml \
	player/qml/TracksPageSubtitle.qml \
	player/qml/TracksListPage.qml \
	player/qml/ControlLayout.qml \
	player/qml/ControlRepeater.qml \
	player/qml/NavigationBox.qml

libqml_module_player_a_Singleton_QML = \
	player/qml/PlayerControlbarControls.qml
libqml_module_player_a_SOURCES = $(QML_MODULE_player_SRC)
nodist_libqml_module_player_a_SOURCES = player/qmlassets.cpp $(QML_MODULE_player_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_player.a

libqml_module_playercontrols_a_Module = VLC.PlayerControls
libqml_module_playercontrols_a_ModulePath = VLC/PlayerControls
libqml_module_playercontrols_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_playercontrols_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_playercontrols_a_QML = \
	player/qml/controlbarcontrols/HighResolutionTimeWidget.qml \
	player/qml/controlbarcontrols/ArtworkInfoWidget.qml \
	player/qml/controlbarcontrols/AspectRatioWidget.qml \
	player/qml/controlbarcontrols/AtoBButton.qml \
	player/qml/controlbarcontrols/BackButton.qml \
	player/qml/controlbarcontrols/BookmarkButton.qml \
	player/qml/controlbarcontrols/ChapterNextButton.qml \
	player/qml/controlbarcontrols/ChapterPreviousButton.qml \
	player/qml/controlbarcontrols/DvdMenuButton.qml \
	player/qml/controlbarcontrols/ExpandingSpacerWidget.qml \
	player/qml/controlbarcontrols/ExtendedSettingsButton.qml \
	player/qml/controlbarcontrols/FasterButton.qml \
	player/qml/controlbarcontrols/FrameNextButton.qml \
	player/qml/controlbarcontrols/FramePrevButton.qml \
	player/qml/controlbarcontrols/FullscreenButton.qml \
	player/qml/controlbarcontrols/InfoButton.qml \
	player/qml/controlbarcontrols/LangButton.qml \
	player/qml/controlbarcontrols/LoopButton.qml \
	player/qml/controlbarcontrols/MenuButton.qml \
	player/qml/controlbarcontrols/NavigationBoxButton.qml \
	player/qml/controlbarcontrols/NavigationWidget.qml \
	player/qml/controlbarcontrols/NextButton.qml \
	player/qml/controlbarcontrols/OpenButton.qml \
	player/qml/controlbarcontrols/PlaybackSpeedButton.qml \
	player/qml/controlbarcontrols/PlayButton.qml \
	player/qml/controlbarcontrols/PlayerSwitchButton.qml \
	player/qml/controlbarcontrols/PlaylistButton.qml \
	player/qml/controlbarcontrols/PreviousButton.qml \
	player/qml/controlbarcontrols/ProgramButton.qml \
	player/qml/controlbarcontrols/QuitButton.qml \
	player/qml/controlbarcontrols/RandomButton.qml \
	player/qml/controlbarcontrols/RendererButton.qml \
	player/qml/controlbarcontrols/RecordButton.qml \
	player/qml/controlbarcontrols/ReverseButton.qml \
	player/qml/controlbarcontrols/SkipBackButton.qml \
	player/qml/controlbarcontrols/SkipForwardButton.qml \
	player/qml/controlbarcontrols/SlowerButton.qml \
	player/qml/controlbarcontrols/SnapshotButton.qml \
	player/qml/controlbarcontrols/SpacerWidget.qml \
	player/qml/controlbarcontrols/StopButton.qml \
	player/qml/controlbarcontrols/TeletextButton.qml \
	player/qml/controlbarcontrols/TeletextColorButton.qml \
	player/qml/controlbarcontrols/TeletextWidget.qml \
	player/qml/controlbarcontrols/VolumeWidget.qml \
	player/qml/controlbarcontrols/Fallback.qml
libqml_module_playercontrols_a_Singleton_QML =
libqml_module_playercontrols_a_SOURCES = $(QML_MODULE_playercontrols_SRC)
nodist_libqml_module_playercontrols_a_SOURCES = playercontrols/qmlassets.cpp $(QML_MODULE_playercontrols_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_playercontrols.a

libqml_module_playlist_a_Module = VLC.Playlist
libqml_module_playlist_a_ModulePath = VLC/Playlist
libqml_module_playlist_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_playlist_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_playlist_a_QML = \
	playlist/qml/JuxtopposedBookmarkList.qml \
	playlist/qml/JuxtopposedQueuePanel.qml \
	playlist/qml/JuxtopposedQueueRow.qml \
	playlist/qml/PlaylistDelegate.qml \
	playlist/qml/PlaylistPane.qml \
	playlist/qml/PlaylistDetachedWindow.qml
libqml_module_playlist_a_Singleton_QML =
libqml_module_playlist_a_SOURCES = $(QML_MODULE_playlist_SRC)
nodist_libqml_module_playlist_a_SOURCES = playlist/qmlassets.cpp $(QML_MODULE_playlist_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_playlist.a

libqml_module_style_a_Module = VLC.Style
libqml_module_style_a_ModulePath = VLC/Style
libqml_module_style_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_style_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_style_a_QML =
libqml_module_style_a_Singleton_QML = \
	style/AcrylicController.qml \
	style/JuxtopposedPalette.qml \
	style/VLCStyle.qml
libqml_module_style_a_SOURCES = $(QML_MODULE_style_SRC)
nodist_libqml_module_style_a_SOURCES = style/qmlassets.cpp $(QML_MODULE_style_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_style.a

libqml_module_util_a_Module = VLC.Util
libqml_module_util_a_ModulePath = VLC/Util
libqml_module_util_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_util_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_util_a_QML = \
	util/qml/ModelSortSettingHandler.qml \
	util/qml/DefaultFlickableScrollHandler.qml \
	util/qml/ViewDragAutoScrollHandler.qml \
	util/qml/FSM.qml \
	util/qml/FSMState.qml \
	util/qml/NativeMenu.qml \
	util/qml/FadeControllerStateGroup.qml \
	util/qml/GridSizeHelper.qml \
	util/qml/DelayedBehavior.qml \
	util/qml/DelegateTouchTapHandler.qml
libqml_module_util_a_Singleton_QML = \
	util/qml/Helpers.qml
libqml_module_util_a_SOURCES = $(QML_MODULE_util_SRC)
nodist_libqml_module_util_a_SOURCES = util/qmlassets.cpp $(QML_MODULE_util_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_util.a

libqml_module_widgets_a_Module = VLC.Widgets
libqml_module_widgets_a_ModulePath = VLC/Widgets
libqml_module_widgets_a_CXXFLAGS = $(libqt_plugin_la_CXXFLAGS)
libqml_module_widgets_a_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
libqml_module_widgets_a_QML = \
	widgets/qml/LabelExt.qml \
	widgets/qml/ActionButtonOverlay.qml \
	widgets/qml/ActionButtonPrimary.qml \
	widgets/qml/BannerTabButton.qml \
	widgets/qml/BannerCone.qml \
	widgets/qml/BusyIndicatorExt.qml \
	widgets/qml/CaptionLabel.qml \
	widgets/qml/CheckButton.qml \
	widgets/qml/CheckedDelegate.qml \
	widgets/qml/ComboBoxExt.qml \
	widgets/qml/AcrylicBackground.qml \
	widgets/qml/FloatingNotification.qml \
	widgets/qml/AnimatedBackground.qml \
	widgets/qml/CSDThemeButton.qml \
	widgets/qml/CSDThemeButtonSet.qml \
	widgets/qml/CSDWindowButton.qml \
	widgets/qml/CSDWindowButtonSet.qml \
	widgets/qml/CSDTitlebarTapNDrapHandler.qml \
	widgets/qml/CSDMouseStealer.qml \
	widgets/qml/CurrentIndicator.qml \
	widgets/qml/DragItem.qml \
	widgets/qml/DrawerExt.qml \
	widgets/qml/ExpandGridView.qml \
	widgets/qml/ExpandGridItemView.qml \
	widgets/qml/EmptyLabel.qml \
	widgets/qml/EmptyLabelButton.qml \
	widgets/qml/EmptyLabelHint.qml \
	widgets/qml/GridItem.qml \
	widgets/qml/GridSortFilterControls.qml \
	widgets/qml/HorizontalResizeHandle.qml \
	widgets/qml/IconLabel.qml \
	widgets/qml/IconButton.qml \
	widgets/qml/IconToolButton.qml \
	widgets/qml/IconTrackButton.qml \
	widgets/qml/ImageToolButton.qml \
	widgets/qml/ListViewExt.qml \
	widgets/qml/TableViewExt.qml \
	widgets/qml/TableViewDelegateExt.qml \
	widgets/qml/TableRowDelegate.qml \
	widgets/qml/TableHeaderDelegate.qml \
	widgets/qml/DefaultShadow.qml \
	widgets/qml/ListLabel.qml \
	widgets/qml/MediaCover.qml \
	widgets/qml/SubtitleLabel.qml \
	widgets/qml/MenuCaption.qml \
	widgets/qml/MenuLabel.qml \
	widgets/qml/NavigableCol.qml \
	widgets/qml/NavigableRow.qml \
	widgets/qml/PlayCover.qml \
	widgets/qml/RoundButtonExt.qml \
	widgets/qml/ScaledImage.qml \
	widgets/qml/TextAutoScroller.qml \
	widgets/qml/SearchBox.qml \
	widgets/qml/TextInputFocus.qml \
	widgets/qml/SliderExt.qml \
	widgets/qml/SortControl.qml \
	widgets/qml/SpinBoxExt.qml \
	widgets/qml/StackViewExt.qml \
	widgets/qml/PageLoader.qml \
	widgets/qml/PageExt.qml \
	widgets/qml/JuxtopposedLibraryHeader.qml \
	widgets/qml/JuxtopposedMediaRow.qml \
	widgets/qml/JuxtopposedCategoryList.qml \
	widgets/qml/JuxtopposedSettingSlider.qml \
	widgets/qml/JuxtopposedSettingChoice.qml \
	widgets/qml/ButtonExt.qml \
	widgets/qml/TableColumns.qml \
	widgets/qml/TextFieldExt.qml \
	widgets/qml/TextToolButton.qml \
	widgets/qml/TrackDelayButton.qml \
	widgets/qml/VideoProgressBar.qml \
	widgets/qml/VideoQualityLabels.qml \
	widgets/qml/ListSubtitleLabel.qml \
	widgets/qml/PointingTooltip.qml \
	widgets/qml/FrostedGlassEffect.qml \
	widgets/qml/ToolTipExt.qml \
	widgets/qml/DropShadowImage.qml \
	widgets/qml/DoubleShadow.qml \
	widgets/qml/FadingEdge.qml \
	widgets/qml/FadingEdgeForItemView.qml \
	widgets/qml/PopupIconToolButton.qml \
	widgets/qml/PartialEffect.qml \
	widgets/qml/ViewHeader.qml \
	widgets/qml/ProgressIndicator.qml \
	widgets/qml/ImageExt.qml \
	widgets/qml/EnhancedImageExt.qml \
	widgets/qml/ScrollBarExt.qml \
	widgets/qml/FastBlend.qml \
	widgets/qml/RadioButtonExt.qml \
	widgets/qml/RoundedRectangleShadow.qml \
	widgets/qml/VoronoiSnow.qml \
	widgets/qml/DualKawaseBlur.qml \
	widgets/qml/CheckBoxExt.qml \
	widgets/qml/ProgressBarExt.qml
libqml_module_widgets_a_Singleton_QML =
libqml_module_widgets_a_SOURCES = $(QML_MODULE_widgets_SRC)
nodist_libqml_module_widgets_a_SOURCES = widgets/qmlassets.cpp $(QML_MODULE_widgets_NODIST_EXTRA)
QML_MODULE_LIBS += libqml_module_widgets.a

if ENABLE_QT
libqt_plugin_la_LIBADD += $(QML_MODULE_LIBS)
noinst_LIBRARIES += $(QML_MODULE_LIBS)
endif

if HAVE_QMLCACHEGEN
QML_MODULE_dialogs_SRC = $(libqml_module_dialogs_a_QML) $(libqml_module_dialogs_a_Singleton_QML)
QML_MODULE_dialogs_NODIST_EXTRA = dialogs/qmlcache_loader.cpp
QML_MODULE_maininterface_SRC = $(libqml_module_maininterface_a_QML) $(libqml_module_maininterface_a_Singleton_QML)
QML_MODULE_maininterface_NODIST_EXTRA = maininterface/qmlcache_loader.cpp
QML_MODULE_medialibrary_SRC = $(libqml_module_medialibrary_a_QML) $(libqml_module_medialibrary_a_Singleton_QML)
QML_MODULE_medialibrary_NODIST_EXTRA = medialibrary/qmlcache_loader.cpp
QML_MODULE_menus_SRC = $(libqml_module_menus_a_QML) $(libqml_module_menus_a_Singleton_QML)
QML_MODULE_menus_NODIST_EXTRA = menus/qmlcache_loader.cpp
QML_MODULE_network_SRC = $(libqml_module_network_a_QML) $(libqml_module_network_a_Singleton_QML)
QML_MODULE_network_NODIST_EXTRA = network/qmlcache_loader.cpp
QML_MODULE_player_SRC = $(libqml_module_player_a_QML) $(libqml_module_player_a_Singleton_QML)
QML_MODULE_player_NODIST_EXTRA = player/qmlcache_loader.cpp
QML_MODULE_playercontrols_SRC = $(libqml_module_playercontrols_a_QML) $(libqml_module_playercontrols_a_Singleton_QML)
QML_MODULE_playercontrols_NODIST_EXTRA = playercontrols/qmlcache_loader.cpp
QML_MODULE_playlist_SRC = $(libqml_module_playlist_a_QML) $(libqml_module_playlist_a_Singleton_QML)
QML_MODULE_playlist_NODIST_EXTRA = playlist/qmlcache_loader.cpp
QML_MODULE_style_SRC = $(libqml_module_style_a_QML) $(libqml_module_style_a_Singleton_QML)
QML_MODULE_style_NODIST_EXTRA = style/qmlcache_loader.cpp
QML_MODULE_util_SRC = $(libqml_module_util_a_QML) $(libqml_module_util_a_Singleton_QML)
QML_MODULE_util_NODIST_EXTRA = util/qmlcache_loader.cpp
QML_MODULE_widgets_SRC = $(libqml_module_widgets_a_QML) $(libqml_module_widgets_a_Singleton_QML)
QML_MODULE_widgets_NODIST_EXTRA = widgets/qmlcache_loader.cpp
endif


libqt_plugin_la_QML_MODULES = \
	dialogs \
	maininterface \
	medialibrary \
	menus \
	network \
	player \
	playercontrols \
	playlist \
	style \
	util \
	widgets

libqt_plugin_la_ALL_QML = \
	$(libqml_module_dialogs_a_QML) \
	$(libqml_module_maininterface_a_QML) \
	$(libqml_module_medialibrary_a_QML) \
	$(libqml_module_menus_a_QML) \
	$(libqml_module_network_a_QML) \
	$(libqml_module_player_a_QML) \
	$(libqml_module_player_a_Singleton_QML) \
	$(libqml_module_playercontrols_a_QML) \
	$(libqml_module_playlist_a_QML) \
	$(libqml_module_style_a_QML) \
	$(libqml_module_style_a_Singleton_QML) \
	$(libqml_module_util_a_QML) \
	$(libqml_module_util_a_Singleton_QML) \
	$(libqml_module_widgets_a_QML)

# Shaders:
libqt_plugin_la_SHADER_FRAG = shaders/FadingEdge.frag \
                              shaders/Noise.frag \
                              shaders/RectFilter.frag \
                              shaders/SDFAARoundedTexture.frag \
                              shaders/SDFAARoundedTexture_cropsupport_bordersupport.frag \
                              shaders/DitheredTexture.frag \
                              shaders/FastBlend.frag \
                              shaders/FastBlend_additive.frag \
                              shaders/FastBlend_multiply.frag \
                              shaders/FastBlend_screen.frag \
                              shaders/RoundedRectangleShadow.frag \
                              shaders/RoundedRectangleShadow_hollow.frag \
                              shaders/VoronoiSnow.frag \
                              shaders/DualKawaseBlur_upsample.frag \
                              shaders/DualKawaseBlur_downsample.frag \
                              shaders/DualKawaseBlur_upsample_postprocess.frag

libqt_plugin_la_SHADER_VERT = shaders/FadingEdgeX.vert \
                              shaders/FadingEdgeY.vert \
                              shaders/SubTexture.vert

libqt_plugin_la_SHADER = $(libqt_plugin_la_SHADER_FRAG) $(libqt_plugin_la_SHADER_VERT)

if ENABLE_QT

BUILT_SOURCES += \
    $(libqt_plugin_la_SHADER_FRAG:.frag=.frag.qsb) \
    $(libqt_plugin_la_SHADER_VERT:.vert=.vert.qsb)

QSB_PARAMS = --glsl="300 es,130,150" --batchable -O
if HAVE_WIN32
QSB_PARAMS += --hlsl=50 -c
endif
if HAVE_DARWIN
QSB_PARAMS += --msl=12
endif

QSB_EXTRA_PATH =
if HAVE_FXC
QSB_EXTRA_PATH += $(FXC_PATH):
endif

qsb_verbose = $(qsb_verbose_$(V))
qsb_verbose_ = $(qsb_verbose__$(AM_DEFAULT_VERBOSITY))
qsb_verbose_0 = @echo "  QSB     " $@;
qsb_verbose__0 = $(qsb_verbose_0)

QSB_PARAMS_VERBOSE = $(QSB_PARAMS_VERBOSE_$(V))
QSB_PARAMS_VERBOSE_ = $(QSB_PARAMS_VERBOSE__$(AM_DEFAULT_VERBOSITY))
QSB_PARAMS_VERBOSE_0 = -s
QSB_PARAMS_VERBOSE__0 = $(QSB_PARAMS_VERBOSE_0)

EXTRA_DIST += shaders/Common.glsl

$(libqt_plugin_la_SHADER_FRAG:.frag=.frag.qsb) $(libqt_plugin_la_SHADER_VERT:.vert=.vert.qsb): shaders/Common.glsl

$(libqt_plugin_la_SHADER_FRAG:.frag=.frag.qsb):
	$(qsb_verbose)PATH="$(QSB_EXTRA_PATH)$$PATH" $(QSB) $(QSB_PARAMS) $(QSB_PARAMS_VERBOSE) -o $@ $(srcdir)/$(@:.qsb=)

$(libqt_plugin_la_SHADER_VERT:.vert=.vert.qsb):
	$(qsb_verbose)PATH="$(QSB_EXTRA_PATH)$$PATH" $(QSB) $(QSB_PARAMS) $(QSB_PARAMS_VERBOSE) -o $@ $(srcdir)/$(@:.qsb=)

shaders/shaders.qrc: $(libqt_plugin_la_SHADER) Makefile
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/scripts/genqrc.py -o $@ --prefix shaders --sources $(libqt_plugin_la_SHADER_FRAG:.frag=.frag.qsb) $(libqt_plugin_la_SHADER_VERT:.vert=.vert.qsb)

shaders/shaders.cpp: shaders/shaders.qrc $(libqt_plugin_la_SHADER_FRAG:.frag=.frag.qsb) $(libqt_plugin_la_SHADER_VERT:.vert=.vert.qsb)
	$(AM_V_GEN) $(RCC) $(RCC_EXTRA_PARAMS) --name shaders -o $@ $<

libqt_plugin_la_SOURCES += $(libqt_plugin_la_SHADER)
CLEANFILES += shaders/shaders.qrc

lib_qt_plugin_la_QRC = assets.qrc

if HAVE_WIN32
lib_qt_plugin_la_QRC += windows.qrc

windows.cpp: $(libqt_plugin_la_RES)
windows.cpp: windows.qrc
	$(AM_V_GEN)$(RCC) $(RCC_EXTRA_PARAMS) --name windows -o $@ $<

nodist_libqt_plugin_la_SOURCES += windows.cpp
endif

EXTRA_DIST += $(lib_qt_plugin_la_QRC) $(libqt_plugin_la_RES) $(libqt_plugin_la_SHADER)

nodist_libqt_plugin_la_SOURCES += assets.cpp  shaders/shaders.cpp
endif

# Emit shader source dependencies so that the built shaders are properly
# registered with the sources as prerequisites
shaders/shaders.deps: Makefile
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_GEN){ \
	  for s in $(libqt_plugin_la_SHADER_FRAG) $(libqt_plugin_la_SHADER_VERT); do \
	    echo "$$s.qsb: $(srcdir)/$$s"; \
	  done; \
	} > $@.tmp && \
	if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

BUILT_SOURCES += shaders/shaders.deps
CLEANFILES    += shaders/shaders.deps

-include shaders/shaders.deps

if ENABLE_VLM
nodist_libqt_plugin_la_SOURCES += dialogs/vlm/vlm.moc.cpp
endif

# Dynamically generate the rules dependencies so that rebuilds are working
# properly. Since this is not GNU Make, we cannot generate the rules using
# text functions.
QML_RULES_DEPS = $(libqt_plugin_la_QML_MODULES:=/qml.deps)

# Always replace a remade included dependency file. GNU make 4.3 can miss a
# main Makefile rebuilt indirectly when an included file retains its timestamp.
# The per-module source-list stamps below still suppress unrelated QRC rebuilds.
# Since every QML modules is built into a given directory, we can use $(@D)
# to get the module name.
$(QML_RULES_DEPS): Makefile
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_GEN){ \
	  echo '$(@D)/qmldir.qrc: $(@D)/qmldir'; \
	  echo '$(@D)/res.qrc $(@D)/qmldir: $(@D)/qml_modules.stamp'; \
	  echo '$(@D)/qmlcachegen.rsp: $$(builddir)/$(@D)/res.qrc'; \
	  echo '$(@D)/qmlcache_loader.cpp: $(@D)/qmlcachegen.rsp'; \
	  echo '$(@D)/qmlassets.cpp: $$(builddir)/$(@D)/qmldir.qrc $$(builddir)/$(@D)/res.qrc'; \
	  echo '$(@D)/qmlassets.cpp: $$(libqml_module_$(@D)_a_QML) $$(libqml_module_$(@D)_a_Singleton_QML)'; \
	  echo 'qml_$(@D)_cachegen_sources = $$(libqml_module_$(@D)_a_QML) $$(libqml_module_$(@D)_a_Singleton_QML)'; \
	  echo 'qml_$(@D)_cachegen_qml = $$(qml_$(@D)_cachegen_sources:.qml=.cpp)'; \
	  echo 'qml_$(@D)_cachegen_js = $$(qml_$(@D)_cachegen_qml:.js=.cpp)'; \
	  echo '$$(qml_$(@D)_cachegen_js:.mjs=.cpp): $$(builddir)/$(@D)/res.qrc'; \
	} > $@.tmp && mv $@.tmp $@

BUILT_SOURCES += $(QML_RULES_DEPS)
CLEANFILES += $(QML_RULES_DEPS)

-include $(QML_RULES_DEPS)

# QRC/qmldir generation depends on the list of files (defined here) but
# not the file contents. Use per-module stamp files so that changes to
# one module's file list don't trigger rebuilds in other modules.
QML_MODULE_STAMPS =
if ENABLE_QT
QML_MODULE_STAMPS += $(libqt_plugin_la_QML_MODULES:=/qml_modules.stamp)
endif

# Pre-expand target lists into named variables so that automake does not
# see the same '$(libqt_plugin_la_QML_MODULES' prefix repeated across
# conditionals and emit spurious "was already defined" warnings.
QML_MODULE_RES_QRC = $(libqt_plugin_la_QML_MODULES:=/res.qrc)
QML_MODULE_QMLDIR = $(libqt_plugin_la_QML_MODULES:=/qmldir)
QML_MODULE_QMLDIR_QRC = $(libqt_plugin_la_QML_MODULES:=/qmldir.qrc)

$(QML_MODULE_STAMPS): Makefile
	$(AM_V_at)$(MKDIR_P) $(@D); \
	echo '$(libqml_module_$(@D)_a_Module) $(libqml_module_$(@D)_a_ModulePath) $(libqml_module_$(@D)_a_QML) $(libqml_module_$(@D)_a_Singleton_QML)' > $@.tmp; \
	if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi

$(QML_MODULE_RES_QRC):
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/scripts/genqrc.py \
	-o $@ \
	-s $(srcdir) \
	--prefix qt/qml/$(libqml_module_$(@D)_a_ModulePath) \
	--sources $(libqml_module_$(@D)_a_QML) $(libqml_module_$(@D)_a_Singleton_QML)

$(QML_MODULE_QMLDIR):
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/scripts/genqmldir.py \
	-o $@ \
	--prefix qt/qml \
	--module $(libqml_module_$(@D)_a_Module) \
	--sources $(libqml_module_$(@D)_a_QML) \
	--singletons $(libqml_module_$(@D)_a_Singleton_QML)

$(QML_MODULE_QMLDIR_QRC):
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/scripts/genqrc.py -o $@ \
	--prefix qt/qml/$(libqml_module_$(@D)_a_ModulePath) --sources $<

CLEANFILES += $(QML_MODULE_STAMPS) $(QML_MODULE_RES_QRC) $(QML_MODULE_QMLDIR) $(QML_MODULE_QMLDIR_QRC) $(libqml_module_util_a_QML:.qml=.cpp)

if HAVE_QMLCACHEGEN

libqt_plugin_la_CPPFLAGS += -DQT_USE_QMLCACHEGEN

cachegen_verbose = $(cachegen_verbose_$(V))
cachegen_verbose_ = $(cachegen_verbose__$(AM_DEFAULT_VERBOSITY))
cachegen_verbose_0 = @echo " CACHEGEN " $@;
cachegen_verbose__0 = $(cachegen_verbose_0)

if HAVE_QT67
QML_CACHEGEN_EXTRA_ARGS=--validate-basic-blocks
endif

# Each .qml/.js/.mjs is compiled with its module's res.qrc as prerequisite. The
# per-module qrc isn't derivable from the target path, so pick it out of $^.
qml_resource = for f in $^; do case $$f in *.qrc) echo --resource=$$f;; esac; done

.qml.cpp:
	$(cachegen_verbose)$(QMLCACHEGEN) $(QML_CACHEGEN_EXTRA_ARGS) `$(qml_resource)` -o $@ $<

.js.cpp:
	$(cachegen_verbose)$(QMLCACHEGEN) $(QML_CACHEGEN_EXTRA_ARGS) `$(qml_resource)` -o $@ $<

.mjs.cpp:
	$(cachegen_verbose)$(QMLCACHEGEN) $(QML_CACHEGEN_EXTRA_ARGS) `$(qml_resource)` -o $@ $<

# Pre-expand target lists into named variables so that automake does not
# see the same $(libqt_plugin_la_QML_MODULES prefix repeated across
# conditionals and emit spurious "was already defined" warnings.
QML_MODULE_CACHEGEN_RSP = $(libqt_plugin_la_QML_MODULES:=/qmlcachegen.rsp)
QML_MODULE_CACHELOADER = $(libqt_plugin_la_QML_MODULES:=/qmlcache_loader.cpp)
QML_MODULE_CACHEGEN_ASSETS = $(libqt_plugin_la_QML_MODULES:=/qmlassets.cpp)

$(QML_MODULE_CACHEGEN_RSP):
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/scripts/qrc2rsp.py --output $@ $^

$(QML_MODULE_CACHELOADER):
	$(cachegen_verbose)$(QMLCACHEGEN) --resource-name $(@D)_cachegen --resource=$(builddir)/$(@D)/res.qrc -o $@ @$<

$(QML_MODULE_CACHEGEN_ASSETS):
	$(AM_V_GEN)$(RCC) $(RCC_EXTRA_PARAMS) --name $(@D)_assets -o $@ $<


CLEANFILES += \
	$(QML_MODULE_CACHELOADER) \
	$(QML_MODULE_CACHEGEN_RSP) \
	$(QML_MODULE_CACHEGEN_ASSETS) \
	$(libqt_plugin_la_ALL_QML:.qml=.cpp)

else

#package all qml from the genreated QRC files

# See comment above QML_MODULE_RES_QRC for why this is a named variable.
QML_MODULE_ASSETS = $(libqt_plugin_la_QML_MODULES:=/qmlassets.cpp)

$(QML_MODULE_ASSETS):
	$(AM_V_GEN)$(RCC) $(RCC_EXTRA_PARAMS) --name $(@D)_assets \
	-o $@ $(builddir)/$(@D)/qmldir.qrc $(builddir)/$(@D)/res.qrc

nodist_libqt_plugin_la_SOURCES += $(QML_MODULE_ASSETS)
if ENABLE_QT
BUILT_SOURCES += $(QML_MODULE_RES_QRC) $(QML_MODULE_QMLDIR)
endif

endif

EXTRA_DIST += $(libqt_plugin_la_QML)

assets.cpp: $(libqt_plugin_la_RES)
assets.cpp: assets.qrc
	$(AM_V_GEN)$(RCC) $(RCC_EXTRA_PARAMS) --name assets -o $@ $<


if HAVE_QT_GTK
libqt_gtktheme_plugin_la_CXXFLAGS = $(AM_CXXFLAGS) $(GTK3_CFLAGS) $(QT_CFLAGS)
libqt_gtktheme_CPPFLAGS = $(AM_CPPFLAGS)
libqt_gtktheme_plugin_la_LIBADD = $(GTK3_LIBS) $(QT_LIBS)
libqt_gtktheme_plugin_la_LDFLAGS = $(AM_LDFLAGS) $(QT_LDFLAGS)
libqt_gtktheme_plugin_la_SOURCES = \
	style/gtkthemeprovider/gtk_compat.cpp \
	style/gtkthemeprovider/gtk_compat.h \
	style/gtkthemeprovider/gtkthemeprovider.cpp \
	style/gtkthemeprovider/gtk_util.cpp \
	style/gtkthemeprovider/gtk_util.h \
	style/gtkthemeprovider/nav_button_provider_gtk.cpp \
	style/gtkthemeprovider/nav_button_provider_gtk.h \
	style/gtkthemeprovider/scoped_gobject.h
if ENABLE_QT
gui_PLUGINS += libqt_gtktheme_plugin.la
endif
endif

if ENABLE_QT
gui_PLUGINS += libqt_plugin.la
BUILT_SOURCES += $(nodist_libqt_plugin_la_SOURCES)
if !HAVE_WIN32
if !HAVE_OS2
if !HAVE_IOS_OR_TVOS
if !HAVE_XROS
pkglibexec_PROGRAMS = vlc-qt-check
VLC_QT_CHECK_DEP = $(builddir)/../../vlc-qt-check

$(builddir)/../../vlc-qt-check: vlc-qt-check
	$(AM_V_GEN)cd ../../ && $(LN_S) -f gui/qt/vlc-qt-check . && chmod +x vlc-qt-check
endif
endif
endif
endif

base_model_test_SOURCES = tests/test_base_model.cpp \
        util/base_model.cpp util/base_model.hpp util/base_model_p.hpp

nodist_base_model_test_SOURCES = util/base_model.moc.cpp \
        util/listcache.moc.cpp \
        util/locallistcacheloader.moc.cpp

base_model_test_CXXFLAGS = $(AM_CXXFLAGS) $(QT_CFLAGS) -fPIC $(CXXFLAGS_qt)
base_model_test_LDADD = $(QT_LIBS) $(LIBS_qt)
base_model_test_LDFLAGS = $(AM_LDFLAGS) $(QT_LDFLAGS)
check_PROGRAMS = base_model_test
TESTS = base_model_test

if HAVE_QT_QTEST

LIBVLC = ../../../lib/libvlc.la
QT_QTEST_COMMON_cppflags = $(libqt_plugin_la_CPPFLAGS) -I$(builddir)/tests -DTOP_BUILDDIR=\"$(abs_top_builddir)\"
QT_QTEST_COMMON_cxxflags = $(AM_CXXFLAGS) $(QT_CFLAGS) $(QT_QTEST_CFLAGS) -fPIC $(CXXFLAGS_qt)
QT_QTEST_COMMON_ldadd = $(QT_LIBS) $(LIBS_qt) $(QT_QTEST_LIBS) $(LIBVLCCORE) $(LIBVLC)
QT_QTEST_COMMON_ldflags = $(AM_LDFLAGS) $(QT_LDFLAGS) $(QT_QTEST_LDFLAGS)


# test_ml_model

test_ml_model_SOURCES = \
	tests/vlc_stub_modules.cpp tests/vlc_stub_modules.hpp \
	tests/test_ml_model.cpp \
	util/base_model.hpp util/base_model_p.hpp  util/base_model.cpp \
	util/listcache.hpp \
	util/vlctick.cpp util/vlctick.hpp \
	util/locallistcacheloader.hpp \
	medialibrary/mlqmltypes.hpp \
	medialibrary/mlbasemodel.hpp medialibrary/mlbasemodel.cpp \
	medialibrary/mlplaylist.hpp medialibrary/mlplaylist.cpp \
	medialibrary/medialib.hpp medialibrary/medialib.cpp \
	medialibrary/mlhomeactivity.hpp medialibrary/mlhomeactivity.cpp \
	medialibrary/mlhomeactivitydata.hpp medialibrary/mlhomeactivitydata.cpp \
	dialogs/new_playlist_data.hpp dialogs/new_playlist_data.cpp \
	medialibrary/mlthreadpool.hpp medialibrary/mlthreadpool.cpp \
	playlist/playlist_controller.hpp playlist/playlist_controller.cpp \
	playlist/playlist_item.hpp playlist/playlist_item.cpp

nodist_test_ml_model_SOURCES = \
	tests/test_ml_model.moc \
	util/base_model.moc.cpp \
	util/listcache.moc.cpp \
	util/locallistcacheloader.moc.cpp \
	util/vlctick.moc.cpp \
	medialibrary/mlqmltypes.moc.cpp \
	medialibrary/mlbasemodel.moc.cpp \
	medialibrary/medialib.moc.cpp \
	medialibrary/mlhomeactivity.moc.cpp \
	medialibrary/mlthreadpool.moc.cpp \
	playlist/playlist_controller.moc.cpp \
	playlist/playlist_item.moc.cpp

BUILT_SOURCES += tests/test_ml_model.moc
CLEANFILES += tests/test_ml_model.moc
test_ml_model_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_ml_model_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_ml_model_LDADD = $(QT_QTEST_COMMON_ldadd)
test_ml_model_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_ml_model
TESTS += test_ml_model

# test_renderer_manager_model

test_renderer_manager_model_SOURCES = \
	tests/vlc_stub_modules.cpp tests/vlc_stub_modules.hpp \
	tests/test_renderer_manager_model.cpp \
    util/renderer_manager.hpp util/renderer_manager.cpp

nodist_test_renderer_manager_model_SOURCES = \
	tests/test_renderer_manager_model.moc \
    util/renderer_manager.moc.cpp

BUILT_SOURCES += tests/test_renderer_manager_model.moc
CLEANFILES += tests/test_renderer_manager_model.moc
test_renderer_manager_model_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_renderer_manager_model_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_renderer_manager_model_LDADD = $(QT_QTEST_COMMON_ldadd)
test_renderer_manager_model_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_renderer_manager_model
TESTS += test_renderer_manager_model

# test_thread_runner

test_thread_runner_SOURCES = \
	tests/test_thread_runner.cpp \
    medialibrary/mlthreadpool.cpp medialibrary/mlthreadpool.hpp

nodist_test_thread_runner_SOURCES = \
	tests/test_thread_runner.moc \
    medialibrary/mlthreadpool.moc.cpp

BUILT_SOURCES += tests/test_thread_runner.moc
CLEANFILES += tests/test_thread_runner.moc
test_thread_runner_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_thread_runner_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_thread_runner_LDADD = $(QT_QTEST_COMMON_ldadd)
test_thread_runner_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_thread_runner
TESTS += test_thread_runner

# test_vlc_dialog_model

test_vlc_dialog_model_SOURCES = \
	tests/vlc_stub_modules.cpp tests/vlc_stub_modules.hpp \
	tests/test_vlc_dialog_model.cpp \
	dialogs/dialogs/dialogmodel.cpp

nodist_test_vlc_dialog_model_SOURCES = \
	tests/test_vlc_dialog_model.moc \
	dialogs/dialogs/dialogmodel.moc.cpp

BUILT_SOURCES += tests/test_vlc_dialog_model.moc
CLEANFILES += tests/test_vlc_dialog_model.moc
test_vlc_dialog_model_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_vlc_dialog_model_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_vlc_dialog_model_LDADD = $(QT_QTEST_COMMON_ldadd)
test_vlc_dialog_model_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_vlc_dialog_model
TESTS += test_vlc_dialog_model


test_qt_navigation_model_SOURCES = \
	tests/vlc_stub_modules.cpp \
	tests/test_navigation_model.cpp \
	maininterface/navigationmodel.hpp \
	maininterface/navigationmodel.cpp

nodist_test_qt_navigation_model_SOURCES = \
	tests/test_navigation_model.moc \
	maininterface/navigationmodel.moc.cpp

BUILT_SOURCES += tests/test_navigation_model.moc
CLEANFILES += tests/test_navigation_model.moc
test_qt_navigation_model_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_navigation_model_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_navigation_model_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_navigation_model_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_navigation_model
TESTS += test_qt_navigation_model

test_qt_navigation_history_SOURCES = \
	tests/vlc_stub_modules.cpp \
	tests/test_navigation_history.cpp \
	util/navigation_history.hpp \
	util/navigation_history.cpp \
	medialibrary/mlqmltypes.hpp
nodist_test_qt_navigation_history_SOURCES = \
	tests/test_navigation_history.moc \
	util/navigation_history.moc.cpp \
	medialibrary/mlqmltypes.moc.cpp \
	network/networktreeitem.moc.cpp
BUILT_SOURCES += tests/test_navigation_history.moc
CLEANFILES += tests/test_navigation_history.moc
test_qt_navigation_history_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_navigation_history_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_navigation_history_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_navigation_history_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_navigation_history
TESTS += test_qt_navigation_history

test_qt_audio_equalizer_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_audio_equalizer.cpp tests/media_player_probe.c \
	player/audio_equalizer.cpp player/audio_equalizer.hpp \
	util/variables.cpp util/variables.hpp
nodist_test_qt_audio_equalizer_SOURCES = \
	tests/test_audio_equalizer.moc player/audio_equalizer.moc.cpp util/variables.moc.cpp
BUILT_SOURCES += tests/test_audio_equalizer.moc
CLEANFILES += tests/test_audio_equalizer.moc
test_qt_audio_equalizer_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_audio_equalizer_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_audio_equalizer_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_audio_equalizer_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_audio_equalizer
TESTS += test_qt_audio_equalizer

test_qt_audio_waveform_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_audio_waveform.cpp tests/media_player_probe.c \
	player/audio_waveform.cpp player/audio_waveform.hpp util/soutchain.cpp util/soutchain.hpp
nodist_test_qt_audio_waveform_SOURCES = \
	tests/test_audio_waveform.moc player/audio_waveform.moc.cpp
BUILT_SOURCES += tests/test_audio_waveform.moc
CLEANFILES += tests/test_audio_waveform.moc
test_qt_audio_waveform_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_audio_waveform_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_audio_waveform_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_audio_waveform_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_audio_waveform
TESTS += test_qt_audio_waveform

test_qt_home_activity_SOURCES = \
	tests/test_home_activity.cpp tests/network_stream_probe.c \
	medialibrary/mlhomeactivitydata.cpp medialibrary/mlhomeactivitydata.hpp medialibrary/mlqmltypes.hpp \
	dialogs/new_playlist_data.cpp dialogs/new_playlist_data.hpp
nodist_test_qt_home_activity_SOURCES = \
	tests/test_home_activity.moc medialibrary/mlqmltypes.moc.cpp
BUILT_SOURCES += tests/test_home_activity.moc
CLEANFILES += tests/test_home_activity.moc
test_qt_home_activity_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_home_activity_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_home_activity_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_home_activity_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_home_activity
TESTS += test_qt_home_activity

test_qt_new_playlist_SOURCES = \
	tests/test_new_playlist.cpp tests/network_stream_probe.c \
	dialogs/new_playlist_model.cpp dialogs/new_playlist_model.hpp \
	dialogs/new_playlist_data.cpp dialogs/new_playlist_data.hpp medialibrary/mlqmltypes.hpp \
	util/vlctick.cpp util/vlctick.hpp \
	medialibrary/mlplaylist.cpp medialibrary/mlplaylist.hpp
nodist_test_qt_new_playlist_SOURCES = \
	tests/test_new_playlist.moc dialogs/new_playlist_model.moc.cpp medialibrary/mlqmltypes.moc.cpp
BUILT_SOURCES += tests/test_new_playlist.moc
CLEANFILES += tests/test_new_playlist.moc
test_qt_new_playlist_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_new_playlist_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_new_playlist_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_new_playlist_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_new_playlist
TESTS += test_qt_new_playlist

test_qt_network_stream_SOURCES = \
	tests/test_network_stream.cpp tests/network_stream_probe.c \
	dialogs/network_stream_model.cpp dialogs/network_stream_model.hpp
nodist_test_qt_network_stream_SOURCES = \
	tests/test_network_stream.moc dialogs/network_stream_model.moc.cpp
BUILT_SOURCES += tests/test_network_stream.moc
CLEANFILES += tests/test_network_stream.moc
test_qt_network_stream_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_network_stream_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_network_stream_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_network_stream_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_network_stream
TESTS += test_qt_network_stream

test_qt_player_timer_point_SOURCES = \
	tests/test_player_timer_point.cpp player/player_timer_point.hpp
nodist_test_qt_player_timer_point_SOURCES = tests/test_player_timer_point.moc
BUILT_SOURCES += tests/test_player_timer_point.moc
CLEANFILES += tests/test_player_timer_point.moc
test_qt_player_timer_point_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_player_timer_point_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_player_timer_point_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_player_timer_point_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_player_timer_point
TESTS += test_qt_player_timer_point

test_qt_local_music_folder_SOURCES = \
	tests/test_local_music_folder.cpp tests/media_player_probe.c tests/network_stream_probe.c \
	player/local_music_folder.cpp player/local_music_folder.hpp
nodist_test_qt_local_music_folder_SOURCES = \
	tests/test_local_music_folder.moc player/local_music_folder.moc.cpp
BUILT_SOURCES += tests/test_local_music_folder.moc
CLEANFILES += tests/test_local_music_folder.moc
test_qt_local_music_folder_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_local_music_folder_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_local_music_folder_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_local_music_folder_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_local_music_folder
TESTS += test_qt_local_music_folder

test_qt_video_transform_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_video_transform.cpp tests/media_player_probe.c \
	player/video_transform.cpp player/video_transform.hpp \
	util/variables.cpp util/variables.hpp
nodist_test_qt_video_transform_SOURCES = \
	tests/test_video_transform.moc player/video_transform.moc.cpp util/variables.moc.cpp
BUILT_SOURCES += tests/test_video_transform.moc
CLEANFILES += tests/test_video_transform.moc
test_qt_video_transform_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_video_transform_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_video_transform_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_video_transform_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_video_transform
TESTS += test_qt_video_transform

test_qt_subtitle_cue_timeline_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_subtitle_cue_timeline.cpp tests/media_player_probe.c \
	player/subtitle_cue_data.cpp player/subtitle_cue_data.hpp \
	player/subtitle_cue_timeline.cpp player/subtitle_cue_timeline.hpp
nodist_test_qt_subtitle_cue_timeline_SOURCES = \
	tests/test_subtitle_cue_timeline.moc player/subtitle_cue_timeline.moc.cpp
BUILT_SOURCES += tests/test_subtitle_cue_timeline.moc
CLEANFILES += tests/test_subtitle_cue_timeline.moc
test_qt_subtitle_cue_timeline_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_subtitle_cue_timeline_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_subtitle_cue_timeline_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_subtitle_cue_timeline_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_subtitle_cue_timeline
TESTS += test_qt_subtitle_cue_timeline

test_qt_subtitle_appearance_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_subtitle_appearance.cpp tests/media_player_probe.c \
	player/subtitle_appearance.cpp player/subtitle_appearance.hpp util/variables.cpp util/variables.hpp
nodist_test_qt_subtitle_appearance_SOURCES = \
	tests/test_subtitle_appearance.moc player/subtitle_appearance.moc.cpp util/variables.moc.cpp
BUILT_SOURCES += tests/test_subtitle_appearance.moc
CLEANFILES += tests/test_subtitle_appearance.moc
test_qt_subtitle_appearance_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_subtitle_appearance_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_subtitle_appearance_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_subtitle_appearance_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_subtitle_appearance
TESTS += test_qt_subtitle_appearance


endif

QML_LOG_COMPILER = $(builddir)/qml_test -input

if HAVE_QT_QUICK_TEST
qml_test_SOURCES = tests/qml_test.cpp tests/qml_test.hpp
nodist_qml_test_SOURCES = tests/qml_test.moc.cpp
CLEANFILES += $(nodist_qml_test_SOURCES)
qml_test_CPPFLAGS = $(libqt_plugin_la_CPPFLAGS)
qml_test_CXXFLAGS = $(AM_CXXFLAGS) $(QT_CFLAGS) -fPIC $(CXXFLAGS_qt) ${QT_QUICK_TEST_CFLAGS} -DQUICK_TEST_SOURCE_DIR="\"${srcdir}/tests\"" -I"${builddir}/tests"
qml_test_LDADD = $(QT_INDIRECT_LIBS) ${QT_QUICK_TEST_LIBS} $(QT_LIBS) $(LIBS_qt) libqml_module_util.a
qml_test_LDFLAGS = $(AM_LDFLAGS) $(QT_QUICK_TEST_LDFLAGS)
check_PROGRAMS += qml_test
EXTRA_DIST += tests/tst_FSM.qml
TESTS += tests/tst_FSM.qml
endif
endif

EXTRA_DIST += tests/test_juxtopposed_playback_bar.py

if ENABLE_QT
if HAVE_QT_QTEST
# Actual native subtitle preset persistence and rendered-output diagnostic.
# Actual PlayerController callbacks on the native NORMAL-lock playlist player.
test_qt_player_subtitle_delay_SOURCES = \
	tests/vlc_stub_modules.cpp \
	tests/test_player_subtitle_delay.cpp \
	player/player_controller.cpp \
	player/input_models.cpp \
	player/audio_equalizer.cpp \
	player/audio_waveform.cpp \
	player/local_music_folder.cpp \
	player/video_transform.cpp \
	player/subtitle_appearance.cpp \
	player/subtitle_cue_data.cpp \
	player/subtitle_cue_timeline.cpp \
	maininterface/subtitle_preset_model.cpp \
	util/audio_device_model.cpp \
	util/variables.cpp \
	util/varchoicemodel.cpp \
	util/renderer_manager.cpp \
	util/vlctick.cpp \
	dialogs/extensions/extensions_manager.cpp \
	dialogs/extensions/extensions.cpp \
	widgets/native/customwidgets.cpp \
	widgets/native/animators.cpp \
	util/soutchain.cpp \
	util/imagehelper.cpp \
	player/player_controller.hpp \
	player/input_models.hpp \
	player/audio_equalizer.hpp \
	player/audio_waveform.hpp \
	player/local_music_folder.hpp \
	player/video_transform.hpp \
	player/subtitle_appearance.hpp \
	player/subtitle_cue_data.hpp \
	player/subtitle_cue_timeline.hpp \
	maininterface/subtitle_preset_model.hpp \
	util/audio_device_model.hpp \
	util/variables.hpp \
	util/varchoicemodel.hpp \
	util/renderer_manager.hpp \
	util/vlctick.hpp \
	dialogs/extensions/extensions_manager.hpp \
	dialogs/extensions/extensions.hpp \
	widgets/native/customwidgets.hpp \
	widgets/native/animators.hpp \
	util/soutchain.hpp \
	util/imagehelper.hpp
nodist_test_qt_player_subtitle_delay_SOURCES = \
	tests/test_player_subtitle_delay.moc \
	player/player_controller.moc.cpp \
	player/input_models.moc.cpp \
	player/audio_equalizer.moc.cpp \
	player/audio_waveform.moc.cpp \
	player/local_music_folder.moc.cpp \
	player/video_transform.moc.cpp \
	player/subtitle_appearance.moc.cpp \
	player/subtitle_cue_timeline.moc.cpp \
	maininterface/subtitle_preset_model.moc.cpp \
	util/audio_device_model.moc.cpp \
	util/variables.moc.cpp \
	util/varchoicemodel.moc.cpp \
	util/renderer_manager.moc.cpp \
	util/vlctick.moc.cpp \
	dialogs/extensions/extensions_manager.moc.cpp \
	dialogs/extensions/extensions.moc.cpp \
	widgets/native/customwidgets.moc.cpp \
	widgets/native/animators.moc.cpp
BUILT_SOURCES += tests/test_player_subtitle_delay.moc
CLEANFILES += tests/test_player_subtitle_delay.moc
test_qt_player_subtitle_delay_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_player_subtitle_delay_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_player_subtitle_delay_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_player_subtitle_delay_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_player_subtitle_delay
TESTS += test_qt_player_subtitle_delay


test_qt_subtitle_presets_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_subtitle_presets.cpp tests/media_player_probe.c \
	maininterface/subtitle_preset_model.cpp maininterface/subtitle_preset_model.hpp \
	player/subtitle_appearance.cpp player/subtitle_appearance.hpp util/variables.cpp util/variables.hpp
nodist_test_qt_subtitle_presets_SOURCES = \
	tests/test_subtitle_presets.moc maininterface/subtitle_preset_model.moc.cpp \
	player/subtitle_appearance.moc.cpp util/variables.moc.cpp
BUILT_SOURCES += tests/test_subtitle_presets.moc
CLEANFILES += tests/test_subtitle_presets.moc
test_qt_subtitle_presets_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_subtitle_presets_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_subtitle_presets_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_subtitle_presets_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_subtitle_presets
TESTS += test_qt_subtitle_presets
endif
endif
EXTRA_DIST += tests/test_juxtopposed_subtitle_presets.py

if ENABLE_QT
if HAVE_QT_QTEST
# Actual local ML, production controller and NORMAL-lock chapter/bookmark models.
test_qt_native_bookmarks_chapters_SOURCES = \
	tests/test_native_bookmarks_chapters.cpp \
	tests/native_model_probe.c \
	tests/network_stream_probe.c \
	player/player_controller.cpp \
	player/input_models.cpp \
	player/audio_equalizer.cpp \
	player/audio_waveform.cpp \
	player/local_music_folder.cpp \
	player/video_transform.cpp \
	player/subtitle_appearance.cpp \
	player/subtitle_cue_data.cpp \
	player/subtitle_cue_timeline.cpp \
	maininterface/subtitle_preset_model.cpp \
	util/audio_device_model.cpp \
	util/variables.cpp \
	util/varchoicemodel.cpp \
	util/renderer_manager.cpp \
	util/vlctick.cpp \
	dialogs/extensions/extensions_manager.cpp \
	dialogs/extensions/extensions.cpp \
	widgets/native/customwidgets.cpp \
	widgets/native/animators.cpp \
	util/soutchain.cpp \
	util/imagehelper.cpp \
	player/player_controller.hpp \
	player/input_models.hpp \
	player/audio_equalizer.hpp \
	player/audio_waveform.hpp \
	player/local_music_folder.hpp \
	player/video_transform.hpp \
	player/subtitle_appearance.hpp \
	player/subtitle_cue_data.hpp \
	player/subtitle_cue_timeline.hpp \
	maininterface/subtitle_preset_model.hpp \
	util/audio_device_model.hpp \
	util/variables.hpp \
	util/varchoicemodel.hpp \
	util/renderer_manager.hpp \
	util/vlctick.hpp \
	dialogs/extensions/extensions_manager.hpp \
	dialogs/extensions/extensions.hpp \
	widgets/native/customwidgets.hpp \
	widgets/native/animators.hpp \
	util/soutchain.hpp \
	util/imagehelper.hpp \
	medialibrary/mlbookmarkmodel.cpp \
	medialibrary/mlbookmarkmodel.hpp \
	medialibrary/medialib.cpp \
	medialibrary/medialib.hpp \
	medialibrary/mlhomeactivity.cpp \
	medialibrary/mlhomeactivity.hpp \
	medialibrary/mlhomeactivitydata.cpp \
	medialibrary/mlhomeactivitydata.hpp \
	dialogs/new_playlist_data.cpp \
	dialogs/new_playlist_data.hpp \
	medialibrary/mlthreadpool.cpp \
	medialibrary/mlthreadpool.hpp \
	medialibrary/mlqmltypes.hpp \
	playlist/playlist_controller.cpp \
	playlist/playlist_controller.hpp \
	playlist/playlist_item.cpp \
	playlist/playlist_item.hpp
nodist_test_qt_native_bookmarks_chapters_SOURCES = \
	tests/test_native_bookmarks_chapters.moc \
	player/player_controller.moc.cpp \
	player/input_models.moc.cpp \
	player/audio_equalizer.moc.cpp \
	player/audio_waveform.moc.cpp \
	player/local_music_folder.moc.cpp \
	player/video_transform.moc.cpp \
	player/subtitle_appearance.moc.cpp \
	player/subtitle_cue_timeline.moc.cpp \
	maininterface/subtitle_preset_model.moc.cpp \
	util/audio_device_model.moc.cpp \
	util/variables.moc.cpp \
	util/varchoicemodel.moc.cpp \
	util/renderer_manager.moc.cpp \
	util/vlctick.moc.cpp \
	dialogs/extensions/extensions_manager.moc.cpp \
	dialogs/extensions/extensions.moc.cpp \
	widgets/native/customwidgets.moc.cpp \
	widgets/native/animators.moc.cpp \
	medialibrary/mlbookmarkmodel.moc.cpp \
	medialibrary/medialib.moc.cpp \
	medialibrary/mlhomeactivity.moc.cpp \
	medialibrary/mlthreadpool.moc.cpp \
	medialibrary/mlqmltypes.moc.cpp \
	playlist/playlist_controller.moc.cpp \
	playlist/playlist_item.moc.cpp
BUILT_SOURCES += tests/test_native_bookmarks_chapters.moc
CLEANFILES += tests/test_native_bookmarks_chapters.moc
test_qt_native_bookmarks_chapters_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_native_bookmarks_chapters_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_native_bookmarks_chapters_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_native_bookmarks_chapters_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_native_bookmarks_chapters
TESTS += test_qt_native_bookmarks_chapters
endif
endif

EXTRA_DIST += tests/test_juxtopposed_subtitle_timeline.py tests/test_page_loader_routes.py
EXTRA_DIST += tests/test_music_network_navigation.py tests/run_music_network_navigation.py \
	tests/test_playlist_single_play.py tests/run_playlist_single_play.py

if ENABLE_QT
if HAVE_QT_QTEST
test_qt_conversion_controller_SOURCES = \
	tests/vlc_stub_modules.cpp tests/test_conversion_controller.cpp tests/media_player_probe.c tests/network_stream_probe.c \
	dialogs/conversion_controller.cpp dialogs/conversion_controller.hpp util/soutchain.cpp util/soutchain.hpp
nodist_test_qt_conversion_controller_SOURCES = \
	tests/test_conversion_controller.moc dialogs/conversion_controller.moc.cpp
BUILT_SOURCES += tests/test_conversion_controller.moc
CLEANFILES += tests/test_conversion_controller.moc
test_qt_conversion_controller_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_conversion_controller_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_conversion_controller_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_conversion_controller_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_conversion_controller
TESTS += test_qt_conversion_controller

# Execute exact production key-routing methods with native Qt/VLC dispatch.
tests/shared_modal_hotkey_production.inc: $(srcdir)/tests/generate_shared_modal_hotkey_test.py $(srcdir)/maininterface/interface_window_handler.cpp $(srcdir)/maininterface/mainctx.cpp
	$(AM_V_GEN)$(PYTHON3) $(srcdir)/tests/generate_shared_modal_hotkey_test.py --repo-root $(top_srcdir) --output $@
tests/test_shared_modal_hotkeys.moc: $(srcdir)/tests/test_shared_modal_hotkeys.cpp tests/shared_modal_hotkey_production.inc
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $(srcdir)/tests/test_shared_modal_hotkeys.cpp
test_qt_shared_modal_hotkeys_SOURCES = tests/vlc_stub_modules.cpp tests/media_player_probe.c tests/test_shared_modal_hotkeys.cpp util/vlchotkeyconverter.cpp util/vlchotkeyconverter.hpp
nodist_test_qt_shared_modal_hotkeys_SOURCES = tests/test_shared_modal_hotkeys.moc tests/shared_modal_hotkey_production.inc util/vlchotkeyconverter.moc.cpp
BUILT_SOURCES += tests/shared_modal_hotkey_production.inc tests/test_shared_modal_hotkeys.moc
CLEANFILES += tests/shared_modal_hotkey_production.inc tests/shared_modal_hotkey_production.inc.json tests/test_shared_modal_hotkeys.moc
EXTRA_DIST += tests/generate_shared_modal_hotkey_test.py tests/test_juxtopposed_conversion_dialog.py
test_qt_shared_modal_hotkeys_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir)/src
test_qt_shared_modal_hotkeys_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_shared_modal_hotkeys_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_shared_modal_hotkeys_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_shared_modal_hotkeys
TESTS += test_qt_shared_modal_hotkeys
endif
endif

# Production General/dialog-host component suites and GNU make remake oracle.
EXTRA_DIST += tests/juxtopposed_general_preferences_fixture.py \
    tests/test_juxtopposed_global_settings.py tests/test_juxtopposed_dialogs_host.py \
    tests/test_qml_dependencies.py tests/fixtures/juxtopposed_global_settings_assets.json \
    tests/fixtures/qml_dependencies_eager_recipe.inc

# Original Light/Dark volume artwork and production native-binding contract.
EXTRA_DIST += tests/test_juxtopposed_light_volume.py \
    tests/fixtures/juxtopposed_light_volume_assets.json \
    tests/fixtures/juxtopposed_light_volume_reference.png \
    tests/fixtures/juxtopposed_light_volume_reference.md

# Production backdrop software fallback and shared General/Host setup checks.
EXTRA_DIST += tests/juxtopposed_backdrop_fixture.py tests/test_juxtopposed_settings_backdrop.py

# Warm list-only QML metadata and preserved stale-include regression.
EXTRA_DIST += tests/test_qml_module_lists.py tests/fixtures/qml_dependencies_stable_cmp_recipe.inc

# Interface maintained contract, authored assets and historical OFF-skin negative.
EXTRA_DIST += tests/test_juxtopposed_interface_settings.py \
    tests/fixtures/juxtopposed_interface_settings_assets.json \
    tests/fixtures/juxtopposed_interface_v1_obsolete.json \
    tests/fixtures/juxtopposed_interface_v1_original.py.txt

# Genuine Interface storage/model fixture. Runtime must discover the matching
# rebuilt Qt plugin options; missing/stale options fail the original assertions.
if ENABLE_QT
if HAVE_QT_QTEST
test_qt_interface_preferences_SOURCES = tests/test_interface_preferences.cpp \
    tests/interface_preferences_probe.c maininterface/interface_preferences_model.cpp \
    maininterface/interface_preferences_model.hpp util/checked_preferences_store.cpp \
    util/checked_preferences_store.hpp
nodist_test_qt_interface_preferences_SOURCES = tests/test_interface_preferences.moc \
    maininterface/interface_preferences_model.moc.cpp
BUILT_SOURCES += tests/test_interface_preferences.moc
CLEANFILES += tests/test_interface_preferences.moc
tests/test_qt_interface_preferences-test_interface_preferences.$(OBJEXT): tests/test_interface_preferences.moc
test_qt_interface_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_interface_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_interface_preferences_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_interface_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_interface_preferences
TESTS += test_qt_interface_preferences

# Real QApplication/static platform imports: opt-in actual desktop gate only.
test_qt_fullscreen_screen_policy_SOURCES = tests/test_fullscreen_screen_policy.cpp util/fullscreen_screen_policy.hpp
nodist_test_qt_fullscreen_screen_policy_SOURCES = tests/test_fullscreen_screen_policy.moc
BUILT_SOURCES += tests/test_fullscreen_screen_policy.moc
CLEANFILES += tests/test_fullscreen_screen_policy.moc
tests/test_qt_fullscreen_screen_policy-test_fullscreen_screen_policy.$(OBJEXT): tests/test_fullscreen_screen_policy.moc
test_qt_fullscreen_screen_policy_CPPFLAGS = $(QT_QTEST_COMMON_cppflags)
test_qt_fullscreen_screen_policy_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_fullscreen_screen_policy_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_fullscreen_screen_policy_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
EXTRA_PROGRAMS = test_qt_fullscreen_screen_policy
if HAVE_WIN32
test_qt_interface_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_interface_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_interface_preferences_LDADD += -lntdll
test_qt_fullscreen_screen_policy_SOURCES += util/winpthreads_shutdown.cpp
test_qt_fullscreen_screen_policy_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_fullscreen_screen_policy_LDADD += -lntdll
endif
if HAVE_LIBATOMIC
test_qt_interface_preferences_LDADD += -latomic
test_qt_fullscreen_screen_policy_LDADD += -latomic
endif
endif
endif

# Actual toolbar FSM with original policy/timer oracle and supplied native boundary.
EXTRA_DIST += tests/test_interface_toolbar_policy.py tests/tst_InterfaceToolbarPolicy.qml \
    tests/fixtures/interface_toolbar/MainCtx.qml tests/fixtures/interface_toolbar/Player.qml

# Playback maintained native-shaped contract and exact original asset ledger.
EXTRA_DIST += tests/test_juxtopposed_playback_settings.py \
    tests/fixtures/juxtopposed_playback_fields.json tests/fixtures/juxtopposed_playback_assets.json

# Genuine Playback model/save/live-policy fixture. Its declaration-only header
# is the MOC input; direct Windows .exe targets must order that included output.
if ENABLE_QT
if HAVE_QT_QTEST
tests/test_playback_preferences.moc: $(srcdir)/tests/test_playback_preferences.hpp
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<
test_qt_playback_preferences_SOURCES = tests/test_playback_preferences.cpp \
    tests/test_playback_preferences.hpp tests/playback_preferences_probe.c \
    maininterface/playback_preferences_model.cpp maininterface/playback_preferences_model.hpp \
    util/checked_preferences_store.cpp util/checked_preferences_store.hpp util/playback_seek_policy.hpp
nodist_test_qt_playback_preferences_SOURCES = tests/test_playback_preferences.moc \
    maininterface/playback_preferences_model.moc.cpp
BUILT_SOURCES += tests/test_playback_preferences.moc
CLEANFILES += tests/test_playback_preferences.moc
tests/test_qt_playback_preferences-test_playback_preferences.$(OBJEXT): tests/test_playback_preferences.moc
test_qt_playback_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_playback_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_playback_preferences_LDADD = $(QT_QTEST_COMMON_ldadd)
test_qt_playback_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_playback_preferences
TESTS += test_qt_playback_preferences
if HAVE_WIN32
test_qt_playback_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_playback_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_playback_preferences_LDADD += -lntdll
endif
if HAVE_LIBATOMIC
test_qt_playback_preferences_LDADD += -latomic
endif
endif
endif

# Audio maintained native-shaped UI and strict original artwork ledger.
EXTRA_DIST += tests/test_juxtopposed_audio_settings.py \
    tests/fixtures/juxtopposed_audio_fields.json tests/fixtures/juxtopposed_audio_assets.json

# Video maintained native-shaped UI and strict original artwork ledger.
EXTRA_DIST += tests/test_juxtopposed_video_settings.py \
    tests/fixtures/juxtopposed_video_fields.json tests/fixtures/juxtopposed_video_assets.json

# Exact frozen Audio/Video/native float fixture bodies; no skipped desired rows.
if ENABLE_QT
if HAVE_QT_QTEST
tests/test_audio_preferences.moc: $(srcdir)/tests/test_audio_preferences.cpp
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<
test_qt_audio_preferences_SOURCES = tests/test_audio_preferences.cpp \
    util/checked_preferences_store.hpp \
    maininterface/audio_preferences_model.cpp \
    maininterface/audio_preferences_model.hpp \
    tests/general_preferences_probe.c \
    tests/audio_preferences_pcm_probe.c \
    util/audio_defaults_policy.hpp \
    util/audio_windows_devices.cpp \
    util/audio_windows_devices.hpp \
    player/audio_equalizer.cpp \
    player/audio_equalizer.hpp \
    util/variables.cpp \
    util/variables.hpp
nodist_test_qt_audio_preferences_SOURCES = tests/test_audio_preferences.moc \
    maininterface/audio_preferences_model.moc.cpp \
    player/audio_equalizer.moc.cpp \
    util/variables.moc.cpp
BUILT_SOURCES += tests/test_audio_preferences.moc
CLEANFILES += tests/test_audio_preferences.moc
tests/test_qt_audio_preferences-test_audio_preferences.$(OBJEXT): tests/test_audio_preferences.moc
test_qt_audio_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_audio_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_audio_preferences_LDADD = util/libqt_plugin_la-checked_preferences_store.lo $(QT_QTEST_COMMON_ldadd)
test_qt_audio_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_audio_preferences
TESTS += test_qt_audio_preferences
if HAVE_WIN32
test_qt_audio_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_audio_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_audio_preferences_LDADD += -lntdll $(LIBCOM) -luuid
endif
if HAVE_LIBATOMIC
test_qt_audio_preferences_CXXFLAGS += -DQT_HAS_LIBATOMIC
test_qt_audio_preferences_LDADD += -latomic
endif

tests/test_video_preferences.moc: $(srcdir)/tests/test_video_preferences.cpp
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<
test_qt_video_preferences_SOURCES = tests/test_video_preferences.cpp \
    util/checked_preferences_store.hpp \
    maininterface/video_preferences_model.cpp \
    maininterface/video_preferences_model.hpp \
    tests/video_preferences_probe.c
nodist_test_qt_video_preferences_SOURCES = tests/test_video_preferences.moc \
    maininterface/video_preferences_model.moc.cpp
BUILT_SOURCES += tests/test_video_preferences.moc
CLEANFILES += tests/test_video_preferences.moc
tests/test_qt_video_preferences-test_video_preferences.$(OBJEXT): tests/test_video_preferences.moc
test_qt_video_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_video_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_video_preferences_LDADD = util/libqt_plugin_la-checked_preferences_store.lo $(QT_QTEST_COMMON_ldadd)
test_qt_video_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_video_preferences
TESTS += test_qt_video_preferences
if HAVE_WIN32
test_qt_video_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_video_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_video_preferences_LDADD += -lntdll
endif
if HAVE_LIBATOMIC
test_qt_video_preferences_CXXFLAGS += -DQT_HAS_LIBATOMIC
test_qt_video_preferences_LDADD += -latomic
endif

tests/test_checked_float_preferences.moc: $(srcdir)/tests/test_checked_float_preferences.cpp
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<
test_qt_checked_float_preferences_SOURCES = tests/test_checked_float_preferences.cpp \
    util/checked_preferences_store.hpp \
    tests/general_preferences_probe.c
nodist_test_qt_checked_float_preferences_SOURCES = tests/test_checked_float_preferences.moc
BUILT_SOURCES += tests/test_checked_float_preferences.moc
CLEANFILES += tests/test_checked_float_preferences.moc
tests/test_qt_checked_float_preferences-test_checked_float_preferences.$(OBJEXT): tests/test_checked_float_preferences.moc
test_qt_checked_float_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_checked_float_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_checked_float_preferences_LDADD = util/libqt_plugin_la-checked_preferences_store.lo $(QT_QTEST_COMMON_ldadd)
test_qt_checked_float_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_checked_float_preferences
TESTS += test_qt_checked_float_preferences
if HAVE_WIN32
test_qt_checked_float_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_checked_float_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_checked_float_preferences_LDADD += -lntdll
endif
if HAVE_LIBATOMIC
test_qt_checked_float_preferences_CXXFLAGS += -DQT_HAS_LIBATOMIC
test_qt_checked_float_preferences_LDADD += -latomic
endif

endif
endif

EXTRA_DIST += tests/generate_owned_audio_fixture.py

EXTRA_DIST += tests/test_juxtopposed_subtitles_settings.py \
    tests/fixtures/juxtopposed_subtitles_fields.json tests/fixtures/juxtopposed_subtitles_assets.json

# Exact desired native subtitle oracle; actual ordinary production store object.
if ENABLE_QT
if HAVE_QT_QTEST
tests/test_subtitles_preferences.moc: $(srcdir)/tests/test_subtitles_preferences.cpp
	$(moc_verbose)$(MOC) $(MOC_CPPFLAGS) -o $@ $<
test_qt_subtitles_preferences_SOURCES = tests/test_subtitles_preferences.cpp \
    maininterface/subtitles_preferences_model.cpp maininterface/subtitles_preferences_model.hpp \
    player/subtitle_appearance.cpp player/subtitle_appearance.hpp \
    maininterface/subtitle_preset_model.cpp maininterface/subtitle_preset_model.hpp \
    util/variables.cpp util/variables.hpp tests/subtitles_preferences_probe.c \
    util/checked_preferences_store.hpp
nodist_test_qt_subtitles_preferences_SOURCES = tests/test_subtitles_preferences.moc \
    maininterface/subtitles_preferences_model.moc.cpp player/subtitle_appearance.moc.cpp \
    maininterface/subtitle_preset_model.moc.cpp util/variables.moc.cpp
BUILT_SOURCES += tests/test_subtitles_preferences.moc
CLEANFILES += tests/test_subtitles_preferences.moc
# Order the object, not a conditional program target: Automake owns its recipe.
tests/test_qt_subtitles_preferences-test_subtitles_preferences.$(OBJEXT): tests/test_subtitles_preferences.moc
test_qt_subtitles_preferences_CPPFLAGS = $(QT_QTEST_COMMON_cppflags) -I$(top_srcdir) -I$(top_srcdir)/src
test_qt_subtitles_preferences_CXXFLAGS = $(QT_QTEST_COMMON_cxxflags)
test_qt_subtitles_preferences_LDADD = util/libqt_plugin_la-checked_preferences_store.lo $(QT_QTEST_COMMON_ldadd)
test_qt_subtitles_preferences_LDFLAGS = $(QT_QTEST_COMMON_ldflags)
check_PROGRAMS += test_qt_subtitles_preferences
TESTS += test_qt_subtitles_preferences
if HAVE_WIN32
test_qt_subtitles_preferences_SOURCES += util/winpthreads_shutdown.cpp
test_qt_subtitles_preferences_LDFLAGS += -Wl,--wrap=pthread_key_delete
test_qt_subtitles_preferences_LDADD += -lntdll
endif
if HAVE_LIBATOMIC
test_qt_subtitles_preferences_CXXFLAGS += -DQT_HAS_LIBATOMIC
test_qt_subtitles_preferences_LDADD += -latomic
endif
endif
endif
EXTRA_DIST += tests/generate_subtitles_fixtures.sh

# Library settings component and supplied model contract.
EXTRA_DIST += tests/test_juxtopposed_library_settings.py tests/juxtopposed_library_preferences_fixture.py \
    tests/fixtures/juxtopposed_library_fields.json tests/fixtures/juxtopposed_library_assets.json

# Native Library tests compile a private policy bridge and WinMsg probe.
# No diagnostic exports are added to shipping Core or plugins.
EXTRA_DIST += tests/test_library_preferences.cpp tests/library_preferences_probe.c \
    tests/library_fixture_fetcher.c

EXTRA_DIST += tests/test_library_ipc_errors.py tests/library_ipc_receiver_errors.c \
    tests/library_ipc_sender_errors.c tests/test_library_opening_process.cpp \
    tests/native_library/prepare.py tests/native_library/run.py tests/native_library/startup.py \
    tests/native_library/preferences.mk tests/native_library/modules.mk \
    tests/native_library/process.mk tests/native_library/README.md
