// SPDX-License-Identifier: GPL-2.0-or-later
// Private native setup driver; never included by the ordinary GUI target.
QObject *setupPopup()
{
    for (auto *object : objects())
        if (object->objectName() == QStringLiteral("juxtopposedSetupDialog")
                && object->property("visible").toBool()) return object;
    return nullptr;
}
void exerciseSetup(const QString &mode)
{
    auto *general = context->getGeneralPreferences();
    auto *provider = DialogsProvider::getInstance();
    auto *host = named(QStringLiteral("juxtopposedDialogsHost"));
    auto *settings = context->getIntf()->mainSettings;
    check(general && provider && host && settings, "real native setup providers exist");
    check(config_GetInt("qt-privacy-ask") == 0, "owned setup fixture starts with a previously decided native policy");
    const auto bindings = context->getControlsPreferences()->activeShortcuts();
    for (const char *key : {"key-play-pause", "key-toggle-fullscreen", "key-audio-track"}) {
        char *value = config_GetPsz(key);
        check(bindings.value(QString::fromLatin1(key)).toString() == QString::fromUtf8(value ? value : ""),
              "tour exposes the active native startup binding");
        free(value);
    }
    if (mode == QStringLiteral("setup-reopen")) {
        QTest::qWait(500);
        check(!general->setupRequired() && !general->setupReady() && !setupPopup() && !provider->qmlDialogVisible(),
              "fresh OS process restores completed setup without reopening its modal");
        check(general->values().value(QStringLiteral("theme")).toInt() == 1
              && general->values().value(QStringLiteral("accentIndex")).toInt() == 6,
              "fresh OS process restores the saved Light theme and Purple accent");
        check(settings->value(GeneralPreferencesModel::setupVersionKey()).toInt() == 1,
              "fresh OS process reads the actual completed setup marker");
        provider->showNetworkStreamDialog();
        wait([] { return named(QStringLiteral("networkCancel")); }, "normal network workflow opens after completed setup restart");
        click(named(QStringLiteral("networkCancel")));
        wait([&] { return !provider->qmlDialogVisible(); }, "normal network workflow closes after completed setup restart");
        capture(QStringLiteral("native-setup-restored.png"));
        report.insert("nativeSetupRestartAccepted", true);
        return;
    }
    wait([] { return setupPopup(); }, "actual setup opens automatically after owned saved policy readiness");
    check(general->setupRequired() && general->setupReady() && provider->qmlDialogVisible(),
          "actual native setup holds the shared modal hotkey guard");
    const auto before = general->values();
    const auto source = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    context->getMediaLibrary()->addAndPlay(source);
    wait([&] { return player()->getUrl() == source && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; },
         "owned native audio decodes while setup remains open", 15000);
    wait([] { return nativeTime() > VLC_TICK_FROM_MS(200); }, "native primary clock advances while setup is open");
    const auto playingInput = player()->getInput();
    const auto playingClock = nativeTime();
    const auto sample = named(QStringLiteral("juxtopposedSetupPreviewControls"));
    check(sample != nullptr, "actual sample player exists during primary playback");
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
        sample->mapToScene(QPointF(40, sample->height() / 2)).toPoint());
    wait([&] { return nativeTime() > playingClock + VLC_TICK_FROM_MS(200); }, "primary clock keeps advancing through sample-player pointer input");
    check(setupPopup() && player()->getInput() == playingInput
          && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING,
          "inert setup sample leaves the real native primary playing");
    { vlc_player_locker lock(context->getIntf()->p_player); vlc_player_Pause(context->getIntf()->p_player); }
    wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED; },
         "owned setup primary pauses normally");
    const auto input = player()->getInput();
    const auto clock = nativeTime();
    const auto position = player()->getPosition();
    auto volume = [&] { vlc_player_locker lock(context->getIntf()->p_player); return vlc_player_aout_GetVolume(context->getIntf()->p_player); };
    auto mute = [&] { vlc_player_locker lock(context->getIntf()->p_player); return vlc_player_aout_IsMuted(context->getIntf()->p_player); };
    const auto initialVolume = volume();
    const auto initialMute = mute();
    const auto unchanged = [&] {
        return player()->getInput() == input && player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED
            && nativeTime() == clock && player()->getPosition() == position
            && qAbs(volume() - initialVolume) < 0.0001 && mute() == initialMute;
    };
    capture(QStringLiteral("native-setup-page-one.png"));
    for (const char *method : {"showGlobalSettingsDialog", "showNetworkStreamDialog", "showNewPlaylistDialog", "showConversionDialog"}) {
        check(QMetaObject::invokeMethod(provider, method, Qt::DirectConnection), "actual provider receives competing setup route");
        QTest::qWait(40);
        check(setupPopup() && provider->qmlDialogVisible() && unchanged(),
              "competing native route preserves exclusive setup and exact paused input");
    }
    // Real pointer input lands on the inert sample player rather than the
    // primary native controller, including its volume slider.
    for (const char *name : {"juxtopposedPlay", "juxtopposedMute", "juxtopposedQueue"}) {
        auto *preview = named(QStringLiteral("juxtopposedSetupPreviewControls"));
        QQuickItem *button = nullptr;
        for (auto *candidate : preview->findChildren<QQuickItem *>())
            if (candidate->objectName() == QString::fromLatin1(name)) button = candidate;
        check(button && !button->property("activeFocusOnTab").toBool(), "sample player button is excluded from keyboard traversal");
        const auto point = button->mapToScene(button->boundingRect().center()).toPoint();
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point); QTest::qWait(50);
        check(unchanged(), "actual setup sample pointer input preserves the native primary clocks volume and mute");
    }
    click(named(QStringLiteral("setupTheme1")));
    click(named(QStringLiteral("setupAccent6")));
    check(general->values() == before && unchanged(), "actual theme and accent drafts leave saved preferences and paused primary untouched");
    capture(QStringLiteral("native-setup-light-purple.png"));
    click(named(QStringLiteral("setupNext")));
    check(setupPopup()->property("pageIndex").toInt() == 1 && general->values() == before && unchanged(),
          "native Next reaches actual shortcut tour without saving or media action");
    capture(QStringLiteral("native-setup-shortcut-tour.png"));
    click(named(QStringLiteral("setupBack")));
    check(setupPopup()->property("draftTheme").toInt() == 1 && setupPopup()->property("draftAccent").toInt() == 6,
          "actual Back retains both setup drafts");
    if (mode == QStringLiteral("setup-cancel")) {
        QTest::keyClick(window, Qt::Key_Escape); QTest::qWait(150);
        check(!setupPopup() && !provider->qmlDialogVisible() && general->setupRequired()
              && !settings->contains(GeneralPreferencesModel::setupVersionKey()) && general->values() == before && unchanged(),
              "native Escape closes without saving appearance or completion");
        general->reload(); QTest::qWait(150);
        check(!setupPopup() && !provider->qmlDialogVisible(), "native reload does not reopen postponed setup in the same process");
        report.insert("nativeSetupCancelAccepted", true);
        return;
    }
    click(named(QStringLiteral("setupNext")));
    const auto config = QString::fromUtf8(qgetenv("VLC_OWNED_SETUP_CONFIG"));
    check(QFileInfo::exists(config), "owned setup native configuration exists before controlled storage failure");
    HANDLE handle = CreateFileW(reinterpret_cast<LPCWSTR>(config.utf16()), GENERIC_READ,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    check(handle != INVALID_HANDLE_VALUE, "owned native configuration sharing restriction is established");
    // Always release our own handle before a failing assertion can unwind.
    click(named(QStringLiteral("setupFinish")));
    const bool retained = setupPopup() && general->setupRequired() && !general->lastError().isEmpty()
        && !settings->contains(GeneralPreferencesModel::setupVersionKey());
    CloseHandle(handle);
    check(retained && unchanged(), "actual failed setup save retains drafts modal completion state and paused playback");
    capture(QStringLiteral("native-setup-save-error.png"));
    click(named(QStringLiteral("setupFinish")));
    wait([] { return !setupPopup(); }, "actual Finish retry closes setup after checked native persistence");
    check(!general->setupRequired() && !general->setupReady() && !provider->qmlDialogVisible()
          && general->values().value(QStringLiteral("theme")).toInt() == 1
          && general->values().value(QStringLiteral("accentIndex")).toInt() == 6
          && settings->value(GeneralPreferencesModel::setupVersionKey()).toInt() == 1 && unchanged(),
          "successful native Finish persists Light Purple completion and preserves primary playback");
    capture(QStringLiteral("native-setup-finished.png"));
    report.insert("nativeSetupFinishAccepted", true);
}
