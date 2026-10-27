// SPDX-License-Identifier: GPL-2.0-or-later
// Included only by the private in-process native UI journey.
void exerciseBusyDialog(const std::function<bool()> &unchanged)
{
    const QDir base(qEnvironmentVariable("VLC_OWNED_UI_BASE"));
    check(!base.canonicalPath().isEmpty(),"busy modal fixture has its existing owned output directory");
    const auto source=base.filePath("owned-busy-conversion.wav");
    const auto destination=base.filePath("owned-cancelled-conversion.flac");
    check(!QFileInfo::exists(source) && !QFileInfo::exists(destination),
          "busy modal fixture never replaces an existing source or destination");
    {
        QFile file(source);
        check(file.open(QIODevice::WriteOnly|QIODevice::NewOnly),"fresh owned native conversion fixture opens");
        constexpr int seconds=2400;
        constexpr quint32 dataBytes=seconds*44100*2;
        QDataStream stream(&file);stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF",4);stream<<quint32(dataBytes+36);
        stream.writeRawData("WAVEfmt ",8);
        stream<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(44100)<<quint32(88200)<<quint16(2)<<quint16(16);
        stream.writeRawData("data",4);stream<<dataBytes;
        QByteArray block;
        QDataStream samples(&block,QIODevice::WriteOnly);samples.setByteOrder(QDataStream::LittleEndian);
        quint32 random=0x42a24f3bu;
        for(int sample=0;sample<44100;++sample){
            random^=random<<13;random^=random>>17;random^=random<<5;
            samples<<qint16(random);
        }
        bool complete=true;
        for(int second=0;second<seconds;++second)
            if(file.write(block)!=block.size()){complete=false;break;}
        check(complete,"owned conversion noise payload is written completely");
        check(stream.status()==QDataStream::Ok && file.size()==qint64(dataBytes)+44,
              "owned real WAV fixture has the complete declared PCM payload");
    }
    const auto original=ownedJourneyFileHash(source);
    const auto tempBefore=base.entryList({QStringLiteral(".vlc-convert-*")},QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot);
    auto *provider=DialogsProvider::getInstance();
    auto *opener=named(QStringLiteral("juxtopposedStripmenu"));
    check(provider && opener,"actual native provider and title-strip opener exist for the busy modal");
    const auto popup=[](const char *name)->QObject *{
        for(auto *object:objects())
            if(object->objectName()==QString::fromLatin1(name) && object->property("visible").toBool())return object;
        return nullptr;
    };
    opener->forceActiveFocus(Qt::TabFocusReason);
    provider->showConversionDialog();
    wait([&]{return popup("juxtopposedConversionDialog")!=nullptr;},"actual native conversion dialog opens");
    auto *controller=provider->conversion();
    controller->setSourceUrl(QUrl::fromLocalFile(source));
    controller->setDestinationUrl(QUrl::fromLocalFile(destination));
    check(controller->canStart(),"real native controller accepts the generated WAV and new FLAC destination");
    click(named(QStringLiteral("conversionConvert")));
    QDeadlineTimer started(10000);
    while(controller->busy() && !controller->nativeInputActive() && !started.hasExpired())QTest::qWait(1);
    check(controller->busy() && controller->nativeInputActive(),
          "busy modal verification begins with a real active native conversion input");
    struct BusyRoute{const char *method;const char *dialog;};
    const BusyRoute routes[]={{"showGlobalSettingsDialog","juxtopposedGlobalSettings"},
        {"showNetworkStreamDialog","juxtopposedNetworkDialog"},{"showNewPlaylistDialog","juxtopposedNewPlaylistDialog"}};
    for(const auto &route:routes){
        check(controller->busy() && controller->nativeInputActive(),"conversion remains actively native before the blocked modal request");
        check(QMetaObject::invokeMethod(provider,route.method,Qt::DirectConnection),"real provider receives the competing modal request");
        check(popup("juxtopposedConversionDialog") && !popup(route.dialog) && provider->qmlDialogVisible(),
              "actual busy host refuses the competing modal without closing the conversion");
        check(controller->sourceUrl()==QUrl::fromLocalFile(source) && controller->destinationUrl()==QUrl::fromLocalFile(destination)
              && controller->busy() && unchanged(),"blocked native modal request preserves the job and primary playback");
    }
    provider->showConversionDialog();
    auto *cancel=named(QStringLiteral("conversionCancel"));
    check(controller->busy() && controller->nativeInputActive()
          && popup("juxtopposedConversionDialog") && cancel && cancel->hasActiveFocus(),
          "same-dialog native reentry focuses Cancel without resetting the active job");
    auto *dialog=popup("juxtopposedConversionDialog");
    QQuickItem *area=nullptr;
    for(auto *object:objects())if(auto *item=qobject_cast<QQuickItem *>(object);
        item && item->isVisible() && item->property("popup").value<QObject *>()==dialog){area=item;break;}
    check(area && area->width()>4,"actual busy modal retains its noninteractive drag header");
    const auto before=QPointF(dialog->property("x").toDouble(),dialog->property("y").toDouble());
    const auto start=area->mapToScene(area->boundingRect().center()).toPoint();
    const auto end=start+QPoint(24,18);
    check(QRect(QPoint(),window->size()).contains(start) && QRect(QPoint(),window->size()).contains(end),
          "busy-modal pointer drag stays inside its owned window");
    QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,start);
    QTest::mouseMove(window,end,30);QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,end);
    const auto after=QPointF(dialog->property("x").toDouble(),dialog->property("y").toDouble());
    check(qAbs(after.x()-before.x())>4 && qAbs(after.y()-before.y())>4
          && controller->busy() && controller->nativeInputActive() && unchanged(),
          "actual header drag moves the busy dialog while real conversion and primary playback remain independent");
    auto *close=named(QStringLiteral("conversionClose"));
    check(close && close->isEnabled(),"actual busy modal Close action is available for cancellation");
    QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,close->mapToScene(close->boundingRect().center()).toPoint());
    wait([&]{return !controller->busy() && !popup("juxtopposedConversionDialog");},
         "actual busy Close cancels the worker and closes after its normal completion",15000);
    check(controller->state()==ConversionController::Cancelled && !controller->nativeInputActive()
          && !QFileInfo::exists(destination),"native cancellation finishes without publishing the incomplete FLAC output");
    check(base.entryList({QStringLiteral(".vlc-convert-*")},QDir::Dirs|QDir::Hidden|QDir::NoDotAndDotDot)==tempBefore
          && ownedJourneyFileHash(source)==original,"cancelled native conversion removes its staging directory and preserves source bytes");
    check(opener->hasActiveFocus() && unchanged(),"busy-close completion restores its real opener and preserves primary playback");
    provider->showNetworkStreamDialog();
    wait([&]{return popup("juxtopposedNetworkDialog")!=nullptr;},"normal native routing resumes after the worker cancellation");
    QTest::keyClick(window,Qt::Key_Escape);
    wait([&]{return !provider->qmlDialogVisible();},"post-cancellation native modal dismisses normally");
    report.insert("nativeBusyModalAccepted",true);
    report.insert("busySourceSha256",QString::fromLatin1(original.toHex()));
    capture(QStringLiteral("native-after-busy-modal-cancel.png"));
}
