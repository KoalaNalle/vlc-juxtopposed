// SPDX-License-Identifier: GPL-2.0-or-later
// Actual Interface editor and startup title consumer; private GUI only.
void exerciseInterfaceTitle(const QString &mode)
{
    auto *model=context->getInterfacePreferences();
    check(model && model->capabilities().value("titleFileName").toBool(),
          "actual Interface title editor has a linked native consumer");
    const bool off=mode.endsWith(QStringLiteral("-off"));
    const bool initial=mode.endsWith(QStringLiteral("-initial"));
    const bool startup=!off;
    check(context->showMediaNameInTitle()==startup
          && model->values().value("titleFileName").toBool()==startup && !model->restartRequired(),
          "cold startup title policy matches saved preference without stale feedback");
    auto *native=context->getIntf()->p_player;
    const auto start=[&](const QString &filename) {
        const bool play=off && !filename.endsWith(QStringLiteral(".mp4"));
        const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(filename));
        context->getIntf()->p_mainPlaylistController->append(
            QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
        context->setPlayerView(true);
        wait([&]{vlc_player_locker lock(native);const auto *stats=vlc_player_GetStatistics(native);
            return player()->getUrl()==url && stats && (filename.endsWith(QStringLiteral(".mp4"))
                ? stats->i_decoded_video>0 : stats->i_decoded_audio>0);},
             "owned input decodes through the actual main player",15000);
        if(!play){vlc_player_locker lock(native);vlc_player_Pause(native);}
        wait([&]{return player()->getPlayingState()==(play?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
             "title journey reaches its requested playing or paused state");
        if(!play){auto last=nativeTime();QElapsedTimer stable;stable.start();
            wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>1000;},
                 "paused title journey clock settles before editor actions",5000);}
    };
    const auto titleMatches=[&] {
        auto *strip=named(QStringLiteral("juxtopposedStripTitle"));
        return strip && !player()->getName().isEmpty()
            && strip->property("text").toString()==(startup?player()->getName():QStringLiteral("VLC Media Player"))
            && window->title()==(startup?player()->getName():QString{});
    };
    start(QStringLiteral("owned-settings.wav"));
    wait(titleMatches,"actual custom audio strip and native Windows caption obey cold startup policy");
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
    const auto volume=player()->getVolume();const auto mute=player()->isMuted();QElapsedTimer elapsed;elapsed.start();
    const auto unchanged=[&]{return player()->getInput()==input && player()->getRate()==rate
        && player()->getVolume()==volume && player()->isMuted()==mute && titleMatches()
        && context->showMediaNameInTitle()==startup
        && player()->getPlayingState()==(off?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
        && (off?std::abs(nativeTime()-clock-vlc_tick_t(VLC_TICK_FROM_MS(elapsed.elapsed())*rate))<VLC_TICK_FROM_MS(1500):nativeTime()==clock);};
    QTest::qWait(150);check(unchanged(),"no-write title and primary-state control passes");
    auto *provider=DialogsProvider::getInstance();
    const auto open=[&]{
        check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens actual Interface settings host");
        wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category exists");
        click(named(QStringLiteral("globalCategoryinterface")));
        wait([]{return named(QStringLiteral("interfacetitleFileName"));},"actual title toggle materializes");
    };
    const auto reveal=[&]{auto *toggle=named(QStringLiteral("interfacetitleFileName"));
        toggle->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);return toggle;};
    const auto dismiss=[&]{QTest::keyClick(window,Qt::Key_Escape);
        wait([&]{return !provider->qmlDialogVisible();},"Interface settings dismiss normally");};
    open();auto *toggle=reveal();check(toggle->isEnabled(),"actual title toggle is enabled");
    if(initial){
        const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
        check(QFileInfo(config).isFile(),"owned title configuration exists");
        struct TitleSharingLock {HANDLE handle;~TitleSharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}}
            lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
        check(lock.handle!=INVALID_HANDLE_VALUE,"owned title config locks for a genuine save failure");
        click(toggle);
        const bool failed=model->values().value("titleFileName").toBool()==startup && !model->lastError().isEmpty();
        CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
        check(failed && unchanged(),"real locked-save failure rolls back policy and exposes retry feedback");
    }
    click(reveal());
    check(model->values().value("titleFileName").toBool()!=startup && model->lastError().isEmpty()
          && model->restartRequired() && unchanged(),
          "actual title pointer edit saves restart policy without changing current title or playback");
    reveal();QTest::keyClick(window,Qt::Key_Space);QTest::qWait(120);
    check(model->values().value("titleFileName").toBool()==startup && !model->restartRequired() && unchanged(),
          "keyboard restoration clears restart feedback and preserves current title and playback");
    click(reveal());check(model->restartRequired() && unchanged(),"actual title editor leaves the opposite policy for cold reopen");
    dismiss();open();toggle=reveal();
    check(toggle->property("selected").toBool()!=startup && model->restartRequired() && unchanged(),
          "actual Interface dialog reopen retains saved title policy and current startup title");
    dismiss();check(unchanged(),"complete title editor journey preserves primary playback");
    capture(QStringLiteral("native-title-audio.png"));
    start(QStringLiteral("owned-clip.mp4"));
    wait(titleMatches,"same-instance next video keeps startup title policy despite saved opposite default");
    capture(QStringLiteral("native-title-video.png"));
    report.insert("nativeInterfaceTitleAccepted",true);
    report.insert("titleStartupEnabled",startup);
    report.insert("fullBetaAccepted",false);
}
