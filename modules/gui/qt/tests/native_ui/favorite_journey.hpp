// SPDX-License-Identifier: GPL-2.0-or-later
// Real indexed media, actual tile input and asynchronous source ownership.
QQuickItem *favoriteTile()
{
    auto *bar=named(QStringLiteral("juxtopposedLivePlaybackBar"));
    if(!bar)return nullptr;
    auto value=expression(bar,QStringLiteral("layoutRow.itemFor(%1)").arg(ControlListModel::JUXTO_FAVORITE));
    auto *object=value.value<QObject*>();
    if(!object&&value.canConvert<QJSValue>())object=value.value<QJSValue>().toQObject();
    return qobject_cast<QQuickItem*>(object);
}
void exerciseFavorite(const QString &mode)
{
    auto *current=context->getCurrentMedia();
    check(current!=nullptr,"actual MainCtx owns current-media favorite service");
    const auto first=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    const auto second=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("second.flac")));
    const auto play=[&](const QUrl &url) {
        context->getIntf()->p_mainPlaylistController->append(QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
        context->setPlayerView(true);
        wait([&]{return player()->getUrl()==url&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},"owned media actually decodes before favorite checks",15000);
        {vlc_player_locker lock(context->getIntf()->p_player);vlc_player_Pause(context->getIntf()->p_player);}
        wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"owned primary pauses for exact favorite-state checks");
        wait([]{return favoriteTile()!=nullptr;},"saved profile renders the actual Favorite tile");
    };
    auto *profiles=context->getPlayerProfiles();
    if(mode!=QStringLiteral("favorite-reopen")) {
        check(profiles->beginLayoutEdit(),"owned profile starts isolated tile draft");
        check(profiles->layout()->insertTile(1,2,0,ControlListModel::JUXTO_FAVORITE),"owned music layout adds Favorite");
        check(profiles->saveLayoutDraft(),"checked profile save publishes Favorite tile");
    }
    auto *library=context->getMediaLibrary();
    if(mode==QStringLiteral("favorite-disabled")) {
        check(!library&&!context->hasMediaLibrary(),"Library-off run has no active media-library service");
        play(first);QTest::qWait(100);
        check(!current->available()&&!current->busy()&&!current->favorite()&&current->error().isEmpty(),"Library-off Favorite stays unavailable without background work");
        check(!favoriteTile()->isEnabled()&&!current->setFavorite(true),"Library-off actual tile and native mutation are both gated");
        report.insert("nativeFavoriteDisabledAccepted",true);return;
    }
    check(library!=nullptr,"favorite journey uses real media-library owner");
    if(mode==QStringLiteral("favorite-initial")||mode==QStringLiteral("favorite-failure")) {
        library->addFolder(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")));
        wait([&]{return library->idle()&&library->homeActivity()->mediaCount()==3;},"real library indexes only the three owned fixtures",30000);
    }
    play(first);
    wait([&]{return current->available()&&!current->busy();},"actual current-media URL resolves to a native indexed record");
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();
    const auto volume=player()->getVolume();const auto muted=player()->isMuted();
    const auto unchanged=[&]{return player()->getInput()==input&&nativeTime()==clock&&player()->getRate()==rate
        &&player()->getVolume()==volume&&player()->isMuted()==muted&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;};
    if(mode==QStringLiteral("favorite-failure")) {
        struct Lookup {int64_t id=0;};auto nativeId=std::make_shared<Lookup>();auto ready=std::make_shared<bool>(false);
        library->runOnMLThread<Lookup>(library,[mrl=first.toEncoded()](vlc_medialibrary_t *ml,Lookup &value) {
            ml_unique_ptr<vlc_ml_media_t> media{vlc_ml_get_media_by_mrl(ml,mrl.constData())};
            if(media)value.id=media->i_id;
        },[nativeId,ready](quint64,Lookup &value){*nativeId=value;*ready=true;});
        wait([&]{return *ready;},"failure fixture resolves the actual current media ID on the native worker");
        check(nativeId->id>0&&!current->favorite(),"owned failure fixture starts with a real unchecked media record");
        QDir directory(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT"));
        const auto marker=[&](const QString &name){QFile file(directory.filePath(name));return file.open(QIODevice::WriteOnly)&&file.write("ready\n")==6;};
        QSaveFile request(directory.filePath(QStringLiteral("favorite-failure-ready.json")));
        check(request.open(QIODevice::WriteOnly),"owned database-lock precondition marker opens");
        request.write(QJsonDocument(QJsonObject{{"database",QDir(QVLCUserDir(VLC_USERDATA_DIR)).filePath(QStringLiteral("ml/ml.db"))},
            {"mediaId",QString::number(nativeId->id)}}).toJson());
        check(request.commit(),"owned database-lock precondition publishes atomically");
        wait([&]{return directory.exists(QStringLiteral("favorite-failure-locked"))||directory.exists(QStringLiteral("favorite-failure-lock-error.json"));},"owned runner reports database-writer lock result",10000);
        check(!directory.exists(QStringLiteral("favorite-failure-lock-error.json"))&&directory.exists(QStringLiteral("favorite-failure-locked")),"real isolated database writer lock is held before the actual Favorite click");
        click(favoriteTile());
        wait([&]{return !current->busy()&&!current->error().isEmpty();},"genuine locked-database Favorite write returns an actionable native error",30000);
        check(!current->favorite()&&unchanged(),"failed native write preserves accepted favorite and exact paused primary");
        check(favoriteTile()->isEnabled()&&favoriteTile()->hasActiveFocus(),"actual failed-save tile retains focus and allows retry");
        capture(QStringLiteral("native-favorite-save-error.png"));
        check(marker(QStringLiteral("favorite-failure-observed")),"owned failed-write result releases the test writer lock");
        wait([&]{return directory.exists(QStringLiteral("favorite-failure-unlocked"));},"isolated writer lock is released before retry",10000);
        QTest::keyClick(window,Qt::Key_Space);
        wait([&]{return !current->busy()&&current->favorite()&&current->error().isEmpty();},"actual focused Space retry saves Favorite and clears the error");
        check(unchanged(),"successful retry preserves the same paused native input and clock");
        capture(QStringLiteral("native-favorite-save-retry.png"));
        report.insert("nativeFavoriteFailureAccepted",true);return;
    }
    if(mode==QStringLiteral("favorite-reopen")) {
        check(current->favorite(),"new OS process reads the actual persisted favorite from library database");
        check(favoriteTile()->isEnabled()&&expression(favoriteTile(),QStringLiteral("Accessible.checked")).toBool(),"reopened actual tile exposes saved checked state");
        click(favoriteTile());wait([&]{return !current->busy()&&!current->favorite();},"actual reopened Favorite tile removes favorite through checked native save");
        check(current->error().isEmpty()&&unchanged(),"reopened removal preserves paused primary and has no save error");
        report.insert("nativeFavoriteReopenAccepted",true);return;
    }
    check(!current->favorite()&&favoriteTile()->isEnabled(),"owned fresh indexed media starts with enabled unchecked tile");
    click(favoriteTile());
    wait([&]{return !current->busy()&&current->favorite();},"actual pointer Favorite click saves and reads native favorite state");
    check(current->error().isEmpty()&&unchanged(),"favorite mutation preserves the paused primary input and exact playback state");
    check(favoriteTile()->property("checked").toBool(),"actual Favorite button checked property follows confirmed native save");
    check(expression(favoriteTile(),QStringLiteral("Accessible.checked")).toBool(),"actual Favorite tile publishes checked accessibility state");
    const auto accessibilityState=[] {
        auto *item=favoriteTile();auto *interface=QAccessible::queryAccessibleInterface(item);
        return QJsonObject{{"active",QAccessible::isActive()},{"interface",interface!=nullptr},
            {"checked",interface&&interface->state().checked},{"checkable",interface&&interface->state().checkable},
            {"role",interface?int(interface->role()):-1},{"buttonChecked",item->property("checked").toBool()}};
    };
    report.insert("favoriteAccessibilityBeforeActivation",accessibilityState());
    QAccessible::setActive(true);QTest::qWait(80);
    report.insert("favoriteAccessibilityAfterActivation",accessibilityState());
    auto *accessible=QAccessible::queryAccessibleInterface(favoriteTile());
    check(accessible&&accessible->state().checkable&&accessible->state().checked,"actual Qt accessibility interface exposes a checked Favorite control");
    // A genuine external library write must update the player tile as well.
    auto external=std::make_shared<bool>(false);
    library->runOnMLThread(library,[mrl=first.toEncoded()](vlc_medialibrary_t *ml) {
        ml_unique_ptr<vlc_ml_media_t> media{vlc_ml_get_media_by_mrl(ml,mrl.constData())};
        if(media)vlc_ml_media_set_favorite(ml,media->i_id,false);
    },[external]{*external=true;});
    wait([&]{return *external&&!current->busy()&&!current->favorite();},"native favorites-changed event refreshes the current tile after an external library write");
    check(unchanged(),"external favorite change keeps primary playback untouched");
    auto *actions=accessible->actionInterface();
    check(actions&&actions->actionNames().contains(QAccessibleActionInterface::pressAction()),"actual favorite accessibility interface offers native Press action");
    actions->doAction(QAccessibleActionInterface::pressAction());
    wait([&]{return !current->busy()&&current->favorite();},"actual accessibility Press saves favorite through the same native owner");
    favoriteTile()->forceActiveFocus(Qt::MouseFocusReason);
    wait([&]{return favoriteTile()->hasActiveFocus()&&favoriteTile()->isEnabled()&&!current->busy();},"actual Favorite retains active pointer-style focus before Space");
    QTest::keyClick(window,Qt::Key_Space);
    wait([&]{return !current->busy()&&!current->favorite();},"actual Space activation removes favorite through checked native save");
    check(current->error().isEmpty()&&unchanged(),"accessible and keyboard favorite actions preserve the paused primary");
    // A successful write can have a coalesced event-driven read still pending.
    // Settle that real async work before intentionally blocking this queue.
    QSignalSpy refreshes(current,&MLCurrentMedia::changed);qsizetype observed=refreshes.size();
    QElapsedTimer quiet;quiet.start();
    wait([&]{
        if(observed!=refreshes.size()){observed=refreshes.size();quiet.restart();}
        return !current->busy()&&quiet.elapsed()>=200;
    },"owned favorite fixture settles native refreshes before queue barrier",10000);
    auto entered=std::make_shared<std::atomic_bool>(false),release=std::make_shared<std::atomic_bool>(false);
    library->runOnMLThread(library,[entered,release](vlc_medialibrary_t *) {
        entered->store(true);
        for(int n=0;n<1000&&!release->load();++n)QThread::msleep(10);
    },"current-media-favorite");
    wait([&]{return entered->load();},"owned native favorite queue barrier is active");
    report.insert("favoriteBeforeQueuedWrite",QJsonObject{{"busy",current->busy()},{"favorite",current->favorite()}});
    check(!current->busy()&&!current->favorite(),"owned queue-barrier precondition has no earlier refresh or accepted favorite");
    check(current->setFavorite(true)&&current->busy(),"favorite save enters native busy state while its worker is queued");
    check(!current->setFavorite(false),"second mutation is rejected while checked save is pending");
    play(second);
    check(!current->favorite(),"source change clears stale favorite before prior source job completes");
    release->store(true);
    wait([&]{return current->available()&&!current->busy();},"new source resolves after the older queued favorite write");
    check(!current->favorite()&&current->error().isEmpty(),"older source completion cannot mark the new source favorite");
    play(first);wait([&]{return current->available()&&!current->busy();},"original source resolves again after source race");
    check(current->favorite(),"explicit queued favorite action is saved for its original media");
    capture(QStringLiteral("native-favorite-checked.png"));
    context->getIntf()->p_mainPlaylistController->clear();
    wait([]{return player()->getInput()==nullptr;},"actual queue Clear releases the current native input");
    wait([&]{return !current->available()&&!current->busy();},"Favorite becomes unavailable after input release despite retained last URL");
    check(!current->favorite()&&current->error().isEmpty()&&!current->setFavorite(false),"released-input Favorite cannot mutate the last source's library record");
    report.insert("nativeFavoriteAccepted",true);
}
