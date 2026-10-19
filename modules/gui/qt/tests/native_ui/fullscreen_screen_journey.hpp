// SPDX-License-Identifier: GPL-2.0-or-later
// Actual preference editor and main-window handler; no visible monitor-1 use.
void exerciseFullscreenScreen(const QString &mode)
{
    const bool initial=mode.endsWith(QStringLiteral("-initial"));
    const bool stale=mode.endsWith(QStringLiteral("-stale"));
    const bool explicitChoice=mode.contains(QStringLiteral("-explicit-"));
    const bool video=mode.endsWith(QStringLiteral("-video"));
    const bool playing=mode.endsWith(QStringLiteral("-playing"));
    const auto screens=QGuiApplication::screens();auto *target=window->screen();
    const auto sampleWindow=[&](const char *phase){const auto geometry=window->geometry();const auto margins=window->frameMargins();
        auto samples=report.value("windowSamples").toArray();samples.append(QJsonObject{{"phase",QString::fromLatin1(phase)},
            {"screenIndex",screens.indexOf(window->screen())},{"qtVisible",window->isVisible()},
            {"windowsVisible",bool(IsWindowVisible(reinterpret_cast<HWND>(window->winId())))},
            {"states",int(window->windowStates())},{"holders",int(window->property("__windowFullScreen").toUInt())},
            {"geometry",QJsonArray{geometry.x(),geometry.y(),geometry.width(),geometry.height()}},
            {"frameMargins",QJsonArray{margins.left(),margins.top(),margins.right(),margins.bottom()}},
            {"devicePixelRatio",window->devicePixelRatio()}});report.insert("windowSamples",samples);};
    const int targetIndex=screens.indexOf(target);
    const int startup=stale?999:explicitChoice?targetIndex:-1;
    auto *model=context->getInterfacePreferences();
    check(target && target->name()==qEnvironmentVariable("VLC_OWNED_UI_SCREEN") && targetIndex>=0,
          "fullscreen screen journey is confined to the approved visible monitor");
    window->showNormal();
    wait([&]{return window->visibility()==QWindow::Windowed && window->screen()==target;},
         "screen journey starts from a normal window on the approved monitor");
    check(context->fullscreenScreen()==startup && model->values().value("fullscreenScreen").toInt()==startup
          && !model->restartRequired(),"actual MainCtx and model restore the exact cold screen policy");
    const auto rows=model->screens();check(rows.size()==screens.size(),"screen choices enumerate genuine connected Qt screens");
    for(int i=0;i<rows.size();++i)check(rows[i].toMap().value("index").toInt()==i
        && rows[i].toMap().value("name").toString()==screens[i]->name(),"screen choice index and label agree with the actual Qt output");
    auto *native=context->getIntf()->p_player;
    const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(
        video?QStringLiteral("owned-controls.mp4"):QStringLiteral("owned-settings.wav")));
    context->getIntf()->p_mainPlaylistController->append(
        QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
    context->setPlayerView(true);
    wait([&]{vlc_player_locker lock(native);const auto *stats=vlc_player_GetStatistics(native);
        return player()->getUrl()==url && stats && (video?stats->i_decoded_video>0:stats->i_decoded_audio>0);},
         "owned media decodes through the actual primary player",15000);
    if(!playing){vlc_player_locker lock(native);vlc_player_Pause(native);}
    wait([&]{return player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
         "screen journey reaches its requested playing or paused state");
    if(!playing){auto last=nativeTime();QElapsedTimer stable;stable.start();
        wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>1000;},
             "paused native clock settles before screen policy changes",5000);}
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
    const auto volume=player()->getVolume();const auto mute=player()->isMuted();QElapsedTimer elapsed;elapsed.start();
    const auto queueState=[&]{auto *queue=context->getIntf()->p_playlist;vlc::playlist::vlc_playlist_locker lock(queue);
        QVector<quint64> ids;for(size_t i=0;i<vlc_playlist_Count(queue);++i)ids.push_back(vlc_playlist_item_GetId(vlc_playlist_Get(queue,i)));
        return qMakePair(ids,vlc_playlist_GetCurrentIndex(queue));};
    const auto selectedTracks=[&]{vlc_player_locker lock(native);QVector<const vlc_es_id_t *> ids;
        for(auto category:{AUDIO_ES,VIDEO_ES,SPU_ES})for(size_t i=0;i<vlc_player_GetTrackCount(native,category);++i){
            const auto *track=vlc_player_GetTrackAt(native,category,i);if(track->selected)ids.push_back(track->es_id);}
        return ids;};
    const auto queue=queueState();const auto tracks=selectedTracks();
    const auto unchanged=[&]{return player()->getInput()==input && context->fullscreenScreen()==startup
        && player()->getRate()==rate && player()->getVolume()==volume && player()->isMuted()==mute
        && queueState()==queue && selectedTracks()==tracks
        && player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
        && (playing?std::abs(nativeTime()-clock-vlc_tick_t(VLC_TICK_FROM_MS(elapsed.elapsed())*rate))<VLC_TICK_FROM_MS(1500):nativeTime()==clock);};
    QTest::qWait(150);check(unchanged(),"no-write primary state control passes before screen editing");
    auto *provider=DialogsProvider::getInstance();
    const auto open=[&]{check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens actual Interface host");
        wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category exists");
        click(named(QStringLiteral("globalCategoryinterface")));
        wait([]{return named(QStringLiteral("interfacefullscreenScreen"));},"actual fullscreen screen editor materializes");};
    const auto reveal=[&]{auto *item=named(QStringLiteral("interfacefullscreenScreen"));
        check(item && item->isEnabled(),"actual fullscreen screen combo is enabled");
        item->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);return item;};
    const auto select=[&](int value,bool pointer){auto *combo=reveal();click(combo);
        wait([&]{return expression(combo,QStringLiteral("popup.visible")).toBool();},"actual screen popup opens");
        const int index=value+1;
        if(pointer){expression(combo,QStringLiteral("popup.contentItem.positionViewAtIndex(%1, ListView.Contain)").arg(index));QTest::qWait(100);
            auto result=expression(combo,QStringLiteral("popup.contentItem.itemAtIndex(%1)").arg(index));
            auto *row=result.value<QObject *>();if(!row && result.canConvert<QJSValue>())row=result.value<QJSValue>().toQObject();
            auto *item=qobject_cast<QQuickItem *>(row);check(item,"actual screen popup delegate is visible for pointer selection");click(item);
        }else{QTest::keyClick(window,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(window,Qt::Key_Down);QTest::keyClick(window,Qt::Key_Return);}
        QTest::qWait(100);
    };
    const auto dismiss=[&]{QTest::keyClick(window,Qt::Key_Escape);wait([&]{return !provider->qmlDialogVisible();},"actual Interface host dismisses normally");};
    open();
    wait([&]{return model->capabilities().value("fullscreenScreen").toBool();},
         "actual Interface opening refreshes the ready embedded Windows compositor capability");
    auto *combo=reveal();
    check(combo->property("currentIndex").toInt()==(stale?-1:startup+1),
          "actual combo preserves the cold choice or truthfully leaves a missing choice unselected");
    if(stale)capture(QStringLiteral("native-screen-stale-editor.png"));
    const auto values=model->values();
    for(const QVariant &bad:{QVariant(-2),QVariant(screens.size()),QVariant(0.5),QVariant(QStringLiteral("0"))}){
        check(!model->setValue(QStringLiteral("fullscreenScreen"),bad) && model->values()==values && unchanged(),
              "actual service rejects invalid screen choices without changing saved or active primary state");}
    const int edited=stale?-1:explicitChoice?-1:targetIndex;
    if(initial){
        const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
        struct ScreenSharingLock {HANDLE handle;~ScreenSharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}}
            lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
        check(lock.handle!=INVALID_HANDLE_VALUE,"owned screen config locks for a genuine save failure");
        select(edited,true);const bool failed=model->values()==values && !model->lastError().isEmpty();
        CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
        check(failed && unchanged(),"real locked-save failure rolls back policy and exposes retry feedback");
    }
    select(edited,initial);
    check(model->values().value("fullscreenScreen").toInt()==edited && model->lastError().isEmpty()
          && model->restartRequired() && unchanged(),"actual screen edit saves a restart policy without changing the active fullscreen target");
    if(!stale){select(startup,false);check(!model->restartRequired() && unchanged(),"restoring the startup screen clears restart feedback");}
    if(initial){
        auto *embedded=named(QStringLiteral("interfaceembeddedVideo"));check(embedded,"actual embedded-video toggle exists");
        embedded->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);click(embedded);
        check(!model->capabilities().value("fullscreenScreen").toBool() && !combo->isEnabled()
              && model->values().value("fullscreenScreen").toInt()==startup && unchanged(),
              "saving detached mode disables only its unsupported screen editor and preserves the saved target and current input");
        embedded->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);QTest::keyClick(window,Qt::Key_Space);QTest::qWait(100);
        check(model->capabilities().value("fullscreenScreen").toBool() && combo->isEnabled()
              && !model->restartRequired() && unchanged(),"restoring embedded mode re-enables screen selection and clears restart feedback");
    }
    const int saved=(initial || playing)?edited:stale?-1:startup;
    if(initial || playing)select(saved,false);
    dismiss();open();combo=reveal();
    check(combo->property("currentIndex").toInt()==saved+1 && model->restartRequired()==(saved!=startup) && unchanged(),
          "actual dialog reopen preserves saved target and truthful restart feedback");
    dismiss();
    QSignalSpy requests(context,&MainCtx::setInterfaceFullScreen);
    const auto normalGeometry=window->geometry();
    sampleWindow("visible-normal-before");
    context->setInterfaceFullScreen(true);
    wait([&]{return window->windowStates().testFlag(Qt::WindowFullScreen) && window->screen()==target;},
         "real MainCtx signal and main-window handler enter fullscreen on the startup target");
    check(requests.size()==1 && (window->property("__windowFullScreen").toUInt()&WindowStateHolder::INTERFACE),
          "actual main-window fullscreen holder is acquired through the production route");
    check(unchanged(),"visible fullscreen entry preserves primary playback and the startup selection");
    capture(QStringLiteral("native-screen-fullscreen.png"));
    context->setInterfaceFullScreen(false);
    wait([&]{return !window->windowStates().testFlag(Qt::WindowFullScreen) && window->screen()==target;},
         "actual handler returns the visible window to the approved monitor");
    sampleWindow("visible-normal-restored");
    check(window->geometry()==normalGeometry && unchanged(),"same-monitor fullscreen release preserves exact normal client geometry and primary state");
    if(explicitChoice){
        QScreen *other=nullptr;for(auto *screen:screens)if(screen!=target){other=screen;break;}
        check(other,"real second output exists for the hidden cross-screen route");
        window->hide();QTest::qWait(100);
        check(!window->isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(window->winId())),
              "actual app window is hidden in Qt and Windows before cross-screen preparation");
        // Prepare a different output by position alone. This is a screen-policy
        // journey, not an unrelated hidden-window client-resize experiment.
        window->setScreen(other);window->setFramePosition(other->availableGeometry().topLeft()+QPoint(45,45));
        QTest::qWait(100);const auto sourceGeometry=window->geometry();
        sampleWindow("hidden-source-prepared");
        check(window->screen()==other && !window->isVisible(),"actual hidden main window starts on a genuine different output");
        context->setInterfaceFullScreen(true);QTest::qWait(100);
        sampleWindow("hidden-selected-fullscreen");
        check(window->screen()==target && window->windowStates().testFlag(Qt::WindowFullScreen)
              && !window->isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(window->winId())),
              "saved startup target actually moves the hidden app window to monitor2 before presentation");
        // QWindow::show() calls showNormal() on Windows. Retain the fullscreen
        // state that the actual production handler has just established.
        window->setVisible(true);wait([&]{return window->isVisible() && window->screen()==target
            && window->windowStates().testFlag(Qt::WindowFullScreen);},"selected fullscreen is visibly presented only on monitor2");
        check(unchanged(),"real cross-screen selection preserves active input queue tracks and clocks");
        capture(QStringLiteral("native-screen-selected.png"));
        window->hide();QTest::qWait(100);context->setInterfaceFullScreen(false);QTest::qWait(100);
        sampleWindow("hidden-original-restored");
        check(!window->isVisible() && !IsWindowVisible(reinterpret_cast<HWND>(window->winId()))
              && window->screen()==other && window->geometry()==sourceGeometry,
              "final release restores the real original screen and exact geometry while the app remains hidden");
        window->setScreen(target);window->setGeometry(normalGeometry);window->showNormal();
        wait([&]{return window->isVisible() && window->screen()==target;},"normal app presentation resumes only on monitor2");
    }
    check(unchanged(),"complete screen editor and handler journey preserves primary input queue tracks and clocks");
    capture(QStringLiteral("native-screen-windowed.png"));
    report.insert("nativeFullscreenScreenAccepted",true);report.insert("startupScreenIndex",startup);
    report.insert("savedScreenIndex",saved);report.insert("explicitHiddenCrossScreenTested",explicitChoice);
    report.insert("fullBetaAccepted",false);
}
