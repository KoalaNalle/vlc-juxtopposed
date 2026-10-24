// SPDX-License-Identifier: GPL-2.0-or-later
// Included only by the private in-process native UI journey.
QByteArray ownedJourneyFileHash(const QString &path)
{
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "owned native fixture opens for identity verification");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    check(hash.addData(&file), "owned native fixture identity is read completely");
    return hash.result();
}

void exerciseNavigationRecovery(const std::function<bool()> &unchanged)
{
    check(pins()->count()==0 && !pins()->busy(), "native recovery begins with an empty isolated pin store");
    const QDir media(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"));
    check(pins()->pinUrl(QUrl::fromLocalFile(media.filePath("second.flac")), "media", "Owned saved pin"),
          "real native pin service accepts the initial saved descriptor");
    wait([] { return pins()->count()==1 && !pins()->busy(); }, "initial real pin is committed before the storage failure");
    auto *settings=context->getIntf()->mainSettings;
    settings->sync();
    const auto path=QFileInfo(settings->fileName()).canonicalFilePath();
    const auto base=QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).canonicalPath()+QLatin1Char('/');
    check(settings->format()==QSettings::IniFormat && !path.isEmpty()
          && path.startsWith(base,Qt::CaseInsensitive), "storage failure targets only this test's owned portable INI");
    struct ReadLock {
        HANDLE handle=INVALID_HANDLE_VALUE;
        explicit ReadLock(const QString &path) {
            handle=CreateFileW(path.toStdWString().c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            check(handle!=INVALID_HANDLE_VALUE,"owned INI read-sharing handle blocks writes without changing permissions");
        }
        void release() { if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;} }
        ~ReadLock() { release(); }
    };
    const auto before=ownedJourneyFileHash(path);
    ReadLock lock(path);
    check(pins()->pinUrl(QUrl::fromLocalFile(media.filePath("first.flac")), "media", "Owned pending pin"),
          "real native pin service queues the second descriptor while storage is blocked");
    wait([] { return pins()->canRetry() && !pins()->error().isEmpty(); }, "actual blocked save exposes the native Retry state");
    check(pins()->count()==1 && ownedJourneyFileHash(path)==before,
          "failed pin save preserves the published model and exact existing INI bytes");
    check(unchanged(),"blocked pin save preserves primary input paused clocks volume and mute");
    window->resize(qMax(320,window->minimumWidth()),qMax(360,window->minimumHeight()));
    click(named(QStringLiteral("juxtopposedStripnavigation")));
    wait([] { return named(QStringLiteral("juxtopposedNavigation")); }, "actual player drawer opens with its real storage error");
    auto *add=named(QStringLiteral("juxtopposedSidebarAdd"));
    check(add && !add->isEnabled(),"native Add action is disabled while an unsaved pin awaits Retry");
    auto *row=pinRow(0);row->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(window,Qt::Key_Down);QTest::qWait(80);
    auto *retry=named(QStringLiteral("juxtopposedSidebarRetry"));
    check(retry && retry->hasActiveFocus(),"native arrow traversal skips disabled Add and reaches the real Retry action");
    auto *list=named(QStringLiteral("juxtopposedSidebarPinList"));
    const auto bounds=retry->mapRectToItem(list,retry->boundingRect());
    check(bounds.top()>=-.51 && bounds.bottom()<=list->height()+.51,
          "focused native Retry is fully visible in the short drawer");
    QTest::keyClick(window,Qt::Key_Down);QTest::qWait(60);
    auto *dismiss=named(QStringLiteral("juxtopposedSidebarDismiss"));
    check(dismiss && dismiss->hasActiveFocus(),
          "native arrow traversal reaches the real Dismiss action after Retry");
    QTest::keyClick(window,Qt::Key_Up);QTest::qWait(60);
    check(retry->hasActiveFocus(),"native reverse traversal returns from Dismiss to Retry");
    capture(QStringLiteral("native-sidebar-store-blocked.png"));
    lock.release();
    QTest::keyClick(window,Qt::Key_Space);
    wait([] { return pins()->count()==2 && !pins()->busy() && !pins()->canRetry() && pins()->error().isEmpty(); },
         "actual keyboard Retry commits exactly the queued pin after storage is restored");
    check(add->hasActiveFocus(),"successful native Retry restores focus to the enabled Add action");
    check(add->property("visualFocus").toBool(),"keyboard native Retry restores a visible Add focus outline");
    check(unchanged(),"native Retry preserves the primary player and audio state");
    {
        QSettings readback(path,QSettings::IniFormat);readback.setFallbacksEnabled(false);
        if(!settings->group().isEmpty())readback.beginGroup(settings->group());
        const auto saved=QJsonDocument::fromJson(readback.value(SidebarPinsStore::settingsKey()).toString().toUtf8()).object().value("pins").toArray();
        check(saved.size()==2 && saved[0].toObject().value("key").toString()==pins()->get(0).value("key").toString()
              && saved[1].toObject().value("key").toString()==pins()->get(1).value("key").toString(),
              "fresh native settings reader sees both exact committed pin identities in order");
    }
    capture(QStringLiteral("native-sidebar-after-retry.png"));
    auto *pointerPin=pinRow(1);
    QTest::mouseClick(window,Qt::RightButton,Qt::NoModifier,pointerPin->mapToScene(pointerPin->boundingRect().center()).toPoint());
    wait([]{return menu()!=nullptr;},"actual native pin pointer context menu opens after Retry");
    QTest::keyClick(menu(),Qt::Key_Escape);
    wait([]{return menu()==nullptr;},"actual native pin pointer menu dismisses without changing data");
    check(pins()->count()==2 && unchanged(),"pointer context-menu inspection preserves committed pins and primary playback");
    QTest::keyClick(window,Qt::Key_Down);QTest::qWait(80);
    check(add->hasActiveFocus(),"arrow input after pointer-menu dismissal reaches Add without changing playback");
    QTest::keyClick(window,Qt::Key_Up);QTest::qWait(80);
    check(pinRow(1)->hasActiveFocus() && unchanged(),"reverse arrow returns to the saved pin without leaking into volume");
    pointerPin=pinRow(1);
    QTest::mouseClick(window,Qt::RightButton,Qt::NoModifier,pointerPin->mapToScene(pointerPin->boundingRect().center()).toPoint());
    wait([]{return menu()!=nullptr;},"pointer context menu reopens before the keyboard transition check");
    QTest::keyClick(menu(),Qt::Key_Escape);
    wait([]{return menu()==nullptr;},"reopened pointer menu closes before the keyboard transition check");
    pinMenu(1,QStringLiteral("Unpin"));
    wait([] { return pins()->count()==1 && !pins()->busy(); },"actual native menu removes the focused second pin");
    check(pinRow(0)->hasActiveFocus(),"removing a focused pin restores its remaining neighbor by identity");
    pinMenu(0,QStringLiteral("Unpin"));
    wait([] { return pins()->count()==0 && !pins()->busy(); },"actual native menu removes the last pin");
    wait([&] { return add->hasActiveFocus(); },"removing the last native pin restores the Add fallback");
    auto addVisible=[&] {
        const auto bounds=add->mapRectToItem(list,add->boundingRect());
        return add->hasActiveFocus() && bounds.top()>=-.51 && bounds.bottom()<=list->height()+.51;
    };
    QTest::qWait(150);
    check(addVisible(),"last-pin fallback stays fully visible after the empty-list layout settles");
    check(unchanged(),"native pin removal and focus fallback preserve primary playback");
    {
        ReadLock blocked(path);
        check(pins()->pinUrl(QUrl::fromLocalFile(media.filePath("second.flac")),"media","Owned discarded pin"),
              "real native pin service accepts a separate recoverable failure");
        wait([] { return pins()->canRetry(); },"second blocked save exposes the real native error actions");
        auto *recent=named(QStringLiteral("juxtopposedNavigationrecent"));
        check(recent,"actual Recent route remains present during an empty-store failure");
        recent->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyClick(window,Qt::Key_Down);QTest::qWait(60);
        auto *retryAction=named(QStringLiteral("juxtopposedSidebarRetry"));
        check(retryAction && retryAction->hasActiveFocus(),
              "empty-store error traversal skips disabled Add and reaches Retry");
        QTest::keyClick(window,Qt::Key_Down);QTest::qWait(60);
        auto *dismissAction=named(QStringLiteral("juxtopposedSidebarDismiss"));
        check(dismissAction && dismissAction->hasActiveFocus(),
              "empty-store error traversal reaches Dismiss");
        QTest::keyClick(window,Qt::Key_Space);QTest::qWait(100);
        check(!pins()->canRetry() && pins()->error().isEmpty() && pins()->count()==0,
              "actual Dismiss discards the unsaved descriptor instead of publishing it");
        check(add->hasActiveFocus(),"native Dismiss restores the enabled Add action");
        check(add->property("visualFocus").toBool(),"keyboard native Dismiss restores a visible Add focus outline");
    }
    QTest::qWait(150);
    check(addVisible(),"native Add fallback remains fully visible after Dismiss collapses the error footer");
    check(pins()->count()==0 && !pins()->busy() && unchanged(),
          "discarded pin work does not resurrect or disturb primary playback");
    QTest::keyClick(window,Qt::Key_Space);
    wait([] { return menu()!=nullptr; },"recovered native Add action still opens its actual chooser menu");
    QTest::keyClick(menu(),Qt::Key_Escape);
    wait([] { return menu()==nullptr; },"recovered native chooser dismisses normally");
    check(addVisible(),"empty native Add fallback remains reachable after its chooser menu closes");
    capture(QStringLiteral("native-sidebar-after-dismiss.png"));
    report.insert("nativeNavigationRecoveryAccepted",true);
    window->resize(1280,900);
}

void exerciseHomePinMenu(const std::function<bool()> &unchanged)
{
    check(!context->isPlayerView() && pins()->count()==0,"Home menu journey begins in the actual empty Home surface");
    const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath("second.flac"));
    check(pins()->pinUrl(url,"media","Owned Home menu pin"),"Home menu journey stores one generated-media pin");
    wait([]{return pins()->count()==1 && !pins()->busy();},"Home pin is committed before pointer input");
    const auto originalKey=key(0);
    QPointer<QQuickItem> row=pinRow(0);
    auto *navigation=named(QStringLiteral("juxtopposedNavigation"));
    check(navigation,"Home navigation owner exists");
    int signal=-1;
    for(int index=0;index<navigation->metaObject()->methodCount();++index){
        const auto method=navigation->metaObject()->method(index);
        if(method.methodType()==QMetaMethod::Signal && method.name()=="pinMenuRequested"){signal=index;break;}
    }
    check(signal>=0,"Home context-menu request signal is discoverable");
    QSignalSpy requests(navigation,navigation->metaObject()->method(signal));
    check(requests.isValid(),"Home context-menu requests can be observed without altering focus");
    auto observe=[&](const QString &phase){
        const auto *focused=window->activeFocusItem();
        const auto *compositorFocus=context->getIntf()->p_compositor->activeFocusItem();
        auto observations=report.value("homeMenuFocus").toArray();
        observations.append(QJsonObject{{"phase",phase},{"pinAlive",!row.isNull()},
            {"pinActiveFocus",row && row->hasActiveFocus()},{"pinVisualFocus",row && row->property("visualFocus").toBool()},
            {"pinFocusReason",row?row->property("focusReason").toInt():-1},
            {"windowFocus",focused?focused->objectName():QString()},
            {"compositorFocus",compositorFocus?compositorFocus->objectName():QString()},
            {"windowActive",window->isActive()},{"menuVisible",menu()!=nullptr},{"requestSignals",requests.count()}});
        report.insert("homeMenuFocus",observations);
        QJsonArray owners;
        for(auto *object:objects()) if(object->property("openingFocusReason").isValid())
            owners.append(QJsonObject{{"openingReason",object->property("openingFocusReason").toInt()},
                {"visible",object->property("visible").toBool()},
                {"actionSelected",object->property("actionSelected").toBool()},
                {"generation",object->property("generation").toInt()}});
        report.insert("homeMenuOwners",owners);
    };
    QTest::mouseClick(window,Qt::RightButton,Qt::NoModifier,row->mapToScene(row->boundingRect().center()).toPoint());
    wait([]{return menu()!=nullptr;},"Home pin opens its actual pointer context menu");
    observe("pointer-open");
    QTest::keyClick(menu(),Qt::Key_Escape);
    wait([]{return menu()==nullptr;},"Home pointer menu closes by Escape");
    observe("after-pointer-escape");
    check(row && row->hasActiveFocus(),"Home pin retains focus after pointer menu dismissal without synthetic refocusing");
    check(!row->property("visualFocus").toBool(),"Home pointer menu dismissal preserves pointer focus styling");
    for(const auto code : {Qt::Key_Menu,Qt::Key_F10}) {
        const auto before=requests.count();
        QTest::keyClick(window,code,code==Qt::Key_F10?Qt::ShiftModifier:Qt::NoModifier);
        QTest::qWait(100);observe(code==Qt::Key_F10?"shift-f10":"menu-key");
        check(requests.count()==before+1,"Home keyboard menu key reaches the pin without repairing focus between actions");
        wait([]{return menu()!=nullptr;},"Home keyboard context menu is actually visible");
        QTest::keyClick(menu(),Qt::Key_Escape);
        wait([]{return menu()==nullptr;},"Home keyboard context menu closes without selecting an action");
        check(row && row->hasActiveFocus(),"Home pin retains focus through repeated keyboard context menus");
        QTest::qWait(100);observe(code==Qt::Key_F10?"after-shift-f10-escape":"after-menu-key-escape");
        wait([&]{return row && row->property("visualFocus").toBool();},"Home keyboard menu dismissal restores a visible focus outline");
    }
    check(pins()->count()==1 && key(0)==originalKey && unchanged(),"Home non-destructive menu journey preserves exact pin and primary state");
    capture(QStringLiteral("native-home-menu-after-keyboard.png"));
    report.insert("nativeHomePinMenuAccepted",true);
}
