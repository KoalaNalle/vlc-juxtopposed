// SPDX-License-Identifier: GPL-2.0-or-later
// Actual native menu/form, checked profile save and live bar consumers.
QObject *customizerPopup()
{
    for(auto *object:objects())if(object->objectName()==QStringLiteral("juxtopposedPlayerCustomizer")&&object->property("visible").toBool())return object;
    return nullptr;
}
void openCustomizer()
{
    click(named(QStringLiteral("juxtopposedStripmenu")));
    wait([]{return menu()!=nullptr;},"actual global menu opens for Customize Interface");
    auto *global=menu();QAction *entry=nullptr;
    for(auto *action:global->actions())if(action->menu()&&QString(action->text()).remove(QLatin1Char('&'))==QStringLiteral("Tools"))entry=action;
    check(entry!=nullptr,"actual native Tools submenu exists");
    QTest::mouseClick(global,Qt::LeftButton,Qt::NoModifier,global->actionGeometry(entry).center());
    wait([&]{return entry->menu()->isVisible();},"actual pointer opens Tools submenu");
    auto *submenu=entry->menu();QAction *choice=nullptr;
    for(auto *action:submenu->actions())if(QString(action->text()).remove(QLatin1Char('&')).startsWith(QStringLiteral("Customize Interface")))choice=action;
    check(choice&&choice->isEnabled(),"existing native Customize Interface action is enabled");
    QTest::mouseClick(submenu,Qt::LeftButton,Qt::NoModifier,submenu->actionGeometry(choice).center());
    wait([]{return customizerPopup()!=nullptr;},"native Tools action opens the new draft customizer");
}
void customizerAction(const QString &name)
{
    auto *item=named(name);check(item&&item->isEnabled(),"actual customizer action exists and is enabled");
    item->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(50);click(item);
}
QQuickItem *customizerPreview()
{
    auto *loader=named(QStringLiteral("customizerPreview"));
    check(loader!=nullptr,"native draft preview loader exists");
    auto *bar=qobject_cast<QQuickItem*>(loader->property("item").value<QObject*>());
    check(bar!=nullptr,"native draft preview is an actual player bar");return bar;
}
void customizerDrag(QQuickItem *source,const QPoint &end)
{
    check(source!=nullptr,"native drag source exists");
    source->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(50);
    const auto start=source->mapToScene(source->boundingRect().center()).toPoint();
    QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,start);
    for(int n=1;n<=6;++n)QTest::mouseMove(window,start+(end-start)*n/6,10);
    QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,end);QTest::qWait(60);
}
void exerciseCustomizer(const QString &mode)
{
    auto *profiles=context->getPlayerProfiles();auto *layout=profiles->layout();
    check(profiles->available()&&!profiles->needsRecovery(),"real profile coordinator is ready for customizer");
    const bool reopened=mode==QStringLiteral("customizer-reopen");
    const bool playing=mode==QStringLiteral("customizer-playing");
    const auto source=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(reopened?QStringLiteral("second.flac"):QStringLiteral("first.flac")));
    context->getMediaLibrary()->addAndPlay(source);context->setPlayerView(true);
    wait([&]{return player()->getUrl()==source&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},"real audio decodes for customizer journey",15000);
    wait([]{return nativeTime()>VLC_TICK_FROM_MS(200)&&named(QStringLiteral("juxtopposedLivePlaybackBar"));},"actual music player and native timeline become ready");
    check(player()->getFileName()==source.fileName()&&player()->getFolderName()==QStringLiteral("Owned Pins Ää 日本語"),"native filename and folder metadata follow the current source URL");
    auto *bar=named(QStringLiteral("juxtopposedLivePlaybackBar"));
    check(bar->property("customized").toBool(),"real music bar consumes its native profile layout");
    if(reopened) {
        const auto video=layout->snapshot().value(QStringLiteral("video")).toMap();
        const auto audio=layout->snapshot().value(QStringLiteral("audio")).toMap();
        check(video.value(QStringLiteral("columns")).toList()[1].toList()==QVariantList{int(ControlListModel::JUXTO_FORWARD_15)}
              &&video.value(QStringLiteral("timelineAbove")).toBool(),"new OS process restores edited video controls and timeline position");
        check(audio.value(QStringLiteral("columns")).toList()[1].toList().last().toInt()==ControlListModel::STOP_BUTTON
              &&!audio.value(QStringLiteral("timelineAbove")).toBool(),"new OS process restores independent edited music controls and timeline position");
        check(!bar->property("stacked").toBool(),"actual restarted music bar uses the saved inline timeline");
        check(!customizerPopup()&&!layout->editing(),"normal startup restores layout without reopening draft editing");
        capture(QStringLiteral("native-customizer-reopened.png"));report.insert("nativeCustomizerReopenAccepted",true);return;
    }
    if(!playing) {
        {vlc_player_locker lock(context->getIntf()->p_player);vlc_player_Pause(context->getIntf()->p_player);}
        wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"primary pauses before exact-state customizer checks");
    }
    const auto before=layout->snapshot();const auto input=player()->getInput();const auto clock=nativeTime();
    const auto position=player()->getPosition();const auto rate=player()->getRate();const auto volume=player()->getVolume();const auto mute=player()->isMuted();
    const auto seek=bar->property("seekItem");
    check(seek.isValid()&&seek.value<QObject*>(),"actual music controls own a real native seek object");
    const auto unchanged=[&]{return player()->getInput()==input&&player()->getRate()==rate&&player()->getVolume()==volume&&player()->isMuted()==mute
        &&(playing?player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&nativeTime()>=clock:
            player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED&&nativeTime()==clock&&player()->getPosition()==position);};
    openCustomizer();
    check(layout->editing()&&context->getIntf()->p_mainPlaylistController&&unchanged(),"opening native customizer preserves the primary media");
    check(bar->opacity()==0,"native live controls remain alive but are not drawn beneath the draft preview");
    auto *preview=customizerPreview();
    const auto selectedValue=expression(preview,QStringLiteral("layoutRow.itemFor(0).parent"));
    auto *selectedObject=selectedValue.value<QObject*>();
    if(!selectedObject&&selectedValue.canConvert<QJSValue>())selectedObject=selectedValue.value<QJSValue>().toQObject();
    QPointer<QQuickItem> selected=qobject_cast<QQuickItem*>(selectedObject);
    check(selected!=nullptr,"actual editable Play tile exists in native video preview");
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,selected->mapToScene(selected->boundingRect().center()).toPoint());
    check(selected&&selected->hasActiveFocus()&&unchanged(),"actual pointer selection gives the draft tile keyboard focus without controlling media");
    QTest::keyClick(window,Qt::Key_Delete);QTest::qWait(40);
    check(!layout->draftVideo()->left()->getControls().contains(ControlListModel::PLAY_BUTTON)&&layout->snapshot()==before&&unchanged(),
          "actual Delete after pointer selection removes only the selected draft tile");
    bool filled=true;
    for(int n=0;filled&&n<40;++n)filled=layout->insertTile(0,0,layout->draftVideo()->left()->rowCount(),ControlListModel::STOP_BUTTON);
    check(filled,"owned native draft fills a control column for clipping checks");
    QQuickItem *viewport=nullptr;
    for(auto *object:objects())if(object->objectName()==QStringLiteral("juxtopposedCustomControls"))
        if(auto *item=qobject_cast<QQuickItem*>(object);item&&preview->isAncestorOf(item))viewport=item;
    check(viewport!=nullptr,"overflow check targets the actual draft viewport");
    viewport->setProperty("contentX",200.);QTest::qWait(40);
    check(viewport->property("contentX").toDouble()>100,"native draft control row actually scrolls horizontally");
    const auto overflow=layout->draftSnapshot();
    customizerDrag(named(QStringLiteral("customizerPaletteTile5")),viewport->mapToScene(QPointF(-15,16)).toPoint());
    check(layout->draftSnapshot()==overflow&&unchanged(),"actual native drag into clipped row padding preserves the draft and media");
    customizerAction(QStringLiteral("customizerPaletteTile5"));customizerAction(QStringLiteral("customizerAddCentre"));
    check(layout->draftSnapshot()!=before&&layout->snapshot()==before&&unchanged(),"actual Add Centre edits the video draft only");
    customizerAction(QStringLiteral("customizerCancel"));wait([]{return !customizerPopup();},"actual Cancel closes native customizer");
    check(layout->snapshot()==before&&!layout->editing()&&unchanged(),"native Cancel preserves accepted layout and primary state");
    check(bar->opacity()==1,"Cancel restores the existing live controls presentation");
    openCustomizer();
    customizerAction(QStringLiteral("customizerPaletteTile5"));customizerAction(QStringLiteral("customizerAddCentre"));
    customizerAction(QStringLiteral("customizerTimelineAbove"));
    customizerAction(QStringLiteral("customizerAudio"));
    customizerAction(QStringLiteral("customizerPaletteTile7"));customizerAction(QStringLiteral("customizerAddCentre"));
    customizerAction(QStringLiteral("customizerTimelineAbove"));
    const auto draft=layout->draftSnapshot();check(draft!=before&&layout->snapshot()==before&&unchanged(),"both native player drafts are isolated until checked Save");
    capture(QStringLiteral("native-customizer-draft.png"));
    {
        QLockFile blocker(context->getIntf()->mainSettings->fileName()+QStringLiteral(".player-profiles.lock"));
        check(blocker.tryLock(0),"owned conflicting profile writer lock is acquired");
        customizerAction(QStringLiteral("customizerSave"));
        check(customizerPopup()&&layout->editing()&&layout->snapshot()==before&&layout->draftSnapshot()==draft&&!profiles->lastError().isEmpty()&&unchanged(),
              "actual save conflict retains editable draft accepted layout and primary media");
        capture(QStringLiteral("native-customizer-save-error.png"));
    }
    customizerAction(QStringLiteral("customizerSave"));wait([]{return !customizerPopup();},"actual Save retry closes after checked native profile persistence");
    check(layout->snapshot()==draft&&!layout->editing()&&!profiles->unsaved()&&unchanged(),"successful native Save publishes both layouts without changing media state");
    check(bar->opacity()==1,"checked Save restores the live controls presentation");
    check(named(QStringLiteral("juxtopposedLivePlaybackBar"))==bar&&bar->property("seekItem")==seek&&!bar->property("stacked").toBool(),
          "actual edited music bar reuses its native seek object and switches timeline placement");
    const auto rendered=QJsonDocument::fromJson(expression(bar,QStringLiteral("JSON.stringify(controlLayout)")).toString().toUtf8()).toVariant().toMap();
    check(rendered==draft.value(QStringLiteral("audio")).toMap(),"rendered music bar consumes the accepted native columns");
    capture(QStringLiteral("native-customizer-applied.png"));
    if(playing)wait([&]{return nativeTime()>clock+VLC_TICK_FROM_MS(500);},"decoded primary clock continues through draft cancel error retry and live layout changes");
    report.insert("nativeCustomizerAccepted",true);report.insert("primaryPlaying",playing);
}
