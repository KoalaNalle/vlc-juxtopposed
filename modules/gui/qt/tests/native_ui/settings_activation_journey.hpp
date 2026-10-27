// SPDX-License-Identifier: GPL-2.0-or-later
// Actual settings controls and consumers; included only in the private GUI.
void exerciseSettingsActivation(const QString &mode)
{
    auto *interface = context->getInterfacePreferences();
    auto *audio = context->getAudioPreferences();
    check(interface && audio, "actual MainCtx owns both preference services");
    check(interface->capabilities().value("fullscreenControls").toBool()
          && audio->capabilities().value("volumeStep").toBool(),
          "the verified fullscreen and authored volume-step gates remain enabled");
    check(audio->capabilities().value("outputDevice").toBool()
          && audio->capabilities().value("startupVolume").toBool(),
          "verified Windows device and startup-volume consumers are linked independently");
    check(!audio->capabilities().value("passthrough").toBool(),
          "receiver acceptance remains independent and unavailable");
    const bool reopened = mode.endsWith(QStringLiteral("-reopen"));
    const bool playing = mode.endsWith(QStringLiteral("-playing"));
    if (reopened) {
        check(!interface->values().value("fullscreenControls").toBool()
              && !context->showFullscreenControls() && !interface->restartRequired(),
              "cold process consumes the saved fullscreen Off without a stale restart warning");
        check(std::abs(audio->values().value("volumeStep").toDouble() - 5.) < .00001
              && !audio->restartRequired(),
              "cold process restores the saved five-point volume step without a restart warning");
        exerciseFullscreenControls(QStringLiteral("fullscreen-controls-audio-off"));
    } else {
        const auto url = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"))
                                               .filePath(QStringLiteral("owned-settings.wav")));
        context->getIntf()->p_mainPlaylistController->append(
            QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded), {}, {}}}, true);
        context->setPlayerView(true);
        wait([&]{ return player()->getUrl() == url && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; },
             "owned long media starts through the actual playlist", 15000);
        if (!playing) {
            vlc_player_locker lock(context->getIntf()->p_player);
            vlc_player_Pause(context->getIntf()->p_player);
        }
        wait([&]{ return player()->getPlayingState() == (playing ? PlayerController::PLAYING_STATE_PLAYING
                                                                : PlayerController::PLAYING_STATE_PAUSED); },
             "primary reaches the requested playing or paused state");
        if (!playing) {
            vlc_tick_t last = nativeTime(); QElapsedTimer stable; stable.start();
            wait([&]{ auto value = nativeTime(); if (value != last) { last=value; stable.restart(); }
                      return stable.elapsed() >= 1000; }, "paused native clock settles before editor actions", 5000);
        }
    }
    const auto input = player()->getInput(); const auto clock = nativeTime();
    const auto rate = player()->getRate(); const auto volume = player()->getVolume(); const auto mute = player()->isMuted();
    const auto state = player()->getPlayingState();
    const auto unchanged = [&]{ return player()->getInput() == input && player()->getRate() == rate
        && player()->getVolume() == volume && player()->isMuted() == mute && player()->getPlayingState() == state
        && (playing ? nativeTime() >= clock : nativeTime() == clock); };
    QTest::qWait(150); check(unchanged(), "no-write state control passes before actual settings input");
    auto *provider = DialogsProvider::getInstance();
    const auto open = [&]{
        check(QMetaObject::invokeMethod(provider, "showGlobalSettingsDialog"), "native route opens the real settings host");
        wait([]{ return named(QStringLiteral("globalCategoryinterface")); }, "actual category buttons materialize");
    };
    const auto category = [&](const QString &key, const QString &pageName){
        click(named(QStringLiteral("globalCategory") + key));
        wait([&]{ return named(pageName); }, "actual requested category loads with its production service");
    };
    const auto reveal = [&](const QString &name){
        auto *item=named(name); check(item, "actual settings editor exists");
        item->forceActiveFocus(Qt::TabFocusReason); QTest::qWait(150); return item;
    };
    open(); category(QStringLiteral("interface"), QStringLiteral("juxtopposedInterfaceSettings"));
    auto *toggle=reveal(QStringLiteral("interfacefullscreenControls"));
    check(toggle->isEnabled(), "actual fullscreen switch is enabled in the ordinary host");
    if (!reopened) {
        const bool initial=interface->values().value("fullscreenControls").toBool();
        const bool startup=context->showFullscreenControls();
        click(toggle);
        wait([&]{ return interface->values().value("fullscreenControls").toBool() != initial; },
             "actual pointer toggle commits the fullscreen preference");
        check(context->showFullscreenControls() == startup && interface->restartRequired(),
              "fullscreen editor changes the startup policy without changing this running player's policy");
        check(named(QStringLiteral("interfaceSettingsRestart")), "fullscreen editor presents its real restart feedback");
        wait([&]{
            for (auto *parent=toggle->parentItem(); parent; parent=parent->parentItem())
                if (parent->clip() && !parent->boundingRect().adjusted(-.5,-.5,.5,.5).contains(
                        toggle->mapRectToItem(parent,toggle->boundingRect()))) return false;
            return true;
        }, "focused fullscreen switch remains fully inside clipped viewports after restart feedback reflows");
        QTest::keyClick(window, Qt::Key_Space);
        wait([&]{ return interface->values().value("fullscreenControls").toBool() == initial; },
             "keyboard activation restores the starting fullscreen value");
        check(!interface->restartRequired(), "restoring the starting fullscreen value clears restart feedback");
        if (initial) click(toggle); // Leave Off for the independent cold-process check.
        check(!interface->values().value("fullscreenControls").toBool(), "actual editor leaves the owned fullscreen policy Off");
    }
    check(unchanged(), "fullscreen preference editing preserves the primary state");
    capture(QStringLiteral("native-fullscreen-settings-editor.png"));
    category(QStringLiteral("audio"), QStringLiteral("juxtopposedAudioSettings"));
    auto *field=reveal(QStringLiteral("audiovolumeStep"));
    check(field->isEnabled(), "actual authored volume-step input is enabled");
    check(!audio->restartRequired(), "unrelated fullscreen restart feedback does not leak into Audio");
    const auto type = [&](const QString &value){
        click(reveal(QStringLiteral("audiovolumeStep")));
        QTest::keyClick(window, Qt::Key_A, Qt::ControlModifier);
        for (const auto character : value) {
            const auto key=character == QLatin1Char('.') ? Qt::Key_Period
                : static_cast<Qt::Key>(Qt::Key_0 + character.digitValue());
            QTest::keyClick(window,key);
        }
        QTest::keyClick(window, Qt::Key_Return); QTest::qWait(100);
    };
    if (!reopened) {
        const auto previous=audio->values().value("volumeStep").toDouble();
        type(QStringLiteral("0"));
        check(audio->values().value("volumeStep").toDouble() == previous && !audio->lastError().isEmpty(),
              "actual editor refuses an out-of-range step without changing native policy");
        type(QStringLiteral("2.5"));
        check(std::abs(audio->values().value("volumeStep").toDouble() - 2.5) < .00001
              && audio->lastError().isEmpty() && !audio->restartRequired(),
              "actual numeric keyboard edit saves immediately without a false restart warning");
        check(unchanged(), "editing the live step does not itself change volume or primary playback");
        player()->setVolume(.4f);
        wait([]{ return std::abs(player()->getVolume()-.4f) < .002f; }, "real player accepts a known volume baseline");
        player()->setVolumeUp();
        wait([]{ return std::abs(player()->getVolume()-.425f) < .002f; },
             "next native volume action immediately consumes the editor's two-and-a-half-point step");
        player()->setVolumeDown();
        wait([]{ return std::abs(player()->getVolume()-.4f) < .002f; }, "native down action reverses the exact step");
        player()->setVolume(volume);
        wait([&]{ return player()->getVolume()==volume; }, "owned fixture restores the exact primary volume");
        const auto path=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
        check(!path.isEmpty() && QFileInfo(path).isFile(), "owned native config exists before genuine write failure");
        const auto handle=CreateFileW(path.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);
        check(handle != INVALID_HANDLE_VALUE, "owned config can be locked for actual save failure");
        type(QStringLiteral("5"));
        const bool failed=std::abs(audio->values().value("volumeStep").toDouble()-2.5)<.00001
            && !audio->lastError().isEmpty();
        CloseHandle(handle);
        check(failed, "real locked-save failure rolls back policy and exposes retry feedback");
        type(QStringLiteral("5"));
        check(std::abs(audio->values().value("volumeStep").toDouble()-5.)<.00001
              && audio->lastError().isEmpty() && !audio->restartRequired(),
              "same actual editor retries and saves the five-point policy");
        player()->setVolume(.4f);
        wait([]{ return std::abs(player()->getVolume()-.4f)<.002f; }, "second volume action has a settled baseline");
        player()->setVolumeUp();
        wait([]{ return std::abs(player()->getVolume()-.45f)<.002f; },
             "native action uses the changed five-point policy without restarting");
        player()->setVolume(volume);
        wait([&]{ return player()->getVolume()==volume; }, "second live-action check restores primary volume");
    }
    check(unchanged(), "Audio editor validation save retry and live actions preserve input rate mute and playback state");
    capture(QStringLiteral("native-volume-step-settings-editor.png"));
    click(named(QStringLiteral("globalSettingsClose")));
    wait([&]{ return !provider->qmlDialogVisible(); }, "actual settings Close dismisses normally");
    open(); category(QStringLiteral("audio"), QStringLiteral("juxtopposedAudioSettings"));
    field=reveal(QStringLiteral("audiovolumeStep"));
    check(std::abs(field->property("text").toString().toDouble()-5.)<.00001,
          "reopening the real Audio page displays the saved step");
    QTest::keyClick(window,Qt::Key_Escape);
    wait([&]{ return !provider->qmlDialogVisible(); }, "Escape closes immediate-write settings normally");
    check(unchanged(), "settings reopen and Escape preserve primary playback");
    if (!reopened) {
        auto *slider=named(QStringLiteral("juxtopposedVolume"));
        check(slider && slider->isEnabled(), "actual live volume slider exists after the settings dialog closes");
        named(QStringLiteral("juxtopposedStripmenu"))->forceActiveFocus(Qt::OtherFocusReason);
        QTest::qWait(100);
        const auto point=slider->mapToScene(slider->boundingRect().center()).toPoint();
        // Do not use click(): it forces keyboard focus before pointer input and
        // would conceal the difference between mouse focus and visual focus.
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point);QTest::qWait(150);
        check(slider->hasActiveFocus(), "direct pointer selection gives the actual volume slider active focus");
        const auto beforeVolume=player()->getVolume();const auto beforeClock=nativeTime();
        const auto step=audio->values().value("volumeStep").toDouble()/100.;
        QTest::keyClick(window,Qt::Key_Right);
        const bool adjusted=QTest::qWaitFor([&]{return std::abs(player()->getVolume()-beforeVolume-step)<.002;},1500);
        report.insert("volumePointerKey",QJsonObject{{"beforeVolume",beforeVolume},{"afterVolume",player()->getVolume()},
            {"step",step},{"beforeClock",double(beforeClock)},{"afterClock",double(nativeTime())},
            {"activeFocus",slider->hasActiveFocus()},{"visualFocus",slider->property("visualFocus").toBool()}});
        check(adjusted && (playing ? nativeTime()>=beforeClock : nativeTime()==beforeClock),
              "pointer-focused volume owns Right without seeking primary playback");
        player()->setVolume(volume);
        wait([&]{return player()->getVolume()==volume;}, "pointer-key fixture restores the primary volume");
    }
    report.insert("nativeSettingsActivationAccepted",true);
    report.insert("settingsImmediateWriteNoDraft",true);
}
