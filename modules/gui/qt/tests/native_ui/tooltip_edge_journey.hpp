// SPDX-License-Identifier: GPL-2.0-or-later
// Actual title-strip tooltip bounds; private driver only.
void exerciseTooltipEdge()
{
    const auto screenName = qEnvironmentVariable("VLC_OWNED_UI_SCREEN");
    check(window->screen() && window->screen()->name() == screenName,
          "tooltip journey remains on the selected test monitor");
    window->showMaximized();
    wait([]{ return window->visibility() == QWindow::Maximized; },
         "actual native window maximizes before edge placement checks");
    auto *button = named(QStringLiteral("juxtopposedStripclose"));
    check(button && button->isEnabled(), "actual visible title-strip Close control exists");
    window->requestActivate();
    // Focus shows help without activating the destructive Close action.
    button->forceActiveFocus(Qt::TabFocusReason);
    wait([&]{ return button->hasActiveFocus(); }, "Close owns actual keyboard focus without activation");
    auto *popup = button->findChild<QObject *>(QStringLiteral("juxtopposedStripTooltipclose"));
    if (!popup) {
        const auto value = expression(button, QStringLiteral("C.ToolTip.toolTip"));
        popup = value.value<QObject *>();
        if (!popup && value.canConvert<QJSValue>()) popup = value.value<QJSValue>().toQObject();
    }
    check(popup != nullptr, "actual title-strip tooltip is materialized");
    wait([&]{ return popup->property("visible").toBool()
                     && popup->property("text").toString() == QStringLiteral("Close"); },
         "keyboard focus opens the real Close tooltip", 5000);
    QTest::qWait(350);
    auto *content = qobject_cast<QQuickItem *>(popup->property("contentItem").value<QObject *>());
    check(content && content->parentItem(), "tooltip has actual rendered popup content");
    auto *item = content->parentItem();
    const QRectF bounds(item->mapToGlobal(QPointF{}), item->size());
    const QRectF screenBounds(window->screen()->availableGeometry());
    const auto rect = [](const QRectF &r) { return QJsonArray{r.x(), r.y(), r.width(), r.height()}; };
    const auto visibleHelpCount = [] {
        int count = 0;
        for (auto *object : objects())
            if (object->objectName().startsWith(QStringLiteral("juxtopposedStripTooltip"))
                && object->property("visible").toBool()) ++count;
        return count;
    };
    report.insert("tooltipGlobalBounds", rect(bounds));
    report.insert("screenAvailableBounds", rect(screenBounds));
    report.insert("popupType", popup->property("popupType").toInt());
    capture(QStringLiteral("native-titlebar-tooltip-edge.png"));
    check(screenBounds.adjusted(-1,-1,1,1).contains(bounds),
          "complete Close tooltip stays within the selected monitor available bounds");
    check(visibleHelpCount() == 1, "keyboard focus shows exactly one title-strip tooltip");
    check(window->isVisible() && window->visibility() == QWindow::Maximized,
          "tooltip inspection never activates Close or changes window mode");
    QJsonArray restored;
    for (const auto width : {1000, 640}) {
        window->showNormal(); window->resize(width, 600); QTest::qWait(200);
        const auto available = window->screen()->availableGeometry();
        window->setFramePosition(QPoint(available.right()-window->frameGeometry().width()+1,
                                        available.top()+12));
        QTest::qWait(200);
        check(window->screen() && window->screen()->name() == screenName,
              "restored edge window remains on the selected test monitor");
        for (const auto pointer : {false, true}) {
            auto *home = named(QStringLiteral("juxtopposedStriplogo"));
            check(home != nullptr, "real Home button remains available for focus transfer");
            home->forceActiveFocus(Qt::TabFocusReason);
            QTest::mouseMove(window, QPoint(window->width()/2,window->height()/2));
            wait([&]{ return !popup->property("visible").toBool(); },
                 "previous Close help closes before the next activation");
            if (pointer)
                QTest::mouseMove(window, button->mapToScene(button->boundingRect().center()).toPoint());
            else button->forceActiveFocus(Qt::TabFocusReason);
            wait([&]{ return popup->property("visible").toBool(); },
                 "actual keyboard or pointer activation opens the edge tooltip");
            QTest::qWait(300);
            const auto helpCount = visibleHelpCount();
            check(helpCount == 1,
                  "hover priority prevents duplicate focused and hovered title-strip help");
            const QRectF actual(item->mapToGlobal(QPointF{}),item->size());
            const QRectF screen(window->screen()->availableGeometry());
            restored.append(QJsonObject{{"width",window->width()},{"pointer",pointer},
                                         {"visibleHelpCount",helpCount},
                                         {"tooltipGlobalBounds",rect(actual)},
                                         {"screenAvailableBounds",rect(screen)}});
            check(screen.adjusted(-1,-1,1,1).contains(actual),
                  "restored normal and narrow keyboard/pointer help stays within its monitor");
        }
    }
    report.insert("restoredEdgeCases",restored);
    capture(QStringLiteral("native-titlebar-tooltip-restored.png"));
    report.insert("nativeTooltipEdgeAccepted", true);
}
