// SPDX-License-Identifier: GPL-2.0-or-later
// Actual preference editor and real player-name -> Windows tray delivery.
void exercisePopupDuration(const QString &mode)
{
    const bool editor=mode.startsWith(QStringLiteral("popup-editor-"));
    const bool initial=mode.endsWith(QStringLiteral("-initial"));
    const bool reopen=mode.endsWith(QStringLiteral("-reopen"));
    const bool playing=mode.endsWith(QStringLiteral("-playing"));
    const bool minimized=mode==QStringLiteral("popup-minimized");
    const bool never=mode==QStringLiteral("popup-never");
    const bool trayOff=mode==QStringLiteral("popup-tray-off");
    const int startup=reopen?0:mode==QStringLiteral("popup-cold-max")?10000:777;
    auto *model=context->getInterfacePreferences();auto *native=context->getIntf()->p_player;
    auto *tray=context->getSysTray();
    check(QSystemTrayIcon::isSystemTrayAvailable(),"real Windows system tray is available");
    check(model && model->capabilities().value("popupDuration").toBool(),"actual popup duration consumer is independently linked");
    check(context->notificationDuration()==startup && model->values().value("popupDuration").toInt()==startup
          && !model->restartRequired(),"actual startup duration agrees with saved native milliseconds without stale feedback");
    check(trayOff?!tray:tray && tray->isVisible(),"actual tray creation obeys cold startup preference");
    QSignalSpy names(player(),&PlayerController::nameChanged);QJsonArray expected,transitions;
    const auto start=[&](const QString &file,bool notify){
        const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(file));
        const int before=names.count();const int beforeVisibility=int(context->interfaceVisibility());
        context->getIntf()->p_mainPlaylistController->append(
            QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
        context->setPlayerView(true);
        wait([&]{vlc_player_locker lock(native);const auto *stats=vlc_player_GetStatistics(native);
            return player()->getUrl()==url && player()->getName()==file && names.count()>before && stats && stats->i_decoded_audio>0;},
            "genuine new input decodes and emits its actual player name",15000);
        if(!playing){vlc_player_locker lock(native);vlc_player_Pause(native);}
        wait([&]{return player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
             "notification journey reaches requested primary playback state");
        if(!playing){auto last=nativeTime();QElapsedTimer stable;stable.start();
            wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>700;},"notification paused clock settles",5000);}
        if(notify)expected.append(file);
        transitions.append(QJsonObject{{"name",file},{"notificationExpected",notify},{"visibilityBefore",beforeVisibility},
            {"visibilityAfter",int(context->interfaceVisibility())},{"nameSignals",names.count()-before},{"startupDuration",context->notificationDuration()}});
        check(context->notificationDuration()==startup && context->getSysTray()==tray,"actual track change retains the original tray and duration snapshot");
    };
    start(QStringLiteral("owned-settings.wav"),!never && !trayOff && !minimized);
    if(editor){
        const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
        const auto volume=player()->getVolume();const auto mute=player()->isMuted();QElapsedTimer elapsed;elapsed.start();
        const auto queueState=[&]{auto *queue=context->getIntf()->p_playlist;vlc::playlist::vlc_playlist_locker lock(queue);QVector<quint64> ids;
            for(size_t i=0;i<vlc_playlist_Count(queue);++i)ids.push_back(vlc_playlist_item_GetId(vlc_playlist_Get(queue,i)));
            return qMakePair(ids,vlc_playlist_GetCurrentIndex(queue));};
        const auto trackState=[&]{vlc_player_locker lock(native);QVector<const vlc_es_id_t *> ids;
            for(auto category:{AUDIO_ES,VIDEO_ES,SPU_ES})for(size_t i=0;i<vlc_player_GetTrackCount(native,category);++i){
                const auto *track=vlc_player_GetTrackAt(native,category,i);if(track->selected)ids.push_back(track->es_id);}return ids;};
        const auto queue=queueState();const auto tracks=trackState();
        const auto unchanged=[&]{return player()->getInput()==input && player()->getRate()==rate && player()->getVolume()==volume
            && player()->isMuted()==mute && queueState()==queue && trackState()==tracks && context->notificationDuration()==startup
            && context->getSysTray()==tray && tray->isVisible()
            && player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
            && (playing?std::abs(nativeTime()-clock-vlc_tick_t(VLC_TICK_FROM_MS(elapsed.elapsed())*rate))<VLC_TICK_FROM_MS(1500):nativeTime()==clock);};
        check(unchanged(),"popup editor no-write primary-state control passes");
        for(const QVariant &bad:{QVariant(true),QVariant(QStringLiteral("500")),QVariant(-1),QVariant(10001),QVariant(1.5),
                                  QVariant(std::numeric_limits<double>::quiet_NaN())})
            check(!model->setValue(QStringLiteral("popupDuration"),bad) && model->values().value("popupDuration").toInt()==startup
                  && !model->restartRequired() && unchanged(),"invalid duration type or range cannot change saved or active state");
        model->reload();auto *provider=DialogsProvider::getInstance();
        const auto open=[&]{check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens actual Interface host");
            wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category materializes");click(named(QStringLiteral("globalCategoryinterface")));
            wait([]{return named(QStringLiteral("interfacepopupDuration"));},"actual popup-duration slider materializes");};
        const auto groupVisible=[&]{auto *row=named(QStringLiteral("interfacepopupDurationRow"));
            auto *viewport=named(QStringLiteral("globalSettingsPageScroll"));
            check(row && viewport,"actual popup group and scrolling viewport exist");
            const auto rect=row->mapRectToItem(viewport,row->boundingRect());
            check(rect.top()>=-0.51 && rect.bottom()<=viewport->height()+0.51,
                  "focused popup title scale and timing note are fully inside actual viewport");};
        const auto reveal=[&]{auto *slider=named(QStringLiteral("interfacepopupDuration"));slider->forceActiveFocus(Qt::TabFocusReason);
            QTest::qWait(150);check(slider->isEnabled() && slider->hasActiveFocus(),"actual popup slider is enabled and keyboard focused");groupVisible();return slider;};
        const auto dismiss=[&]{QTest::keyClick(window,Qt::Key_Escape);wait([&]{return !provider->qmlDialogVisible();},"notification settings dismiss normally");};
        open();auto *slider=reveal();
        check(std::abs(slider->property("value").toDouble()-startup/1000.)<0.00001
              && model->values().value("popupDuration").toInt()==startup && !model->restartRequired(),
              "opening the actual slider preserves arbitrary native milliseconds without rounding or writes");
        auto *note=named(QStringLiteral("interfacepopupDurationTimingNote"));
        check(note && note->isVisible() && note->property("text").toString().contains(QStringLiteral("system default"))
              && expression(slider,QStringLiteral("Accessible.description")).toString().contains(QStringLiteral("system default")),
              "actual visible and accessible duration help explains zero and system timing");
        if(initial){const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
            struct SharingLock {HANDLE handle;~SharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}}
                lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
            check(lock.handle!=INVALID_HANDLE_VALUE,"owned notification config locks for genuine save failure");
            QTest::keyClick(window,Qt::Key_Right);QTest::qWait(100);
            const bool failed=model->values().value("popupDuration").toInt()==startup && !model->lastError().isEmpty();
            CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
            check(failed && !model->restartRequired() && unchanged(),"real locked-save failure rolls back policy and exposes retry feedback");
            check(std::abs(slider->property("value").toDouble()-startup/1000.)<0.00001,"failed keyboard save restores the actual slider value");
            auto *error=named(QStringLiteral("interfaceSettingsError"));check(error && error->isVisible(),"actual notification save failure is visible");}
        groupVisible();
        click(reveal());const int pointerValue=model->values().value("popupDuration").toInt();
        check(pointerValue!=startup && pointerValue>=0 && pointerValue<=10000 && model->lastError().isEmpty()
              && model->restartRequired() && unchanged(),"actual pointer duration edit saves next-start policy without changing primary state");
        check(model->setValue(QStringLiteral("popupDuration"),startup) && !model->restartRequired() && unchanged(),
              "restoring the exact native value clears restart feedback without rounding legacy milliseconds");
        const int target=initial?0:10000;reveal();
        for(int i=0;i<22 && model->values().value("popupDuration").toInt()!=target;++i){
            QTest::keyClick(window,target==0?Qt::Key_Left:Qt::Key_Right);QTest::qWait(80);}
        check(model->values().value("popupDuration").toInt()==target && model->restartRequired() && unchanged(),
              "actual slider keyboard edits reach the exact duration boundary and preserve primary state");
        auto *systemTray=named(QStringLiteral("interfacesystemTray"));check(systemTray && systemTray->isEnabled(),"actual tray dependency toggle exists");
        systemTray->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(150);click(systemTray);
        check(!model->values().value("systemTray").toBool() && !slider->isEnabled()
              && model->values().value("popupDuration").toInt()==target && unchanged(),"saved tray Off disables the editor and retains duration and active tray until restart");
        click(systemTray);QTest::qWait(100);check(slider->isEnabled() && unchanged(),"restoring saved tray On restores the existing editor without replacing the active tray");
        dismiss();open();slider=reveal();
        check(model->values().value("popupDuration").toInt()==target && std::abs(slider->property("value").toDouble()-target/1000.)<0.00001
              && model->restartRequired() && unchanged(),"actual notification dialog reopen retains saved boundary and original active duration");
        capture(QStringLiteral("native-popup-duration-editor.png"));dismiss();check(unchanged(),"complete notification editor preserves active primary media");
        start(QStringLiteral("owned-popup-next.wav"),true);
        report.insert("savedNextStartupDuration",target);
    }else if(minimized){
        window->showMinimized();wait([&]{return context->interfaceVisibility()==QWindow::Minimized;},"actual window and MainCtx reach minimized state");
        start(QStringLiteral("owned-popup-next.wav"),true);
        check(context->interfaceVisibility()==QWindow::Minimized,"new input preserves the real minimized state");
        context->requestInterfaceNormal();window->requestActivate();wait([&]{return context->interfaceVisibility()==QWindow::Windowed;},"actual window restores before next normal track");
        start(QStringLiteral("owned-popup-restored.wav"),false);
    }else{
        if(trayOff){auto *provider=DialogsProvider::getInstance();check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens cold tray-Off host");
            wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category materializes");click(named(QStringLiteral("globalCategoryinterface")));
            wait([]{return named(QStringLiteral("interfacepopupDuration"));},"cold tray-Off duration row materializes");
            check(!named(QStringLiteral("interfacepopupDuration"))->isEnabled() && model->values().value("popupDuration").toInt()==startup,
                  "cold absent tray disables duration editor without discarding its saved value");QTest::keyClick(window,Qt::Key_Escape);}
        start(QStringLiteral("owned-popup-next.wav"),!never && !trayOff);
    }
    report.insert("expectedNotificationNames",expected);report.insert("notificationTransitions",transitions);
    report.insert("startupNotificationDuration",startup);report.insert("nativePopupDurationAccepted",true);report.insert("fullBetaAccepted",false);
}
