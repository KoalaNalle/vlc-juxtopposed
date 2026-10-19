// SPDX-License-Identifier: GPL-2.0-or-later
// Included only by the private in-process native UI journey.
void exerciseMovableDialogs(const std::function<bool()> &unchanged)
{
    struct DialogCase { const char *popup; const char *open; const char *close; };
    const DialogCase dialogs[] = {
        {"juxtopposedNetworkDialog", "showNetworkStreamDialog", "networkClose"},
        {"juxtopposedNewPlaylistDialog", "showNewPlaylistDialog", "newPlaylistClose"},
        {"juxtopposedConversionDialog", "showConversionDialog", "conversionClose"},
        {"juxtopposedGlobalSettings", "showGlobalSettingsDialog", "globalSettingsClose"},
    };
    auto *provider = DialogsProvider::getInstance();
    check(provider, "actual dialog provider exists for native modal journeys");
    const auto popupNamed = [](const char *name) -> QObject * {
        for (auto *object : objects())
            if (object->objectName() == QString::fromLatin1(name) && object->property("visible").toBool())
                return object;
        return nullptr;
    };
    const auto dragArea = [](QObject *popup) -> QQuickItem * {
        for (auto *object : objects())
            if (auto *item = qobject_cast<QQuickItem *>(object);
                item && item->isVisible() && item->property("popup").value<QObject *>() == popup)
                return item;
        return nullptr;
    };
    const auto drag = [&](QObject *popup, const QPoint &delta, bool toOrigin) {
        auto *area = dragArea(popup);
        check(area && area->width() > 4 && area->height() > 4,
              "actual modal has a reachable noninteractive header drag area");
        const QPointF before(popup->property("x").toDouble(), popup->property("y").toDouble());
        const auto start = area->mapToScene(area->boundingRect().center()).toPoint();
        const auto end = toOrigin ? QPoint(1, 1) : start + delta;
        check(QRect(QPoint(), window->size()).contains(start) && QRect(QPoint(), window->size()).contains(end),
              "native modal drag stays inside its owned main window");
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(window, end, 30);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, end);
        QTest::qWait(60);
        const QPointF after(popup->property("x").toDouble(), popup->property("y").toDouble());
        report.insert("lastModalDrag", QJsonObject{{"popup", popup->objectName()},
            {"beforeX", before.x()}, {"beforeY", before.y()}, {"afterX", after.x()}, {"afterY", after.y()},
            {"windowWidth", window->width()}, {"windowHeight", window->height()}});
        if (toOrigin)
            check(qAbs(after.x()) < 0.51 && qAbs(after.y()) < 0.51,
                  "actual pointer drag clamps the modal to its owning window origin");
        else
            check(qAbs(after.x() - before.x()) > 4 || qAbs(after.y() - before.y()) > 4,
                  "actual header pointer input moves the visible modal");
        check(unchanged(), "modal header pointer movement preserves primary playback and audio state");
    };
    QJsonArray accepted;
    for (double scale : {1.0, 1.5, 2.0}) {
        context->setIntfUserScaleFactor(scale);
        window->resize(1280, 900);
        QTest::qWait(150);
        for (const auto &entry : dialogs) {
            auto *opener = named(QStringLiteral("juxtopposedStripmenu"));
            check(opener, "real title-strip opener exists before native modal routing");
            const auto open = [&] {
                opener->forceActiveFocus(Qt::TabFocusReason);
                check(QMetaObject::invokeMethod(provider, entry.open, Qt::DirectConnection),
                      "actual dialog provider accepts the existing modal route");
                wait([&] { return popupNamed(entry.popup) && provider->qmlDialogVisible(); },
                     "existing native modal opens through the actual host");
            };
            open();
            auto *popup = popupNamed(entry.popup);
            drag(popup, QPoint(24, 18), false);
            capture(QStringLiteral("native-modal-%1-scale%2-dragged.png").arg(QString::fromLatin1(entry.popup)).arg(scale));
            window->resize(qMax(640, window->minimumWidth()), qMax(480, window->minimumHeight()));
            QTest::qWait(150);
            drag(popup, {}, true);
            capture(QStringLiteral("native-modal-%1-scale%2-short.png").arg(QString::fromLatin1(entry.popup)).arg(scale));
            click(named(QString::fromLatin1(entry.close)));
            wait([&] { return !popupNamed(entry.popup) && !provider->qmlDialogVisible(); },
                 "actual modal close button dismisses the existing native modal");
            check(opener->hasActiveFocus(), "actual modal dismissal restores its real title-strip opener");
            check(unchanged(), "native modal close preserves paused primary playback and audio state");
            window->resize(1280, 900);
            QTest::qWait(100);
            open();
            popup = popupNamed(entry.popup);
            auto *parent = popup->property("parent").value<QQuickItem *>();
            check(parent, "actual modal retains its item owner after reopening");
            check(qAbs(popup->property("x").toDouble() - qMax(0.0, (parent->width() - popup->property("width").toDouble()) / 2)) < 0.51
                  && qAbs(popup->property("y").toDouble() - qMax(0.0, (parent->height() - popup->property("height").toDouble()) / 2)) < 0.51,
                  "fresh native modal opening recenters after a prior bounded drag");
            QTest::keyClick(window, Qt::Key_Escape);
            wait([&] { return !popupNamed(entry.popup) && !provider->qmlDialogVisible(); },
                 "actual Escape closes the idle modal through its supported policy");
            check(opener->hasActiveFocus() && unchanged(), "native Escape restores the opener and preserves the paused player");
            accepted.append(QJsonObject{{"popup", QString::fromLatin1(entry.popup)}, {"scale", scale}});
        }
    }
    report.insert("nativeMovableModalCases", accepted);
    report.insert("nativeMovableModalDragAccepted", true);
    context->setIntfUserScaleFactor(1);
    window->resize(1280, 900);
    QTest::qWait(150);
}
