// SPDX-License-Identifier: GPL-2.0-or-later
// Exact production cancellation bindings plus the actual NavigationAttached
// implementation. Item hierarchy, event object and requestBack recipient are
// supplied; ordinary VLC must verify popup lifetime and real key delivery.
#include <QFile>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QTest>
#include <QtPlugin>
#include "widgets/native/navigation_attached.hpp"

Q_IMPORT_PLUGIN(QOffscreenIntegrationPlugin)

class TestTracksCancelNavigation : public QObject
{
    Q_OBJECT
    QByteArray listSource, pageSource;
    static QByteArray binding(const QByteArray &source, const QString &name)
    {
        const auto expression = QRegularExpression(QStringLiteral("^\\s*(Navigation\\.%1:[^\\r\\n]*)$").arg(name), QRegularExpression::MultilineOption);
        return expression.match(QString::fromUtf8(source)).captured(1).toUtf8();
    }
    static QByteArray read(const QString &name)
    {
        QFile file(qEnvironmentVariable("VLC_TRACKS_SOURCE_DIR") + QLatin1Char('/') + name);
        if (!file.open(QIODevice::ReadOnly)) return {};
        return file.readAll();
    }
private slots:
    void initTestCase()
    {
        listSource = read(QStringLiteral("TracksListPage.qml"));
        pageSource = read(QStringLiteral("TracksPage.qml"));
        QVERIFY(!listSource.isEmpty()); QVERIFY(!pageSource.isEmpty());
        qmlRegisterType<QQuickItem>("TracksFixture",1,0,"Item");
        qmlRegisterUncreatableType<NavigationAttached>("TracksFixture",1,0,"Navigation","Attached only");
    }
    void cancellation_data()
    {
        QTest::addColumn<bool>("list");
        QTest::addColumn<bool>("throughRow");
        QTest::newRow("subtitle-column") << true << true;
        QTest::newRow("audio-column") << true << true;
        QTest::newRow("playback-speed-button") << true << false;
        QTest::newRow("synchronization-page") << false << false;
    }
    void cancellation()
    {
        QFETCH(bool,list); QFETCH(bool,throughRow);
        const auto &source = list ? listSource : pageSource;
        // Limit the root binding to the region before the first visual child.
        const auto rootPrefix = source.left(source.indexOf(list ? "    Widgets.ButtonExt {" : "    Item {"));
        const auto cancel = binding(rootPrefix, QStringLiteral("cancelAction"));
        QByteArray rowParent;
        if (throughRow) {
            const auto start=listSource.indexOf("    Widgets.NavigableRow {");
            QVERIFY(start>=0);
            const auto end=listSource.indexOf("        TrackColumn {",start);
            QVERIFY(end>start);
            rowParent=binding(listSource.mid(start,end-start),QStringLiteral("parentItem"));
        }
        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QByteArray scene = QByteArrayLiteral(
            "import TracksFixture 1.0\nItem { id: harness; property int cancels: 0;"
            "Item { id: trackMenuController; function requestBack() { harness.cancels += 1 } }"
            "Item { id: root; ") + cancel + '\n' +
            (throughRow ? QByteArrayLiteral("Item { id: row; ")+rowParent+'\n'+
                "Item { id: column; Navigation.parentItem: row; Item { id: target; Navigation.parentItem: column } } }"
                : QByteArrayLiteral("Item { id: target; Navigation.parentItem: root }")) +
            "} function dispatch(event) { target.Navigation.defaultKeyAction(event) } }";
        component.setData(scene,QUrl(QStringLiteral("file:///tracks-cancellation-fixture.qml")));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QObject event;
        event.setProperty("accepted",false);event.setProperty("key",Qt::Key_Escape);
        event.setProperty("modifiers",int(Qt::NoModifier));event.setProperty("text",QString());
        event.setProperty("isAutoRepeat",false);event.setProperty("count",1);
        QVERIFY(QMetaObject::invokeMethod(root.data(),"dispatch",Q_ARG(QVariant,QVariant::fromValue(&event))));
        QVERIFY(event.property("accepted").toBool());
        QCOMPARE(root->property("cancels").toInt(),1);
    }
};
QTEST_MAIN(TestTracksCancelNavigation)
#include "test_tracks_cancel_navigation.moc"
