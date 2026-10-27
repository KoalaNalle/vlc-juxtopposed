// SPDX-License-Identifier: GPL-2.0-or-later
// Actual geometry preference editor and current-versus-next-startup contract.
void exerciseGeometrySettings(const QString &mode)
{
    const bool off=mode.endsWith(QStringLiteral("-off"));
    const bool initial=mode.endsWith(QStringLiteral("-initial"));
    const bool playing=mode.endsWith(QStringLiteral("-playing"));
    const bool startup=!off;
    auto *model=context->getInterfacePreferences();
    check(model && model->capabilities().value("rememberGeometry").toBool(),"actual geometry editor has a linked window consumer");
    check(context->rememberWindowGeometry()==startup && model->values().value("rememberGeometry").toBool()==startup
          && !model->restartRequired(),"cold geometry startup policy agrees with saved setting without stale feedback");
    const auto raw=context->getIntf()->mainSettings->value("geometry").toByteArray();
    QDataStream stream(raw);stream.setVersion(QDataStream::Qt_4_0);
    quint32 magic;quint8 major,minor,maximized,fullscreen;QString screenName;qreal dpr;QRect screenRect,savedClient;QPoint position;
    stream>>magic>>major>>minor>>screenName>>dpr>>screenRect>>position>>savedClient>>maximized>>fullscreen;
    check(stream.status()==QDataStream::Ok && magic==0x1D94200 && major==1 && !maximized && !fullscreen,
          "geometry editor owns a valid saved normal-window fixture");
    const auto startupReport=report.value("startupGeometry").toObject();
    const auto saved=QJsonArray{savedClient.x(),savedClient.y(),savedClient.width(),savedClient.height()};
    if(startup)check(startupReport.value("client").toArray()==saved && startupReport.value("screen").toString()==screenName,
                     "actual cold main window restores exact saved normal bounds before test relocation");
    else {
        const auto client=startupReport.value("client").toArray();QScreen *launch=nullptr;
        for(auto *screen:QGuiApplication::screens())if(screen->name()==startupReport.value("screen").toString())launch=screen;
        check(launch && client.size()==4 && client[2].toInt()==600 && client[3].toInt()==420
              && client!=saved,"actual cold Off startup ignores saved rectangle and uses default600x420 bounds");
        const auto expected=launch->availableGeometry().center()-QPoint(300,210);
        check(QPoint(client[0].toInt(),client[1].toInt())==expected,"actual cold Off window centers on its real launch screen");
    }
    auto *native=context->getIntf()->p_player;
    const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("owned-settings.wav")));
    context->getIntf()->p_mainPlaylistController->append(
        QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
    context->setPlayerView(true);
    wait([&]{vlc_player_locker lock(native);const auto *stats=vlc_player_GetStatistics(native);
        return player()->getUrl()==url && stats && stats->i_decoded_audio>0;},"owned audio decodes through actual primary player",15000);
    if(!playing){vlc_player_locker lock(native);vlc_player_Pause(native);}
    wait([&]{return player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
         "geometry editor reaches requested playing or paused state");
    if(!playing){auto last=nativeTime();QElapsedTimer stable;stable.start();
        wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>700;},"paused geometry editor clock settles",5000);}
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
    const auto volume=player()->getVolume();const auto mute=player()->isMuted();QElapsedTimer elapsed;elapsed.start();
    const auto queueState=[&]{auto *queue=context->getIntf()->p_playlist;vlc::playlist::vlc_playlist_locker lock(queue);QVector<quint64> ids;
        for(size_t i=0;i<vlc_playlist_Count(queue);++i)ids.push_back(vlc_playlist_item_GetId(vlc_playlist_Get(queue,i)));
        return qMakePair(ids,vlc_playlist_GetCurrentIndex(queue));};
    const auto tracksState=[&]{vlc_player_locker lock(native);QVector<const vlc_es_id_t *> ids;
        for(auto category:{AUDIO_ES,VIDEO_ES,SPU_ES})for(size_t i=0;i<vlc_player_GetTrackCount(native,category);++i){
            const auto *track=vlc_player_GetTrackAt(native,category,i);if(track->selected)ids.push_back(track->es_id);}return ids;};
    const auto queue=queueState();const auto tracks=tracksState();const auto geometry=window->geometry();
    const auto unchanged=[&]{return player()->getInput()==input && player()->getRate()==rate && player()->getVolume()==volume
        && player()->isMuted()==mute && queueState()==queue && tracksState()==tracks && context->rememberWindowGeometry()==startup
        && window->geometry()==geometry
        && player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
        && (playing?std::abs(nativeTime()-clock-vlc_tick_t(VLC_TICK_FROM_MS(elapsed.elapsed())*rate))<VLC_TICK_FROM_MS(1500):nativeTime()==clock);};
    QTest::qWait(120);check(unchanged(),"geometry editor no-write state control passes");
    for(const QVariant &bad:{QVariant(1),QVariant(0.0),QVariant(QStringLiteral("false"))}) {
        check(!model->setValue(QStringLiteral("rememberGeometry"),bad) && model->values().value("rememberGeometry").toBool()==startup
              && !model->restartRequired() && unchanged(),"invalid geometry toggle type preserves saved and active state");
    }
    model->reload();auto *provider=DialogsProvider::getInstance();
    const auto open=[&]{check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens actual Interface host");
        wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category materializes");
        click(named(QStringLiteral("globalCategoryinterface")));
        wait([]{return named(QStringLiteral("interfacerememberGeometry"));},"actual geometry toggle materializes");};
    const auto reveal=[&]{auto *toggle=named(QStringLiteral("interfacerememberGeometry"));toggle->forceActiveFocus(Qt::TabFocusReason);
        QTest::qWait(150);check(toggle->isEnabled() && toggle->hasActiveFocus(),"actual geometry toggle is enabled and keyboard focused");return toggle;};
    const auto dismiss=[&]{QTest::keyClick(window,Qt::Key_Escape);wait([&]{return !provider->qmlDialogVisible();},"geometry settings dismiss normally");};
    open();auto *toggle=reveal();
    if(initial){const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
        struct SharingLock {HANDLE handle;~SharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}}
            lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
        check(lock.handle!=INVALID_HANDLE_VALUE,"owned geometry config locks for real save failure");click(toggle);
        const bool failed=model->values().value("rememberGeometry").toBool()==startup && !model->lastError().isEmpty();
        CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
        check(failed && !model->restartRequired() && unchanged(),"real locked-save failure rolls back policy and exposes retry feedback");
        auto *error=named(QStringLiteral("interfaceSettingsError"));
        check(error && error->isVisible(),"actual geometry save failure is visible in Interface settings");}
    click(reveal());check(model->values().value("rememberGeometry").toBool()!=startup && model->lastError().isEmpty()
        && model->restartRequired() && unchanged(),"actual geometry pointer edit saves next-start policy without moving current window or playback");
    reveal();QTest::keyClick(window,Qt::Key_Space);QTest::qWait(150);
    check(model->values().value("rememberGeometry").toBool()==startup && !model->restartRequired() && unchanged(),
          "keyboard restore clears restart feedback and preserves current window and playback");
    if(!playing)click(reveal());
    dismiss();open();toggle=reveal();
    check(toggle->property("selected").toBool()==(playing?startup:!startup)
          && model->restartRequired()==!playing && unchanged(),"actual geometry dialog reopen retains saved target and current startup policy");
    capture(QStringLiteral("native-geometry-editor.png"));dismiss();check(unchanged(),"complete geometry editor preserves active primary state");
    auto *screen=window->screen();window->setGeometry(QRect(screen->availableGeometry().topLeft()+QPoint(66,73),QSize(1120,760)));
    QTest::qWait(150);const auto exit=window->geometry();
    report.insert("exitGeometry",QJsonArray{exit.x(),exit.y(),exit.width(),exit.height()});
    report.insert("savedGeometryBeforeExitSha256",QString::fromLatin1(QCryptographicHash::hash(raw,QCryptographicHash::Sha256).toHex()));
    report.insert("startupRememberGeometry",startup);report.insert("nativeGeometryEditorAccepted",true);report.insert("fullBetaAccepted",false);
}
