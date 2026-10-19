// SPDX-License-Identifier: GPL-2.0-or-later
// Opt-in actual-window consumer checks. Run only after human prompt dismissal.
void exerciseFullscreenControls(const QString &mode)
{
    const bool allowed = mode.endsWith(QStringLiteral("-on"));
    const bool video = mode.contains(QStringLiteral("-video-"));
    const auto screen = qEnvironmentVariable("VLC_OWNED_UI_SCREEN");
    check(!screen.isEmpty() && window->screen() && window->screen()->name() == screen,
          "fullscreen journey is confined to the explicitly selected test monitor");
    check(context->showFullscreenControls() == allowed,
          "actual MainCtx snapshots the requested fullscreen-controls startup option");
    const auto url = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(
        video ? QStringLiteral("owned-clip.mp4") : QStringLiteral("first.flac")));
    context->getIntf()->p_mainPlaylistController->append(
        QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded), {}, {}}}, true);
    context->setPlayerView(true);
    wait([&]{ return player()->getUrl() == url && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; },
         "owned media starts through the actual playlist before fullscreen checks", 15000);
    {
        vlc_player_locker lock(context->getIntf()->p_player);
        vlc_player_Pause(context->getIntf()->p_player);
    }
    wait([]{ return player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED; },
         "primary input pauses before independent window changes");
    vlc_tick_t last = nativeTime(); QElapsedTimer stable; stable.start();
    wait([&]{
        const auto value = nativeTime();
        if (value != last) { last = value; stable.restart(); }
        return stable.elapsed() >= 1200;
    }, "native paused clock settles before fullscreen mutation", 5000);
    const auto input = player()->getInput(); const auto clock = nativeTime();
    const auto rate = player()->getRate(); const auto volume = player()->getVolume();
    const auto muted = player()->isMuted();
    QTest::qWait(200); check(nativeTime() == clock, "no-write paused clock control is exact");
    // Video may auto-hide during the paused-clock settling control above.
    // Locate the materialized owner without assuming it is currently visible;
    // then reveal it and use its real visibility lock before testing policy.
    const auto liveBar = []() -> QQuickItem * {
        for (auto *object : objects())
            if (object->objectName() == QStringLiteral("juxtopposedLivePlaybackBar"))
                if (auto *item = qobject_cast<QQuickItem *>(object)) return item;
        return nullptr;
    };
    wait([&]{ return liveBar() != nullptr; },
         "actual redesigned playback bar materializes in the windowed player");
    auto *root = liveBar();
    while (root && root->metaObject()->indexOfProperty("_controlsForcedVisible") < 0) root = root->parentItem();
    check(root != nullptr, "live bar belongs to the actual production Player root");
    const auto objectFor = [&](const QString &id) -> QObject * {
        const auto value = expression(root, id);
        auto *object = value.value<QObject *>();
        if (!object && value.canConvert<QJSValue>()) object = value.value<QJSValue>().toQObject();
        return object;
    };
    auto *bar = qobject_cast<QQuickItem *>(objectFor(QStringLiteral("controlBar")));
    auto *top = qobject_cast<QQuickItem *>(objectFor(QStringLiteral("topBar")));
    auto *fsm = objectFor(QStringLiteral("playerToolbarVisibilityFSM"));
    check(bar && top && fsm, "actual Player exposes its existing bottom/title/visibility owners to the private driver");
    if (!bar->isVisible()) {
        check(QMetaObject::invokeMethod(fsm, "mouseMove"), "normal pointer-reveal signal reaches the actual visibility owner");
        wait([&]{ return bar->isVisible() && bar->isEnabled(); },
             "normal reveal makes auto-hidden windowed video controls available");
    }
    check(QMetaObject::invokeMethod(fsm, "lock"), "actual visibility owner accepts an interaction lock");
    wait([&]{ return bar->isVisible() && bar->isEnabled(); },
         "real visibility lock keeps the revealed windowed controls available");
    bar->forceActiveFocus(Qt::TabFocusReason);
    check(bar->hasActiveFocus(), "windowed bottom controls own keyboard focus before fullscreen transition");
    window->showFullScreen();
    wait([&]{ return window->visibility() == QWindow::FullScreen && bar->isVisible() == allowed; },
         "actual fullscreen bottom-control visibility follows the saved option");
    check(bar->isEnabled() == allowed, "hidden fullscreen controls reject input during and after fading");
    check(top->isVisible(), "title and exit controls remain available under the independent visibility lock");
    if (!allowed) {
        check(!bar->hasActiveFocus() && top->hasActiveFocus(),
              "fullscreen hiding returns focused bottom controls to the visible title controls");
    }
    QTest::mouseMove(window, QPoint(window->width()/2, window->height()-5));
    QTest::keyClick(window, Qt::Key_Tab); QTest::qWait(100);
    check(bar->isVisible() == allowed && bar->isEnabled() == allowed,
          "actual pointer/keyboard activity cannot override the fullscreen option");
    capture(QStringLiteral("native-fullscreen-controls.png"));
    check(QMetaObject::invokeMethod(fsm, "unlock"), "actual interaction lock releases normally");
    window->showNormal();
    QMetaObject::invokeMethod(fsm, "mouseMove");
    wait([&]{ return window->visibility() != QWindow::FullScreen && bar->isVisible() && bar->isEnabled(); },
         "windowed playback controls return regardless of the fullscreen setting");
    check(player()->getInput() == input && nativeTime() == clock && player()->getRate() == rate
          && player()->getVolume() == volume && player()->isMuted() == muted
          && player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED,
          "fullscreen policy preserves exact paused input clock rate volume and mute");
    check(window->screen() && window->screen()->name() == screen,
          "fullscreen exit leaves the owned test window on the selected monitor");
    capture(QStringLiteral("native-windowed-controls-return.png"));
    report.insert("nativeFullscreenControlsAccepted", true);
}
