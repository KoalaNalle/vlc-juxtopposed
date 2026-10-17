/*****************************************************************************
 * main_interface.cpp : Main interface
 ****************************************************************************
 * Copyright (C) 2006-2011 VideoLAN and AUTHORS
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
 *          Ilkka Ollakka <ileoo@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "qt.hpp"

#include <QQmlEngine>
#include <QJSEngine>

#include "mainctx.hpp"
#include "util/fullscreen_screen_policy.hpp"
#include "mainctx_submodels.hpp"
#include "subtitle_preset_model.hpp"
#include "general_preferences_model.hpp"
#include "interface_preferences_model.hpp"
#include "playback_preferences_model.hpp"
#include "video_preferences_model.hpp"
#include "audio_preferences_model.hpp"
#include "subtitles_preferences_model.hpp"
#include "library_preferences_model.hpp"
#include "privacy_preferences_model.hpp"
#include "controls_preferences_model.hpp"
#include "network_preferences_model.hpp"
#include "performance_preferences_model.hpp"
#include "recording_preferences_model.hpp"
#include "dialogs/preferences/controls_shortcuts.hpp"
#include <vlc_dialog.h>
#include "medialibrary/mlhelper.hpp"
#include "medialibrary/mlhomeactivity.hpp"

#include "compositor.hpp"
#include "util/renderer_manager.hpp"
#include "util/csdbuttonmodel.hpp"

#include "util/vlchotkeyconverter.hpp"
#include "util/qt_dirs.hpp"                     // toNativeSeparators

#include "util/color_scheme_model.hpp"

#include "widgets/native/interface_widgets.hpp"     // bgWidget, videoWidget

#include "playlist/playlist_controller.hpp"
#include "player/player_controller.hpp"

#include "dialogs/dialogs_provider.hpp"
#include "dialogs/systray/systray.hpp"

#include "videosurface.hpp"

#include "menus/menus.hpp"                            // Menu creation

#include "dialogs/toolbar/controlbar_profile_model.hpp"
#include "dialogs/help/help.hpp"

#include <QKeyEvent>

#include <QUrl>
#include <QDate>
#include <QMimeData>
#include <QClipboard>
#include <QInputDialog>

#include <QQmlProperty>

#include <QWindow>
#include <QScreen>

#include <QOperatingSystemVersion>

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QStyleHints>
#endif

#ifdef _WIN32
#include <QFileInfo>
#endif

#include <vlc_interface.h>
#include <vlc_preparser.h>

#define VLC_REFERENCE_SCALE_FACTOR 96.

using  namespace vlc::playlist;

// #define DEBUG_INTF

/* Callback prototypes */
static int PopupMenuCB( vlc_object_t *p_this, const char *psz_variable,
                        vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfShowCB( vlc_object_t *p_this, const char *psz_variable,
                       vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfBossCB( vlc_object_t *p_this, const char *psz_variable,
                       vlc_value_t old_val, vlc_value_t new_val, void *param );
static int IntfRaiseMainCB( vlc_object_t *p_this, const char *psz_variable,
                           vlc_value_t old_val, vlc_value_t new_val,
                           void *param );

const QEvent::Type MainCtx::ToolbarsNeedRebuild =
        (QEvent::Type)QEvent::registerEventType();

namespace
{

template <typename T>
T loadVLCOption(vlc_object_t *obj, const char *name);

template <>
int loadVLCOption<int>(vlc_object_t *obj, const char *name)
{
    return var_InheritInteger(obj, name);
}

template <>
float loadVLCOption<float>(vlc_object_t *obj, const char *name)
{
    return var_InheritFloat(obj, name);
}

template <>
bool loadVLCOption<bool>(vlc_object_t *obj, const char *name)
{
    return var_InheritBool(obj, name);
}

}

MainCtx::MainCtx(qt_intf_t *_p_intf)
    : p_intf(_p_intf)
    , m_csdButtonModel {std::make_unique<CSDButtonModel>(this, this)}
{
    // Read new Interface startup policies once. Recreating a Player or tray
    // later in this process must not mix newly-saved values with CONSTANT QML
    // bindings from a previously-created Player.
    if (config_GetType("qt-controls-auto-hide") == VLC_VAR_INTEGER)
        m_startupControlsAutoHide = var_InheritInteger(p_intf, "qt-controls-auto-hide");
    if (config_GetType("qt-controls-hide-timeout") == VLC_VAR_INTEGER)
        m_startupControlsHideTimeout = var_InheritInteger(p_intf, "qt-controls-hide-timeout");
    m_startupShowMediaNameInTitle = var_InheritBool(p_intf, "qt-name-in-title");
    if (config_GetType("qt-remember-window-geometry") == VLC_VAR_BOOL)
        m_startupRememberGeometry = var_InheritBool(p_intf, "qt-remember-window-geometry");
    if (config_GetType("qt-notification-duration") == VLC_VAR_INTEGER)
        m_startupNotificationDuration = var_InheritInteger(p_intf, "qt-notification-duration");
    m_startupFullscreenScreen = var_InheritInteger(p_intf, "qt-fullscreen-screennumber");

    // If `playlist-autostart` is set, and there are initial files
    // we don't bother loading main display just to unload it
    // immediately after. Instead, we directly load the player
    // page:
    if (var_InheritBool(p_intf, "playlist-autostart"))
    {
        // We can not use `vlc_playlist_GetCurrentIndex()`, because
        // playlist starts playing after the interface is added.
        // We also can not use the playlist controller because it
        // is not updated yet.
        vlc_playlist_locker lock(p_intf->p_playlist);

        if (vlc_playlist_Count(p_intf->p_playlist) > 0)
        {
            m_mainInterfaceModes |= MAININTERFACE_MODE_PLAYER;
        }
    }

    /**
     *  Configuration and settings
     *  Pre-building of interface
     **/

    settings = getSettings();
    m_subtitlePresets = new SubtitlePresetModel(p_intf->p_player, THEMIM->getSubtitleAppearance(),
        settings, VLC_OBJECT(p_intf), this);
    m_colorScheme = new ColorSchemeModel(this);

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    connect(m_colorScheme, &ColorSchemeModel::currentChanged, qGuiApp, [colorScheme = m_colorScheme]() {
        QStyleHints *const styleHints = qGuiApp->styleHints();
        if (unlikely(!styleHints))
            return;

        Qt::ColorScheme scheme;
        switch (colorScheme->currentScheme())
        {
        case ColorSchemeModel::ColorScheme::Day:
            scheme = Qt::ColorScheme::Light;
            break;
        case ColorSchemeModel::ColorScheme::Night:
            scheme = Qt::ColorScheme::Dark;
            break;
        case ColorSchemeModel::ColorScheme::System:
        default:
            styleHints->unsetColorScheme();
            return;
        }

        styleHints->setColorScheme(scheme);
    });
#endif

    m_sort = new SortCtx(this);
    m_search = new SearchCtx(this);
    m_playqueuePanel = new SidePanelCtx(this);
    m_navigationPanel = new SidePanelCtx(this);

    // getOSInfo();
    QOperatingSystemVersion currentOS = QOperatingSystemVersion::current();
    switch (currentOS.type()) {
    case QOperatingSystemVersion::OSType::Windows:
        m_osName = Windows;
        break;

    default:
        m_osName = Unknown;
        break;
    }
    m_osVersion = (currentOS.majorVersion());

    loadPrefs(false);
    loadFromSettingsImpl(false);
    // Snapshot startup preferences after the native theme/settings restore.
    m_generalPreferences = new GeneralPreferencesModel(p_intf, m_colorScheme, settings, this);
    ControlsPreferencesModel::Consumers controlsConsumers;
    controlsConsumers.mousePolicy = true;
    controlsConsumers.remotePolicy = true;
    controlsConsumers.openShortcutEditor = [](ControlsPreferencesModel *model) {
        openControlsShortcutEditor(model, QGuiApplication::focusWindow());
    };
    controlsConsumers.openRemoteSetup = [](ControlsPreferencesModel *model) {
        openControlsRemoteSetup(model, QGuiApplication::focusWindow());
    };
    m_controlsPreferences = new ControlsPreferencesModel(p_intf, std::move(controlsConsumers), this);
    m_networkPreferences = new NetworkPreferencesModel(p_intf,
        NetworkPreferencesModel::Consumers{true, true, true, true}, this);
    // Native policy/decoded-output, hardware on/off, priority rollback, encoder
    // snapshot and isolated rescan checks cover these consumers.
    m_performancePreferences = new PerformancePreferencesModel(p_intf,
        PerformancePreferencesModel::Consumers{true, true, true, true, true, true}, this);
    m_recordingPreferences = new RecordingPreferencesModel(p_intf, settings,
        RecordingPreferencesModel::Consumers{true, true, true, true, true}, this);
    connect(m_recordingPreferences, &RecordingPreferencesModel::discoveryDisabled,
        THEMIM->getRendererManager(), &RendererManager::applyDiscoveryPolicy);
    // Default Consumers gates remain false until each native behavior is verified.
    m_interfacePreferences = new InterfacePreferencesModel(p_intf, settings, this);
    // Precise-seek capability stays gated until actual native app acceptance.
    m_playbackPreferences = new PlaybackPreferencesModel(p_intf, this);
    // Video native consumer acceptance gates remain false.
    m_videoPreferences = new VideoPreferencesModel(p_intf, this);
    // Audio native consumer acceptance gates remain false.
    m_audioPreferences = new AudioPreferencesModel(p_intf, this);
    // Native placement/shortcut consumers passed the full subtitle renderer suite.
    SubtitlesPreferencesModel::Consumers subtitleConsumers;
    subtitleConsumers.primaryAlignment = true;
    subtitleConsumers.feedbackDuration = true;
    subtitleConsumers.shortcutFeedback = true;
    m_subtitlesPreferences = new SubtitlesPreferencesModel(p_intf, m_subtitlePresets,
        subtitleConsumers, this);
    // Opening/folder consumers passed native and actual two-process acceptance.
    // Metadata cancellation/fallback and local/network scopes passed separately.
    LibraryPreferencesModel::Consumers libraryConsumers;
    libraryConsumers.instancePolicy = true;
    libraryConsumers.openingQueue = true;
    libraryConsumers.hiddenAttributes = true;
    libraryConsumers.metadataFetch = true;
    m_libraryPreferences = new LibraryPreferencesModel(p_intf, libraryConsumers, this);

    /* Get the available interfaces */
    m_extraInterfaces = new VLCVarChoiceModel(VLC_OBJECT(p_intf->intf), "intf-add", this);

    vlc_medialibrary_t* ml = vlc_ml_instance_get( p_intf );
    b_hasMedialibrary = (ml != NULL);
    if (b_hasMedialibrary) {
        m_medialib = new MediaLib(p_intf, p_intf->p_mainPlaylistController);
    }

    /* Controlbar Profile Model Creation */
    m_controlbarProfileModel = new ControlbarProfileModel(p_intf->mainSettings, this);

    const QUrl homeUrl = QUrl::fromLocalFile(QVLCUserDir( VLC_HOME_DIR ));
    m_dialogFilepath = getSettings()->value( "filedialog-path", homeUrl ).toUrl();
    PrivacyPreferencesModel::LocalHistory localHistory;
    localHistory.invalidatePendingVisits = [this] {
        if (m_medialib && m_medialib->homeActivity()) m_medialib->homeActivity()->invalidatePendingVisits();
    };
    localHistory.clearVisits = [this] {
        if (!m_medialib || !m_medialib->homeActivity()) {
            MLHomeVisitStore visits(settings);
            return visits.clear() ? QString{} : tr("Could not clear folder or playlist activity.");
        }
        auto *activity=m_medialib->homeActivity();
        activity->clearVisits(); return activity->error();
    };
    m_privacyPreferences = new PrivacyPreferencesModel(p_intf, settings, std::move(localHistory), this);
    connect(m_privacyPreferences,&PrivacyPreferencesModel::metadataPreferenceChanged,
            m_libraryPreferences,&LibraryPreferencesModel::reload);
    connect(m_libraryPreferences,&LibraryPreferencesModel::valuesChanged,
            m_privacyPreferences,&PrivacyPreferencesModel::reload);
    connect(m_privacyPreferences,&PrivacyPreferencesModel::fileDialogHistoryCleared,this,[this,homeUrl] {
        m_dialogFilepath=homeUrl;
    });
    if (m_medialib) connect(m_medialib,&MediaLib::historyClearRequested,this,[this] {
        if (m_privacyPreferences->busy()) return;
        const auto reportFailure=[this] {
            if (!m_privacyPreferences->lastError().isEmpty())
                vlc_dialog_display_error(VLC_OBJECT(p_intf),qtu(tr("Could not clear history")),
                    "%s",qtu(m_privacyPreferences->lastError()));
        };
        if (!m_privacyPreferences->clearHistory()) reportFailure();
        else if (m_privacyPreferences->busy())
            connect(m_privacyPreferences,&PrivacyPreferencesModel::busyChanged,this,
                reportFailure,Qt::SingleShotConnection);
    });

    QString platformName = QGuiApplication::platformName();

    b_hasWayland = platformName.startsWith(QLatin1String("wayland"), Qt::CaseInsensitive);

    /*********************************
     * Create the Systray Management *
     *********************************/
    //postpone systray initialisation to speedup starting time
    QMetaObject::invokeMethod(this, &MainCtx::initSystray, Qt::QueuedConnection);

    /*************************************************************
     * Connect the input manager to the GUI elements it manages  *
     * Beware initSystray did some connects on input manager too *
     *************************************************************/
    /**
     * Connects on nameChanged()
     * Those connects are different because options can impeach them to trigger.
     **/
    /* Main Interface statusbar */
    /* and title of the Main Interface*/
    connect( THEMIM, &PlayerController::inputChanged, this, &MainCtx::onInputChanged );
    connect( THEMIM, &PlayerController::hasVideoOutputChanged, this, &MainCtx::onVOutChanged );

    /* END CONNECTS ON IM */

    /* VideoWidget connects for asynchronous calls */
    connect( this, &MainCtx::askToQuit, THEDP, &DialogsProvider::quit, Qt::QueuedConnection  );

    /** END of CONNECTS**/


    /************
     * Callbacks
     ************/
    libvlc_int_t* libvlc = vlc_object_instance(p_intf);
    var_AddCallback( libvlc, "intf-toggle-fscontrol", IntfShowCB, p_intf );
    var_AddCallback( libvlc, "intf-boss", IntfBossCB, p_intf );
    var_AddCallback( libvlc, "intf-show", IntfRaiseMainCB, p_intf );

    /* Register callback for the intf-popupmenu variable */
    var_AddCallback( libvlc, "intf-popupmenu", PopupMenuCB, p_intf );

    if( var_InheritBool( p_intf, "qt-privacy-ask") )
    {
        //postpone dialog call, as composition might not be ready yet
        QMetaObject::invokeMethod(this, [](){
            THEDP->firstRunDialog();
        }, Qt::QueuedConnection);
    }
    else if (m_medialib)
    {
        QMetaObject::invokeMethod(m_medialib, &MediaLib::reload, Qt::QueuedConnection);
    }

    const struct vlc_preparser_cfg cfg = []{
        struct vlc_preparser_cfg cfg{};
        cfg.types = VLC_PREPARSER_TYPE_PARSE;
        cfg.max_parser_threads = 1;
        cfg.timeout = 0;
        cfg.external_process = false;
        return cfg;
    }();
    m_network_preparser = vlc_preparser_New(VLC_OBJECT(libvlc), &cfg);

#ifdef UPDATE_CHECK
    /* Checking for VLC updates */
    if( var_InheritBool( p_intf, "qt-updates-notif" ) &&
        !var_InheritBool( p_intf, "qt-privacy-ask" ) )
    {
        int interval = var_InheritInteger( p_intf, "qt-updates-days" );
        if( QDate::currentDate() >
            getSettings()->value( "updatedate" ).toDate().addDays( interval ) )
        {
            /* check for update at startup */
            getUpdateModel()->checkUpdate();
            getSettings()->setValue( "updatedate", QDate::currentDate() );
        }
    }
#endif

    m_threadRunner = new ThreadRunner();

    m_initialEffectiveMainInterfaceMode = getEffectiveMainInterfaceMode();
}

MainCtx::~MainCtx()
{
    /* Save states */

    m_threadRunner->destroy();
    delete m_controlsPreferences;
    m_controlsPreferences = nullptr;
    delete m_performancePreferences;
    m_performancePreferences = nullptr;
    delete m_recordingPreferences;
    m_recordingPreferences = nullptr;
    delete m_networkPreferences;
    m_networkPreferences = nullptr;
    delete m_privacyPreferences;
    m_privacyPreferences = nullptr;
    delete m_libraryPreferences;
    m_libraryPreferences = nullptr;
    delete m_subtitlesPreferences;
    m_subtitlesPreferences = nullptr;
    delete m_audioPreferences;
    m_audioPreferences = nullptr;
    delete m_videoPreferences;
    m_videoPreferences = nullptr;
    // Playback borrows native player/playlist state; destroy it before Qt/native teardown.
    delete m_playbackPreferences;
    m_playbackPreferences = nullptr;
    // Both services borrow settings; destroy them before those dependencies.
    delete m_interfacePreferences;
    m_interfacePreferences = nullptr;
    // Tear down the service while its settings/native colour dependencies live.
    delete m_generalPreferences;
    m_generalPreferences = nullptr;
    delete m_subtitlePresets;
    m_subtitlePresets = nullptr;

    if ( !p_intf->preferencesResetPending)
    {
        settings->beginGroup("MainWindow");
        settings->setValue( "ShowRemainingTime", m_showRemainingTime );
        settings->setValue( "interface-scale", QString::number( m_intfUserScaleFactor ) );

        /* Save playlist state */
        settings->setValue( "playqueue-panel-docked", m_playqueuePanel->m_docked );
        settings->setValue( "playqueue-panel-visible", m_playqueuePanel->m_visible );
        settings->setValue( "playqueue-panel-width", QString::number( m_playqueuePanel->m_width ) );

        settings->setValue( "navigation-panel-visible", m_navigationPanel->m_visible );
        settings->setValue( "navigation-panel-width", QString::number( m_navigationPanel->m_width ) );

        settings->setValue( "player-playlist-width", QString::number( m_playerPlaylistWidth ) );
        settings->setValue( "artist-albums-width", QString::number( m_artistAlbumsWidth ) );

        settings->setValue( "grid-view", m_gridView );
        settings->setValue( "grouping", m_grouping );

        settings->setValue( "color-scheme-index", m_colorScheme->currentIndex() );

        settings->setValue( "album-sections", m_albumSections );

        settings->setValue( "lyrics-mode", m_lyricsMode );

        /* Save the stackCentralW sizes */
        settings->endGroup();
    }
    else
    {
        settings->remove("MainWindow");
    }

    if( !p_intf->preferencesResetPending && var_InheritBool( p_intf, "save-recentplay" ) )
        getSettings()->setValue( "filedialog-path", m_dialogFilepath );
    else
        getSettings()->remove( "filedialog-path" );

    /* Unregister callbacks */
    libvlc_int_t* libvlc = vlc_object_instance(p_intf);
    var_DelCallback( libvlc, "intf-boss", IntfBossCB, p_intf );
    var_DelCallback( libvlc, "intf-show", IntfRaiseMainCB, p_intf );
    var_DelCallback( libvlc, "intf-toggle-fscontrol", IntfShowCB, p_intf );
    var_DelCallback( libvlc, "intf-popupmenu", PopupMenuCB, p_intf );

    if (m_medialib)
        delete m_medialib;

    if (m_network_preparser)
        vlc_preparser_Delete(m_network_preparser);

    p_intf->p_mi = NULL;
}

bool MainCtx::hasVLM() const {
#ifdef ENABLE_VLM
    return true;
#else
    return false;
#endif
}

bool MainCtx::useClientSideDecoration() const
{
    //don't show CSD when interface is fullscreen
    return !m_windowTitlebar;
}

bool MainCtx::hasFirstrun() const {
    return var_InheritBool(p_intf,  "qt-privacy-ask" );
}

void MainCtx::setUseGlobalShortcuts( bool useShortcuts )
{
    if (m_useGlobalShortcuts == useShortcuts)
        return;
    m_useGlobalShortcuts = useShortcuts;
    emit useGlobalShortcutsChanged(m_useGlobalShortcuts);
}

void MainCtx::setWindowSuportExtendedFrame(bool support) {
    if (m_windowSuportExtendedFrame == support)
        return;
    m_windowSuportExtendedFrame = support;
    emit windowSuportExtendedFrameChanged();
}

void MainCtx::setWindowExtendedMargin(unsigned int margin) {
    if (margin == m_windowExtendedMargin)
        return;
    m_windowExtendedMargin = margin;
    emit windowExtendedMarginChanged(margin);
}

/*****************************
 *   Main UI handling        *
 *****************************/

void MainCtx::loadPrefs(const bool callSignals)
{
    const auto loadFromVLCOption = [this, callSignals](auto &variable, const char *name
            , const std::function<void(MainCtx *)> signal)
    {
        using variableType = std::remove_reference_t<decltype(variable)>;

        const auto value =  loadVLCOption<variableType>(VLC_OBJECT(p_intf), name);
        if (value == variable)
            return;

        variable = value;
        if (callSignals && signal)
            signal(this);
    };

    bool minimalMode = false;
    /* Are we in the enhanced always-video mode or not ? */
    loadFromVLCOption(minimalMode, "qt-minimal-view", nullptr);
    setMinimalView(minimalMode);

    loadFromVLCOption(m_bgCone, "qt-bgcone", &MainCtx::bgConeToggled);

    /* Should the UI stays on top of other windows */
    loadFromVLCOption(b_interfaceOnTop, "video-on-top", [this](MainCtx *)
    {
        emit interfaceAlwaysOnTopChanged(b_interfaceOnTop);
    });

    loadFromVLCOption(m_hasToolbarMenu, "qt-menubar", &MainCtx::hasToolbarMenuChanged);

    loadFromVLCOption(m_windowTitlebar, "qt-titlebar" , &MainCtx::useClientSideDecorationChanged);

    loadFromVLCOption(m_smoothScroll, "qt-smooth-scrolling", &MainCtx::smoothScrollChanged);

    loadFromVLCOption(m_maxVolume, "qt-max-volume", &MainCtx::maxVolumeChanged);

    loadFromVLCOption(m_juxtopposedControls, "qt-juxtopposed-controls", &MainCtx::juxtopposedControlsChanged);

    loadFromVLCOption(m_pinVideoControls, "qt-pin-controls", &MainCtx::pinVideoControlsChanged);

    loadFromVLCOption(m_pinOpacity, "qt-fs-opacity", &MainCtx::pinOpacityChanged);

    loadFromVLCOption(m_safeArea, "qt-safe-area", &MainCtx::safeAreaChanged);

    loadFromVLCOption(m_mouseHideTimeout, "mouse-hide-timeout", &MainCtx::mouseHideTimeoutChanged);
}

void MainCtx::loadFromSettingsImpl(const bool callSignals)
{
    const auto loadFromSettings = [this, callSignals](auto &variable, const char *name
            , const auto defaultValue, auto obj,  auto signal)
    {
        using variableType = std::remove_reference_t<decltype(variable)>;

        const auto value = getSettings()->value(name, defaultValue).template value<variableType>();
        if (value == variable)
            return;

        variable = value;
        if (obj && callSignals && signal)
            (obj->*signal)(variable);
    };

    loadFromSettings(m_playqueuePanel->m_docked, "MainWindow/playqueue-panel-docked", true, m_playqueuePanel, &SidePanelCtx::dockedChanged);
    loadFromSettings(m_playqueuePanel->m_visible, "MainWindow/playqueue-panel-visible", false, m_playqueuePanel, &SidePanelCtx::visibleChanged);
    loadFromSettings(m_playqueuePanel->m_width, "MainWindow/playqueue-panel-width", 120, m_playqueuePanel, &SidePanelCtx::widthChanged);

    loadFromSettings(m_navigationPanel->m_visible, "MainWindow/navigation-panel-visible", true, m_navigationPanel, &SidePanelCtx::visibleChanged);
    loadFromSettings(m_navigationPanel->m_width, "MainWindow/navigation-panel-width", 120, m_navigationPanel, &SidePanelCtx::widthChanged);

    loadFromSettings(m_playerPlaylistWidth, "MainWindow/player-playlist-width", 120, this, &MainCtx::playerPlaylistWidthChanged);
    loadFromSettings(m_artistAlbumsWidth, "MainWindow/artist-albums-width", 120, this,  &MainCtx::artistAlbumsWidthChanged);

    loadFromSettings(m_gridView, "MainWindow/grid-view", true, this, &MainCtx::gridViewChanged);

    loadFromSettings(m_grouping, "MainWindow/grouping", GROUPING_NONE, this, &MainCtx::groupingChanged);

    loadFromSettings(m_showRemainingTime, "MainWindow/ShowRemainingTime", false, this, &MainCtx::showRemainingTimeChanged);

    loadFromSettings(m_albumSections, "MainWindow/album-sections", true, this, &MainCtx::albumSectionsChanged);

    loadFromSettings(m_lyricsMode, "MainWindow/lyrics-mode", true, this, &MainCtx::lyricsModeChanged);

    const auto colorSchemeIndex = getSettings()->value( "MainWindow/color-scheme-index", 0 ).toInt();
    m_colorScheme->setCurrentIndex(colorSchemeIndex);

    /* user interface scale factor */
    auto userIntfScaleFactor = var_InheritFloat(p_intf, "qt-interface-scale");
    if (userIntfScaleFactor == -1)
        userIntfScaleFactor = getSettings()->value( "MainWindow/interface-scale", 1.0).toDouble();
    if (m_intfUserScaleFactor != userIntfScaleFactor)
    {
        m_intfUserScaleFactor = userIntfScaleFactor;
        updateIntfScaleFactor();
    }
}

void MainCtx::reloadPrefs()
{
    if (m_privacyPreferences) m_privacyPreferences->reload();
    if (p_intf->preferencesResetPending && m_medialib && m_medialib->homeActivity())
        m_medialib->homeActivity()->clearVisits();
    loadPrefs(true);
    if (p_intf->preferencesResetPending) m_subtitlePresets->reload();
    else m_subtitlePresets->refreshDefaults();
}

void MainCtx::onInputChanged( bool hasInput )
{
    if( hasInput == false )
        return;
    int autoRaise = var_InheritInteger( p_intf, "qt-auto-raise" );
    if ( autoRaise == MainCtx::RAISE_NEVER )
        return;
    if( THEMIM->hasVideoOutput() == true )
    {
        if( ( autoRaise & MainCtx::RAISE_VIDEO ) == 0 )
            return;
    }
    else if ( ( autoRaise & MainCtx::RAISE_AUDIO ) == 0 )
        return;
    emit askRaise();
}

void MainCtx::onVOutChanged( bool video )
{
    int autoRaise = var_InheritInteger( p_intf, "qt-auto-raise" );
    if ( video && ( autoRaise & MainCtx::RAISE_VIDEO ) )
        emit askRaise();
}

#ifdef KeyPress
#undef KeyPress
#endif
void MainCtx::sendHotkey(Qt::Key key , Qt::KeyboardModifiers modifiers)
{
    // Direct QML and wheel dispatch must obey the same shared-modal boundary
    // as the native window event filter. Do not create a provider during setup.
    const auto *dialogs = Singleton<DialogsProvider>::getInstance<false>();
    if (dialogs && dialogs->qmlDialogVisible()) return;

    QKeyEvent event(QEvent::KeyPress, key, modifiers );
    int vlckey = qtEventToVLCKey(&event);
    var_SetInteger(vlc_object_instance(p_intf), "key-pressed", vlckey);
}

void MainCtx::sendVLCHotkey(int vlcHotkey)
{
    // Direct QML and wheel dispatch must obey the same shared-modal boundary
    // as the native window event filter. Do not create a provider during setup.
    const auto *dialogs = Singleton<DialogsProvider>::getInstance<false>();
    if (dialogs && dialogs->qmlDialogVisible()) return;

    var_SetInteger(vlc_object_instance(p_intf), "key-pressed", vlcHotkey);
}

void MainCtx::updateIntfScaleFactor()
{
    auto newValue = m_intfUserScaleFactor;
    if (QWindow* window = p_intf->p_compositor ? p_intf->p_compositor->interfaceMainWindow() : nullptr)
    {
        QScreen* screen = window->screen();
        if (screen)
        {
            qreal dpi = screen->logicalDotsPerInch();
            newValue = m_intfUserScaleFactor * dpi / VLC_REFERENCE_SCALE_FACTOR;
        }
    }

    const int FACTOR_RESOLUTION = 100;
    if ( static_cast<int>(m_intfScaleFactor * FACTOR_RESOLUTION)
         == static_cast<int>(newValue * FACTOR_RESOLUTION) ) return;

    m_intfScaleFactor = newValue;
    emit intfScaleFactorChanged();
}

void MainCtx::onWindowVisibilityChanged(QWindow::Visibility visibility)
{
    m_windowVisibility = visibility;
}

void MainCtx::setHasAcrylicSurface(const bool v)
{
    if (m_hasAcrylicSurface == v)
        return;

    m_hasAcrylicSurface = v;
    emit hasAcrylicSurfaceChanged(v);
}

void MainCtx::incrementIntfUserScaleFactor(bool increment)
{
    if (increment)
        setIntfUserScaleFactor(m_intfUserScaleFactor + 0.1);
    else
        setIntfUserScaleFactor(m_intfUserScaleFactor - 0.1);
}

void MainCtx::setIntfUserScaleFactor(double newValue)
{
    const auto value = qBound(getMinIntfUserScaleFactor(), newValue, getMaxIntfUserScaleFactor());
    if (qFuzzyCompare(value, m_intfUserScaleFactor))
        return;
    m_intfUserScaleFactor = value;
    updateIntfScaleFactor();
}

void MainCtx::setHasToolbarMenu( bool hasToolbarMenu )
{
    if (m_hasToolbarMenu == hasToolbarMenu)
        return;

    m_hasToolbarMenu = hasToolbarMenu;

    config_PutInt("qt-menubar", (int) hasToolbarMenu);

    emit hasToolbarMenuChanged();
}

void MainCtx::setPinVideoControls(bool pinVideoControls)
{
    if (m_pinVideoControls == pinVideoControls)
        return;

    m_pinVideoControls = pinVideoControls;
    emit pinVideoControlsChanged();
}

void MainCtx::setPinOpacity(float pinOpacity)
{
    if (m_pinOpacity == pinOpacity)
        return;

    m_pinOpacity = pinOpacity;

    emit pinOpacityChanged();
}

inline void MainCtx::initSystray()
{
    bool b_systrayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    bool b_systrayWanted = var_InheritBool( p_intf, "qt-system-tray" );

    if( var_InheritBool( p_intf, "qt-start-minimized") )
    {
        if( b_systrayAvailable )
        {
            b_systrayWanted = true;
            b_hideAfterCreation = true;
        }
        else
            msg_Err( p_intf, "cannot start minimized without system tray bar" );
    }

    if( b_systrayAvailable && b_systrayWanted )
        m_systray = std::make_unique<VLCSystray>(this);
}

ThreadRunner* MainCtx::threadRunner() const
{
    return m_threadRunner;
}

QUrl MainCtx::folderMRL(const QString &fileMRL) const
{
    return folderMRL(QUrl::fromUserInput(fileMRL));
}

QUrl MainCtx::folderMRL(const QUrl &fileMRL) const
{
    if (fileMRL.isLocalFile())
    {
        const QString f = fileMRL.toLocalFile();
        return QUrl::fromLocalFile(QFileInfo(f).absoluteDir().absolutePath());
    }

    return {};
}

QString MainCtx::displayMRL(const QUrl &mrl) const
{
    return urlToDisplayString(mrl);
}

void MainCtx::setPlayerPlaylistWidth( double factor )
{
    if (factor > 0.0)
    {
        m_playerPlaylistWidth = factor;
        emit playerPlaylistWidthChanged(factor);
    }
}

void MainCtx::setbgCone(bool bgCone)
{
    if (m_bgCone == bgCone)
        return;

    m_bgCone = bgCone;

    emit bgConeToggled();
}

void MainCtx::setMinimalView(bool enable)
{
    if (m_mainInterfaceModes.testFlag(MAININTERFACE_MODE_MINIMAL) == enable)
        return;

    m_mainInterfaceModes.setFlag(MAININTERFACE_MODE_MINIMAL, enable);
    emit mainInterfaceModesChanged(m_mainInterfaceModes);
}

void MainCtx::setPlayerView(bool enable)
{
    if (m_mainInterfaceModes.testFlag(MAININTERFACE_MODE_PLAYER) == enable)
        return;

    m_mainInterfaceModes.setFlag(MAININTERFACE_MODE_PLAYER, enable);
    emit mainInterfaceModesChanged(m_mainInterfaceModes);
}

void MainCtx::setShowRemainingTime( bool show )
{
    m_showRemainingTime = show;
    emit showRemainingTimeChanged(show);
}

void MainCtx::setGridView(bool asGrid)
{
    m_gridView = asGrid;
    emit gridViewChanged( asGrid );
}

void MainCtx::setGrouping(Grouping grouping)
{
    m_grouping = grouping;

    emit groupingChanged(grouping);
}

void MainCtx::setAlbumSections(bool enabled)
{
    if (m_albumSections == enabled)
        return;

    m_albumSections = enabled;
    emit albumSectionsChanged(enabled);
}

void MainCtx::setLyricsMode(bool enabled)
{
    if (m_lyricsMode == enabled)
        return;

    m_lyricsMode = enabled;
    emit lyricsModeChanged(enabled);
}

void MainCtx::setInterfaceAlwaysOnTop( bool on_top )
{
    if (b_interfaceOnTop == on_top)
        return;

    b_interfaceOnTop = on_top;
    emit interfaceAlwaysOnTopChanged(on_top);
}

bool MainCtx::hasEmbededVideo() const
{
    return m_videoSurfaceProvider && m_videoSurfaceProvider->hasVideoEmbed();
}

void MainCtx::setVideoSurfaceProvider(VideoSurfaceProvider* videoSurfaceProvider)
{
    if (m_videoSurfaceProvider)
        disconnect(m_videoSurfaceProvider, &VideoSurfaceProvider::hasVideoEmbedChanged, this, &MainCtx::hasEmbededVideoChanged);
    m_videoSurfaceProvider = videoSurfaceProvider;
    if (m_videoSurfaceProvider)
        connect(m_videoSurfaceProvider, &VideoSurfaceProvider::hasVideoEmbedChanged,
                this, &MainCtx::hasEmbededVideoChanged,
                Qt::QueuedConnection);
    emit hasEmbededVideoChanged(m_videoSurfaceProvider && m_videoSurfaceProvider->hasVideoEmbed());
}

QJSValue MainCtx::urlListToMimeData(const QJSValue &array) {
    // NOTE: Due to a Qt regression since 17318c4
    //       (Nov 11, 2022), it is not possible to
    //       use RFC-2483 compliant string here.
    //       This regression was later corrected by
    //       c25f53b (Jul 31, 2024).
    // NOTE: Qt starts supporting string list since
    //       17318c4, so starting from 6.5.0 a string
    //       list can be used which is not affected
    //       by the said issue. For Qt versions below
    //       6.5.0, use byte array which is used as is
    //       by Qt.
    assert(array.property("length").toInt() > 0);

    QJSEngine* const engine = qjsEngine(this);
    assert(engine);

    QJSValue data;
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    QString string;
    for (int i = 0; i < array.property(QStringLiteral("length")).toInt(); ++i)
    {
        QString decodedUrl;
        const QJSValue element = array.property(i);
        if (element.isUrl())
            // QJSValue does not have `toUrl()`
            decodedUrl = QJSManagedValue(element, engine).toUrl().toString(QUrl::FullyEncoded);
        else if (element.isString())
            // If the element is string, we assume it is already encoded
            decodedUrl = element.toString();
        else if (element.isVariant())
        {
            const QVariant variant = element.toVariant();
            if (variant.typeId() == QMetaType::QUrl)
                decodedUrl = variant.toUrl().toString(QUrl::FullyEncoded);
            else if (variant.typeId() == QMetaType::QString)
                decodedUrl = variant.toString();
            else
                Q_UNREACHABLE();
        }
        else
            Q_UNREACHABLE(); // Assertion failure in debug builds
        string += decodedUrl + QStringLiteral("\r\n");
    }
    string.chop(2);
    data = engine->toScriptValue(string);
#else
    data = array;
#endif
    QJSValue ret = engine->newObject();
    ret.setProperty(QStringLiteral("text/uri-list"), data);
    return ret;
}

VideoSurfaceProvider* MainCtx::getVideoSurfaceProvider() const
{
    return m_videoSurfaceProvider;
}

/************************************************************************
 * Events stuff
 ************************************************************************/

void MainCtx::onWindowClose( QWindow* )
{
    PlaylistController* playlistController = p_intf->p_mainPlaylistController;
    PlayerController* playerController = p_intf->p_mainPlayerController;

    if (m_videoSurfaceProvider)
        m_videoSurfaceProvider->onWindowClosed();
    //We need to make sure that noting is playing anymore otherwise the vout will be closed
    //after the main interface, and it requires (at least with OpenGL) that the OpenGL context
    //from the main window is still valid.
    //vlc_window_ReportClose is currently stubbed
    if (playerController && playerController->hasVideoOutput()) {
        connect(playerController, &PlayerController::playingStateChanged, [this](PlayerController::PlayingState state){
            if (state == PlayerController::PLAYING_STATE_STOPPED) {
                emit askToQuit();
            }
        });
        playlistController->stop();
    }
    else
    {
        emit askToQuit(); /* ask THEDP to quit, so we have a unique method */
    }
}

void MainCtx::toggleToolbarMenu()
{
    setHasToolbarMenu(!m_hasToolbarMenu);
}

void MainCtx::toggleInterfaceFullScreen()
{
    emit setInterfaceFullScreen( m_windowVisibility != QWindow::FullScreen );
}

void MainCtx::emitBoss()
{
    emit askBoss();
}

void MainCtx::emitShow()
{
    emit askShow();
}

void MainCtx::emitRaise()
{
    emit askRaise();
}

VLCVarChoiceModel* MainCtx::getExtraInterfaces()
{
    return m_extraInterfaces;
}

bool MainCtx::pasteFromClipboard()
{
    assert(qApp);
    const QClipboard *const clipboard = qApp->clipboard();
    if (Q_UNLIKELY(!clipboard))
        return false;
    const QMimeData *mimeData = clipboard->mimeData(QClipboard::Selection);
    if (!mimeData || !mimeData->hasUrls())
        mimeData = clipboard->mimeData(QClipboard::Clipboard);

    if (Q_UNLIKELY(!mimeData))
        return false;

    QList<QUrl> urlList = mimeData->urls();
    QString text = mimeData->text();

    if (urlList.count() > 1 || text.contains('\n'))
    {
        // NOTE: The reason that mime data for `text/uri-list` is not used
        //       directly as the placeholder of the input dialog instead of
        //       re-constructing is to decode the urls in the list.
        QString placeholder;

        if (urlList.isEmpty())
        {
            placeholder = std::move(text);
        }
        else
        {
            for (const auto& i : urlList)
                placeholder += i.toString(QUrl::PrettyDecoded) + '\n';
            placeholder.chop(1);
        }

        bool ok = false;
        const QString ret = QInputDialog::getMultiLineText(nullptr,
                                                           qtr("Paste from clipboard"),
                                                           qtr("Do you want to enqueue the following URLs into the play queue?"),
                                                           placeholder,
                                                           &ok);
        if (!ok)
            return false;

        for (const auto& i : QStringView(ret).split('\n'))
        {
            if (i.length() > 0)
                THEMPL->append(i.trimmed().toString(), false);
        }

        return true;
    }
    else if ((urlList.count() == 1) || mimeData->hasText())
    {
        THEDP->openUrlDialog();
        return true;
    }

    return false;
}

/*****************************************************************************
 * PopupMenuCB: callback triggered by the intf-popupmenu playlist variable.
 *  We don't show the menu directly here because we don't want the
 *  caller to block for a too long time.
 *****************************************************************************/
static int PopupMenuCB( vlc_object_t *, const char *,
                        vlc_value_t, vlc_value_t new_val, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;

    if( p_intf->pf_show_dialog )
    {
        p_intf->pf_show_dialog( p_intf->intf, INTF_DIALOG_POPUPMENU,
                                new_val.b_bool, NULL );
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfShowCB: callback triggered by the intf-toggle-fscontrol libvlc variable.
 *****************************************************************************/
static int IntfShowCB( vlc_object_t *, const char *,
                       vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitShow();

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfRaiseMainCB: callback triggered by the intf-show-main libvlc variable.
 *****************************************************************************/
static int IntfRaiseMainCB( vlc_object_t *, const char *,
                            vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitRaise();

    return VLC_SUCCESS;
}

/*****************************************************************************
 * IntfBossCB: callback triggered by the intf-boss libvlc variable.
 *****************************************************************************/
static int IntfBossCB( vlc_object_t *, const char *,
                       vlc_value_t, vlc_value_t, void *param )
{
    qt_intf_t *p_intf = (qt_intf_t *)param;
    p_intf->p_mi->emitBoss();

    return VLC_SUCCESS;
}

bool MainCtx::acrylicActive() const
{
    return m_acrylicActive;
}

void MainCtx::setAcrylicActive(bool newAcrylicActive)
{
    if (m_acrylicActive == newAcrylicActive)
        return;

    m_acrylicActive = newAcrylicActive;
    emit acrylicActiveChanged();
}

bool MainCtx::preferHotkeys() const
{
    return m_preferHotkeys;
}

void MainCtx::setPreferHotkeys(bool enable)
{
    if (m_preferHotkeys == enable)
        return;

    m_preferHotkeys = enable;

    emit preferHotkeysChanged();
}

QWindow *MainCtx::intfMainWindow() const
{
    if (p_intf->p_compositor)
        return p_intf->p_compositor->interfaceMainWindow();
    else
        return nullptr;
}

QVariant MainCtx::settingValue(const QString &key, const QVariant &defaultValue) const
{
    return settings->value(key, defaultValue);
}

void MainCtx::setSettingValue(const QString &key, const QVariant &value)
{
    settings->setValue(key, value);
}

void MainCtx::setAttachedToolTip(QObject *toolTip)
{
    // See QQuickToolTipAttachedPrivate::instance(bool create)
    assert(toolTip);

    // Prevent possible invalid down-casting:
    assert(toolTip->inherits("QQuickToolTip"));

    QQmlEngine* const engine = qmlEngine(toolTip);
    assert(engine);
    assert(engine->objectOwnership(toolTip) == QQmlEngine::ObjectOwnership::JavaScriptOwnership);

    // Dynamic internal property:
#if QT_VERSION < QT_VERSION_CHECK(6, 12, 0)
    static const char* const name = "_q_QQuickToolTip";
#else
    static const char* const name = "_q_shared_QQuickToolTip";
#endif

    if (const auto obj = engine->property(name).value<QObject *>())
    {
        if (engine->objectOwnership(obj) == QQmlEngine::ObjectOwnership::CppOwnership)
            obj->deleteLater();
    }

    // setProperty() will return false, so there is no
    // need to check the return value:
    engine->setProperty(name, QVariant::fromValue(toolTip));

    // Check if the attached tooltip is actually the
    // one that is set
#ifndef NDEBUG
    QMetaObject::invokeMethod(toolTip, [toolTip]() {
        const auto engine = qmlEngine(toolTip);
        if (Q_UNLIKELY(!engine)) // Very unlikely, if not impossible
            return;
        QQmlComponent component(engine);
        component.setData(QByteArrayLiteral("import QtQuick; import QtQuick.Templates as T; Item { }"), {});
        QObject* const obj = component.create();
        assert(obj);
        // Consider disabling setting of custom attached
        // tooltip if the following assertion fails:
        if (QQmlProperty::read(obj, QStringLiteral("T.ToolTip.toolTip"), qmlContext(obj)).value<QObject*>() != toolTip)
            qmlWarning(obj) << "Could not set self as custom ToolTip!";
        obj->deleteLater();
    }, Qt::QueuedConnection);
#endif
}

MainCtx::MainInterfaceMode MainCtx::getEffectiveMainInterfaceMode() const {
    //priority applies across modes
    if (m_mainInterfaceModes & MainCtx::MAININTERFACE_MODE_MINIMAL)
        return MAININTERFACE_MODE_MINIMAL;
    else if (m_mainInterfaceModes & MAININTERFACE_MODE_PLAYER)
        return MAININTERFACE_MODE_PLAYER;
    else if (m_mainInterfaceModes & MAININTERFACE_MODE_MAINDISPLAY)
        return MAININTERFACE_MODE_MAINDISPLAY;
    else
        return MAININTERFACE_MODE_INVALID;
}

double MainCtx::dp(const double px, const double scale)
{
    return std::round(px * scale);
}

double MainCtx::dp(const double px) const
{
    return dp(px, m_intfScaleFactor);
}

bool MainCtx::useXmasCone() const
{
    return (QDate::currentDate().dayOfYear() >= QT_XMAS_JOKE_DAY)
            && var_InheritBool( p_intf, "qt-icon-change" );
}

bool WindowStateHolder::holdFullscreen(QWindow *window, Source source, bool hold, int screenIndex)
{
    return vlc::qt::holdWindowFullscreen(window, unsigned(source), hold, screenIndex);
}

bool WindowStateHolder::holdOnTop(QWindow *window, Source source, bool hold)
{
    QVariant prop = window->property("__windowOnTop");
    bool ok = false;
    unsigned onTopCounter = prop.toUInt(&ok);
    if (!ok)
        onTopCounter = 0;

    if (hold)
        onTopCounter |= source;
    else
        onTopCounter &= ~source;

#ifdef _WIN32
    if (!window->handle()) // do not call hold on top if there is no platform window
        return false;
#endif

    if( onTopCounter != 0 )
    {
#ifdef _WIN32
        // Qt::WindowStaysOnTopHint is broken on Windows: QTBUG-133034, QTBUG-132522.
        // Reported to be fixed by Qt 6.9, but needs additional checking if the solution does not interfere with `CSDWin32EventHandler`.
        SetWindowPos(reinterpret_cast<HWND>(window->winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
        window->setFlag(Qt::WindowStaysOnTopHint, true);
#endif
    }
    else
    {
#ifdef _WIN32
        // Qt::WindowStaysOnTopHint is broken on Windows: QTBUG-133034, QTBUG-132522.
        // Reported to be fixed by Qt 6.9, but needs additional checking if the solution does not interfere with `CSDWin32EventHandler`.
        SetWindowPos(reinterpret_cast<HWND>(window->winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
#else
        window->setFlag(Qt::WindowStaysOnTopHint, false);
#endif
    }

    window->setProperty("__windowOnTop", QVariant::fromValue(onTopCounter));

    return onTopCounter != 0;
}

double MainCtx::artistAlbumsWidth() const
{
    return dp(m_artistAlbumsWidth);
}

void MainCtx::setArtistAlbumsWidth(double newArtistAlbumsWidth)
{
    double unscaledValue = newArtistAlbumsWidth / getIntfScaleFactor();
    if (qFuzzyCompare(m_artistAlbumsWidth, unscaledValue))
        return;

    m_artistAlbumsWidth = unscaledValue;
    emit artistAlbumsWidthChanged( dp(m_artistAlbumsWidth) );
}

#ifdef UPDATE_CHECK
UpdateModel* MainCtx::getUpdateModel() const
{
    if (!m_updateModel)
    {
        m_updateModel = std::make_unique<UpdateModel>(p_intf);

        connect(m_updateModel.get(), &UpdateModel::updateStatusChanged, this, [this](){
            // No-op, modern update pane opens itself automatically when status changes.
            // However, the old update dialog is used as fallback in case for any reason
            // the modern pane is not available.

            const auto openLegacyDialogIfNecessary = [&]() {
                // Fallback:

                if (Q_LIKELY(p_intf &&
                             p_intf->p_compositor &&
                             p_intf->p_compositor->quickWindow() &&
                             p_intf->p_compositor->quickWindow()->contentItem()))
                {
                    const auto target = p_intf->p_compositor->quickWindow()->contentItem();
                    QMetaObject::invokeMethod(target, [target]() {
                        const auto updatePaneLoader = target->findChild<QQuickItem*>(QStringLiteral("updatePaneLoader"));
                        if (!updatePaneLoader || (updatePaneLoader->property("status").toInt() != 1 /* Loader.Ready */))
                        {
                            qDebug("No acknowledgement from the modern update loader, falling back to the old update dialog.");
                            THEDP->updateDialog(DialogsProvider::Show);
                        }
                    }, Qt::QueuedConnection);
                }
                else
                {
                    qDebug("Compositor is not available, falling back to the old update dialog.");
                    THEDP->updateDialog();
                }
            };

            if (m_updateModel->explicitCheck())
            {
                if (m_updateModel->updateStatus() != UpdateModel::Unchecked)
                    openLegacyDialogIfNecessary();
            }
            else
            {
                switch (m_updateModel->updateStatus())
                {
                case UpdateModel::UpToDate:
                case UpdateModel::Unchecked:
                case UpdateModel::Checking:
                    break;
                case UpdateModel::NeedUpdate:
                    qWarning() << "Need update";
                    [[fallthrough]];
                default:
                    openLegacyDialogIfNecessary();
                }
            }
        });
    }

    return m_updateModel.get();
}
#endif
