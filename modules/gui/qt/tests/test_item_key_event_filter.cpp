// SPDX-License-Identifier: GPL-2.0-or-later
// Real QApplication shortcut delivery through the production toolbar filter.
#include "util/item_key_event_filter.hpp"
#include <QApplication>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickView>
#include <QTest>
#include <QtPlugin>

Q_IMPORT_PLUGIN(QOffscreenIntegrationPlugin)
Q_IMPORT_QML_PLUGIN(QtQuick2Plugin)
Q_IMPORT_QML_PLUGIN(QtQmlPlugin)
Q_IMPORT_QML_PLUGIN(QtQmlModelsPlugin)
Q_IMPORT_QML_PLUGIN(QtQmlWorkerScriptPlugin)

namespace {
struct Scene {
    QQuickView view;
    QObject *root=nullptr;
    QString error;
    bool open() {
        view.rootContext()->setContextProperty(QStringLiteral("testWindow"),&view);
        QQmlComponent component(view.engine());
        component.setData(R"qml(
import QtQuick
import Test.InputFilter 1.0
Item {
    id: root
    width: 400; height: 240
    property bool consumeKeys: false
    property bool filterEnabled: true
    property int folderCount: 0
    property int otherShortcutCount: 0
    property int findCount: 0
    property int textCount: 0
    property int ownerReleaseCount: 0
    property int observerPressCount: 0
    property int observerReleaseCount: 0
    Shortcut { sequence: "Ctrl+F"; context: Qt.WindowShortcut; onActivated: root.folderCount += 1 }
    Shortcut { sequence: "Ctrl+O"; context: Qt.WindowShortcut; onActivated: root.otherShortcutCount += 1 }
    FocusScope {
        anchors.fill: parent
        Keys.onShortcutOverride: (event) => {
            if (event.key === Qt.Key_F && event.modifiers === Qt.ControlModifier)
                event.accepted = true
        }
        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_F && event.modifiers === Qt.ControlModifier) {
                root.findCount += 1
                event.accepted = true
            } else if (event.key === Qt.Key_X) {
                root.textCount += 1
                event.accepted = true
            }
        }
        Keys.onReleased: (event) => {
            if (event.key === Qt.Key_F || event.key === Qt.Key_X) {
                root.ownerReleaseCount += 1
                event.accepted = true
            }
        }
        Item { objectName: "focusedChild"; focus: true }
    }
    KeyEventFilter {
        target: testWindow
        enabled: root.filterEnabled
        Keys.onPressed: (event) => {
            // QTest also sends the Control modifier as a separate key event.
            if (event.key !== Qt.Key_Control) root.observerPressCount += 1
            if (root.consumeKeys) event.accepted = true
        }
        Keys.onReleased: (event) => {
            if (event.key !== Qt.Key_Control) root.observerReleaseCount += 1
            if (root.consumeKeys) event.accepted = true
        }
    }
}
)qml",QUrl());
        if(component.isError()) {
            for(const auto &e:component.errors())error+=e.toString()+QLatin1Char('\n');
            return false;
        }
        root=component.create();
        if(!root){error=QStringLiteral("fixture root unavailable");return false;}
        view.setContent(QUrl(),&component,root);
        view.resize(400,240);view.show();view.requestActivate();QTest::qWait(100);
        auto *child=root->findChild<QQuickItem *>(QStringLiteral("focusedChild"));
        if(!child){error=QStringLiteral("focus child unavailable");return false;}
        child->forceActiveFocus(Qt::MouseFocusReason);QTest::qWait(30);
        if(!child->hasActiveFocus()){error=QStringLiteral("fixture child has no active focus");return false;}
        return true;
    }
    int count(const char *name) const{return root->property(name).toInt();}
};
}

class TestItemKeyEventFilter:public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qmlRegisterType<ItemKeyEventFilter>("Test.InputFilter",1,0,"KeyEventFilter");
    }
    void reservedShortcut_data() {
        QTest::addColumn<bool>("consume");
        QTest::newRow("observe")<<false;
        QTest::newRow("consume")<<true;
    }
    void reservedShortcut() {
        QFETCH(bool,consume);Scene scene;QVERIFY2(scene.open(),qPrintable(scene.error));
        scene.root->setProperty("consumeKeys",consume);
        QTest::keyClick(&scene.view,Qt::Key_F,Qt::ControlModifier);QTest::qWait(30);
        QCOMPARE(scene.count("folderCount"),0);
        QCOMPARE(scene.count("findCount"),consume?0:1);
        QCOMPARE(scene.count("observerPressCount"),1);
        QCOMPARE(scene.count("observerReleaseCount"),1);
        QCOMPARE(scene.count("ownerReleaseCount"),consume?0:1);
    }
    void unclaimedShortcutRemainsGlobal() {
        Scene scene;QVERIFY2(scene.open(),qPrintable(scene.error));
        QTest::keyClick(&scene.view,Qt::Key_O,Qt::ControlModifier);QTest::qWait(30);
        QCOMPARE(scene.count("otherShortcutCount"),1);
        QCOMPARE(scene.count("findCount"),0);
        QCOMPARE(scene.count("observerPressCount"),0);
    }
    void ordinaryKeyIsDeliveredOnce() {
        Scene scene;QVERIFY2(scene.open(),qPrintable(scene.error));
        QTest::keyClick(&scene.view,Qt::Key_X);QTest::qWait(30);
        QCOMPARE(scene.count("textCount"),1);
        QCOMPARE(scene.count("observerPressCount"),1);
        QCOMPARE(scene.count("observerReleaseCount"),1);
        QCOMPARE(scene.count("ownerReleaseCount"),1);
    }
    void disabledFilterLeavesOwnerUntouched() {
        Scene scene;QVERIFY2(scene.open(),qPrintable(scene.error));
        scene.root->setProperty("filterEnabled",false);
        QTest::keyClick(&scene.view,Qt::Key_F,Qt::ControlModifier);QTest::qWait(30);
        QCOMPARE(scene.count("folderCount"),0);
        QCOMPARE(scene.count("findCount"),1);
        QCOMPARE(scene.count("observerPressCount"),0);
        QCOMPARE(scene.count("observerReleaseCount"),0);
    }
    void repeatedKeysDoNotAccumulate() {
        Scene scene;QVERIFY2(scene.open(),qPrintable(scene.error));
        for(int n=0;n<5;++n)QTest::keyClick(&scene.view,Qt::Key_F,Qt::ControlModifier);
        QTest::qWait(30);
        QCOMPARE(scene.count("folderCount"),0);
        QCOMPARE(scene.count("findCount"),5);
        QCOMPARE(scene.count("observerPressCount"),5);
        QCOMPARE(scene.count("observerReleaseCount"),5);
    }
};
int main(int argc,char **argv) {
    qputenv("QT_QPA_PLATFORM","offscreen");
    qputenv("QT_QUICK_BACKEND","software");
    QApplication app(argc,argv);
    TestItemKeyEventFilter test;
    return QTest::qExec(&test,argc,argv);
}
#include "test_item_key_event_filter.moc"
