// SPDX-License-Identifier: GPL-2.0-or-later
// Actual editors and elapsed production timers; no direct timeout injection.
void exerciseAutoHide(const QString &mode)
{
    const bool editor=mode.startsWith(QStringLiteral("auto-hide-editor-"));
    const bool reopen=mode.endsWith(QStringLiteral("-reopen"));
    const bool video=mode.contains(QStringLiteral("-video-"));
    const bool playing=reopen || mode.endsWith(QStringLiteral("-playing"));
    const bool pinned=mode.contains(QStringLiteral("-pinned")) || editor;
    const bool zero=mode.endsWith(QStringLiteral("-zero"));
    const bool longDelay=mode.endsWith(QStringLiteral("-long"));
    const int policy=editor?(reopen?2:0):mode.contains(QStringLiteral("-never-"))?0:
                     mode.contains(QStringLiteral("-fullscreen-"))?1:2;
    const int delay=zero?0:longDelay?10000:reopen?2000:1000;
    auto *model=context->getInterfacePreferences();
    check(model && model->capabilities().value("controlAutoHide").toBool()
          && model->capabilities().value("controlHideDelay").toBool(),
          "actual Interface auto-hide and delay consumers are linked independently");
    check(context->controlAutoHideMode()==policy && context->controlsHideTimeout()==delay,
          "actual MainCtx snapshots the requested mode and exact millisecond delay");
    check(model->values().value("controlAutoHide").toInt()==policy
          && model->values().value("controlHideDelay").toInt()==delay && !model->restartRequired(),
          "cold actual preference values agree without stale restart feedback");
    context->setPinVideoControls(pinned);
    const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(
        video?QStringLiteral("owned-controls.mp4"):QStringLiteral("owned-settings.wav")));
    auto *native=context->getIntf()->p_player;
    context->getIntf()->p_mainPlaylistController->append(
        QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
    context->setPlayerView(true);
    wait([&]{vlc_player_locker lock(native);const auto *stats=vlc_player_GetStatistics(native);
        return player()->getUrl()==url && stats && (video?stats->i_decoded_video>0:stats->i_decoded_audio>0);},
         "owned long input decodes through the actual primary player",15000);
    if(!playing){vlc_player_locker lock(native);vlc_player_Pause(native);}
    wait([&]{return player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
         "auto-hide journey reaches its requested playing or paused state");
    if(!playing){auto last=nativeTime();QElapsedTimer stable;stable.start();
        wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>1000;},
             "paused native clock settles before controls testing",5000);}
    QQuickItem *root=nullptr;
    wait([&]{for(auto *object:objects())if(object->objectName()==QStringLiteral("juxtopposedLivePlaybackBar"))
            if(auto *item=qobject_cast<QQuickItem *>(object)){root=item;break;}
        while(root && root->metaObject()->indexOfProperty("_controlsForcedVisible")<0)root=root->parentItem();
        return root!=nullptr;},"actual production Player root materializes independently of visibility");
    const auto objectFor=[&](const QString &id)->QObject *{
        const auto value=expression(root,id);auto *object=value.value<QObject *>();
        if(!object && value.canConvert<QJSValue>())object=value.value<QJSValue>().toQObject();return object;};
    auto *bar=qobject_cast<QQuickItem *>(objectFor(QStringLiteral("controlBar")));
    auto *fsm=objectFor(QStringLiteral("playerToolbarVisibilityFSM"));
    check(bar && fsm,"actual bottom controls and visibility owner exist");
    const auto signalMethod=[&](const char *name){const int index=fsm->metaObject()->indexOfSignal(name);
        check(index>=0,"actual visibility diagnostic signal exists");return fsm->metaObject()->method(index);};
    QSignalSpy mouseSignals(fsm,signalMethod("mouseMove()"));
    QSignalSpy keyboardSignals(fsm,signalMethod("keyboardMove()"));
    QSignalSpy timeoutSignals(fsm,signalMethod("timeout()"));
    const auto timerState=[&]{QJsonArray values;
        for(auto *child:fsm->children())if(child->metaObject()->indexOfProperty("interval")>=0
            && child->metaObject()->indexOfProperty("running")>=0)
            values.append(QJsonObject{{"interval",child->property("interval").toInt()},
                {"running",child->property("running").toBool()}});
        return values;
    };
    QElapsedTimer diagnosticClock;diagnosticClock.start();QTimer diagnosticPoll;
    int lastMouse=-1,lastKeyboard=-1,lastTimeout=-1;
    QObject::connect(&diagnosticPoll,&QTimer::timeout,&diagnosticPoll,[&]{
        if(lastMouse==mouseSignals.count() && lastKeyboard==keyboardSignals.count() && lastTimeout==timeoutSignals.count())return;
        lastMouse=mouseSignals.count();lastKeyboard=keyboardSignals.count();lastTimeout=timeoutSignals.count();
        auto timeline=report.value("visibilityTimeline").toArray();
        timeline.append(QJsonObject{{"milliseconds",diagnosticClock.elapsed()},{"mouseSignals",lastMouse},
            {"keyboardSignals",lastKeyboard},{"timeoutSignals",lastTimeout},{"timers",timerState()},
            {"timeoutDuration",fsm->property("timeoutDuration").toInt()},{"visible",fsm->property("isVisible").toBool()}});
        report.insert("visibilityTimeline",timeline);
    });
    diagnosticPoll.start(20);
    const auto sample=[&](const char *phase){
        QJsonObject value{{"phase",QString::fromLatin1(phase)},{"mouseSignals",mouseSignals.count()},
            {"keyboardSignals",keyboardSignals.count()},{"timeoutSignals",timeoutSignals.count()},
            {"diagnosticMilliseconds",diagnosticClock.elapsed()},{"timers",timerState()},
            {"barVisible",bar->isVisible()},{"barHovered",bar->property("hovered").toBool()}};
        for(const auto *name:{"autoHideMode","automaticallyHide","interactionMayReveal","controlsForcedVisible",
             "lockCount","timeoutDuration","isVisible","started"}){
            QQmlExpression evaluate(qmlContext(fsm),fsm,QString::fromLatin1(name));
            const auto result=evaluate.evaluate();value.insert(QString::fromLatin1(name),QJsonValue::fromVariant(result));
            if(evaluate.hasError())value.insert(QString::fromLatin1(name)+QStringLiteral("Error"),evaluate.error().toString());
        }
        auto samples=report.value("visibilitySamples").toArray();samples.append(value);report.insert("visibilitySamples",samples);
    };
    check(context->pinVideoControls()==pinned,"requested docking layout is applied without changing the auto-hide policy");
    const auto pointerCenter=[&]{QTest::mouseMove(window,QPoint(window->width()*3/4,window->height()/2));
        QTest::mouseMove(window,QPoint(window->width()*3/4+5,window->height()/2+5));};
    if(root->property("juxtopposedNavigationVisible").toBool()){
        pointerCenter();click(named(QStringLiteral("juxtopposedStripnavigation")));
        wait([&]{return !root->property("juxtopposedNavigationVisible").toBool();},"actual navigation affordance closes the initial sidebar lock");}
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
    const auto volume=player()->getVolume();const auto mute=player()->isMuted();QElapsedTimer runTime;runTime.start();
    const auto queueState=[&]{
        auto *queue=context->getIntf()->p_playlist;vlc::playlist::vlc_playlist_locker lock(queue);QVector<quint64> ids;
        for(size_t i=0;i<vlc_playlist_Count(queue);++i)ids.push_back(vlc_playlist_item_GetId(vlc_playlist_Get(queue,i)));
        return qMakePair(ids,vlc_playlist_GetCurrentIndex(queue));
    };
    const auto selectedTracks=[&]{
        vlc_player_locker lock(native);QVector<const vlc_es_id_t *> ids;
        for(auto category:{AUDIO_ES,VIDEO_ES,SPU_ES})for(size_t i=0;i<vlc_player_GetTrackCount(native,category);++i){
            const auto *track=vlc_player_GetTrackAt(native,category,i);if(track->selected)ids.push_back(track->es_id);
        }
        return ids;
    };
    const auto queue=queueState();const auto tracks=selectedTracks();
    const auto unchanged=[&]{return player()->getInput()==input && player()->getRate()==rate
        && player()->getVolume()==volume && player()->isMuted()==mute
        && queueState()==queue && selectedTracks()==tracks
        && player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
        && (playing?std::abs(nativeTime()-clock-vlc_tick_t(VLC_TICK_FROM_MS(runTime.elapsed())*rate))<VLC_TICK_FROM_MS(1500):nativeTime()==clock);};
    QTest::qWait(150);check(unchanged(),"no-write primary state control passes before actual controls activity");
    if(editor){
        auto *provider=DialogsProvider::getInstance();
        const auto open=[&]{check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"),"native route opens actual Interface host");
            wait([]{return named(QStringLiteral("globalCategoryinterface"));},"actual Interface category materializes");
            click(named(QStringLiteral("globalCategoryinterface")));
            wait([]{return named(QStringLiteral("interfacecontrolAutoHide"));},"actual auto-hide editor materializes");};
        const auto reveal=[&](const char *name){auto *item=named(QString::fromLatin1(name));check(item,"actual policy editor exists");
            item->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);return item;};
        const auto select=[&](int index){auto *combo=reveal("interfacecontrolAutoHide");click(combo);
            wait([&]{return expression(combo,QStringLiteral("popup.visible")).toBool();},"actual mode popup opens");
            QTest::keyClick(window,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(window,Qt::Key_Down);
            QTest::keyClick(window,Qt::Key_Return);QTest::qWait(100);};
        const auto keyDelay=[&](Qt::Key key){reveal("interfacecontrolHideDelay");QTest::keyClick(window,key);QTest::qWait(100);};
        const auto setDelay=[&](int target){
            reveal("interfacecontrolHideDelay");
            for(int i=0;i<25 && model->values().value("controlHideDelay").toInt()!=target;++i){
                QTest::keyClick(window,model->values().value("controlHideDelay").toInt()>target?Qt::Key_Left:Qt::Key_Right);
                QTest::qWait(100);
            }
        };
        const auto saved=[&](int modeValue,int ms){return model->values().value("controlAutoHide").toInt()==modeValue
            && model->values().value("controlHideDelay").toInt()==ms && model->lastError().isEmpty();};
        const auto dismiss=[&]{QTest::keyClick(window,Qt::Key_Escape);wait([&]{return !provider->qmlDialogVisible();},"actual immediate-write Interface host dismisses normally");};
        open();check(named(QStringLiteral("interfacecontrolAutoHide"))->isEnabled()
            && named(QStringLiteral("interfacecontrolHideDelay"))->isEnabled(),"actual mode and delay rows are enabled");
        if(!reopen){
            const auto config=qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
            struct AutoHideSharingLock {HANDLE handle;~AutoHideSharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}}
                lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
            check(lock.handle!=INVALID_HANDLE_VALUE,"owned auto-hide config locks for a genuine save failure");
            keyDelay(Qt::Key_Right);
            const bool failed=model->values().value("controlHideDelay").toInt()==1000 && !model->lastError().isEmpty();
            CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
            check(failed && unchanged(),"real locked-save failure rolls back policy and exposes retry feedback");
            keyDelay(Qt::Key_Right);check(saved(0,1500) && model->restartRequired(),"same delay editor retries successfully after unlock in exact milliseconds");
            auto *slider=reveal("interfacecontrolHideDelay");click(slider);
            check(saved(0,5000),"actual pointer midpoint saves five seconds as five thousand native milliseconds");
            setDelay(0);check(saved(0,0),"actual delay editor accepts zero milliseconds explicitly");
            setDelay(10000);check(saved(0,10000),"actual delay editor accepts ten thousand milliseconds explicitly");
            setDelay(1000);
            check(saved(0,1000) && !model->restartRequired(),"restoring original delay clears restart feedback");
        }
        select(reopen?0:2);
        setDelay(2000);
        check(saved(reopen?0:2,2000) && model->restartRequired() && unchanged(),
              "actual mode and delay editing preserves startup consumer and exact primary state");
        if(reopen){select(2);check(saved(2,2000) && !model->restartRequired(),"restoring both cold policies clears restart feedback");}
        dismiss();open();
        check(named(QStringLiteral("interfacecontrolAutoHide"))->property("currentIndex").toInt()==2
              && std::abs(named(QStringLiteral("interfacecontrolHideDelay"))->property("value").toDouble()-2.)<.001,
              "actual dialog reopen retains Always and two seconds");
        dismiss();check(context->controlAutoHideMode()==policy && context->controlsHideTimeout()==delay && unchanged(),
              "saved policies cannot change existing startup mode or timeout");
    }
    const auto idle=[&](bool fullscreen){
        if(fullscreen)window->showFullScreen();else window->showNormal();
        wait([&]{return (window->visibility()==QWindow::FullScreen)==fullscreen;},"actual native window reaches requested fullscreen state");
        check(window->screen() && window->screen()->name()==qEnvironmentVariable("VLC_OWNED_UI_SCREEN"),"auto-hide transition remains on the approved monitor");
        QElapsedTimer elapsed;elapsed.start();pointerCenter();
        const bool shouldHide=policy==2 || (policy==1 && fullscreen);
        sample("idle-pointer-delivered");
        if(shouldHide)wait([&]{return fsm->property("lockCount").toInt()==0;},"real pointer movement releases hover and navigation locks");
        if(!shouldHide){QTest::qWait(delay+350);check(bar->isVisible() && fsm->property("isVisible").toBool(),
            "real idle timeout does not hide controls outside the selected mode");}
        else{
            if(delay>0){wait([&]{return bar->isVisible() && fsm->property("isVisible").toBool();},"actual audio or video pointer movement reveals controls");
                QTest::qWait(qMin(250,delay/3));check(fsm->property("isVisible").toBool(),"real timer does not hide controls before its saved delay");}
            QDeadlineTimer deadline(delay+1500);
            while(fsm->property("isVisible").toBool() && !deadline.hasExpired())QTest::qWait(20);
            sample("idle-expiry-observed");
            check(!fsm->property("isVisible").toBool(),"actual production timer expires without synthetic timeout injection");
            const auto measured=elapsed.elapsed();check(measured>=qMax(0,delay-200) && measured<=delay+1500,
                "elapsed auto-hide delay is bounded around the saved milliseconds");
            wait([&]{return !bar->isVisible();},"actual playback bar completes its hide transition",2000);
            QJsonArray timings=report.value("timerObservations").toArray();timings.append(QJsonObject{{"fullscreen",fullscreen},{"savedMilliseconds",delay},{"elapsedMilliseconds",measured}});report.insert("timerObservations",timings);
        }
        check(unchanged(),"idle controls policy preserves primary playback state and clocks");
    };
    idle(false);capture(QStringLiteral("native-autohide-windowed.png"));
    idle(true);capture(QStringLiteral("native-autohide-fullscreen.png"));
    if(policy!=0 && delay>0){
        // A modifier-only key is actual keyboard activity without a playback shortcut.
        sample("keyboard-before");
        QElapsedTimer elapsed;elapsed.start();QTest::keyClick(window,Qt::Key_Shift);
        wait([&]{return fsm->property("isVisible").toBool() && bar->isVisible();},"actual keyboard input reveals hidden controls");
        report.insert("keyboardRevealMilliseconds",elapsed.elapsed());
        sample("keyboard-after-reveal");
        // Native key delivery itself may wait between press and release. Check
        // the deadline from the input's start, not from the driver's return.
        const auto remaining=delay+100-elapsed.elapsed();
        if(remaining>0)QTest::qWait(remaining);
        sample("keyboard-retention-observed");
        report.insert("keyboardRetentionMilliseconds",elapsed.elapsed());
        check(elapsed.elapsed()<2*delay,"keyboard driver reaches the retention checkpoint before expiry");
        check(fsm->property("isVisible").toBool(),"keyboard reveal retains the existing doubled timeout");
        wait([&]{return !fsm->property("isVisible").toBool();},"actual keyboard reveal expires through its production timer",delay+1500);
        check(elapsed.elapsed()>=2*delay-200 && elapsed.elapsed()<=2*delay+1500,"elapsed keyboard reveal follows twice the saved delay");
    }
    if(policy==2 && !longDelay){
        const auto point=bar->mapToScene(bar->boundingRect().center()).toPoint();
        check(QRectF(QPointF{},QSizeF(window->size())).contains(point),"actual bottom hover target remains in the owned window");
        report.insert("hoverTarget",QJsonObject{{"x",point.x()},{"y",point.y()},{"barWidth",bar->width()},{"barHeight",bar->height()}});
        QTest::mouseMove(window,point);
        QDeadlineTimer hoverDeadline(10000);
        while(!(bar->isVisible() && fsm->property("lockCount").toInt()>0) && !hoverDeadline.hasExpired())QTest::qWait(20);
        sample("hover-lock-observed");
        check(bar->isVisible() && fsm->property("lockCount").toInt()>0,"actual bottom hover reveals and locks the controls");
        QTest::qWait(delay+350);check(bar->isVisible(),"real bottom hover keeps controls usable beyond idle expiry");
        pointerCenter();wait([&]{return !bar->isVisible();},"leaving bottom hover resumes real timed hiding",delay+2500);
        if(delay>0){
            pointerCenter();click(named(QStringLiteral("juxtopposedStripnavigation")));
            wait([&]{return root->property("juxtopposedNavigationVisible").toBool();},"actual title affordance opens navigation");
            pointerCenter();QTest::qWait(delay+350);check(bar->isVisible(),"navigation visibility retains its independent controls lock");
            click(named(QStringLiteral("juxtopposedStripnavigation")));pointerCenter();
            wait([&]{return !bar->isVisible() && fsm->property("lockCount").toInt()==0;},"closing navigation restores idle hiding without a leaked lock",delay+2500);
        }
    }
    window->showNormal();pointerCenter();
    wait([&]{return window->visibility()!=QWindow::FullScreen && window->screen()
        && window->screen()->name()==qEnvironmentVariable("VLC_OWNED_UI_SCREEN");},"normal window returns on the approved monitor");
    check(unchanged(),"all controls timers hover navigation and fullscreen actions preserve primary playback");
    report.insert("nativeAutoHideAccepted",true);report.insert("startupMode",policy);report.insert("startupDelayMilliseconds",delay);
    report.insert("docked",pinned);report.insert("playing",playing);report.insert("video",video);report.insert("fullBetaAccepted",false);
}
