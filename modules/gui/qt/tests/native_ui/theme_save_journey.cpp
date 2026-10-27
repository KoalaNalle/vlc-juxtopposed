// Opt-in checked theme and legacy-widget regression; never in ordinary VLC.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/general_preferences_model.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "dialogs/preferences/simple_preferences.hpp"
#include "menus/custom_menus.hpp"
#include "util/color_scheme_model.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <stdexcept>
#include <windows.h>

namespace {
MainCtx *context=nullptr;QQuickWindow *window=nullptr;
QJsonObject report;QJsonArray checks;
void save()
{
    report.insert("checks",checks);report.insert("privateFixture",true);report.insert("fullBetaAccepted",false);
    QFile file(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(report).toJson());
}
void check(bool value,const char *name)
{
    checks.append(QJsonObject{{"name",name},{"passed",value}});save();
    if(!value)throw std::runtime_error(name);
}
QByteArray bytes(const QString &path)
{QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};}
struct ReadLock {
    HANDLE handle=INVALID_HANDLE_VALUE;
    explicit ReadLock(const QString &path) {handle=CreateFileW(path.toStdWString().c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);}
    void release(){if(handle!=INVALID_HANDLE_VALUE){CloseHandle(handle);handle=INVALID_HANDLE_VALUE;}}
    ~ReadLock(){release();}
};
void run()
{
    try {
        auto *preferences=context->getGeneralPreferences();auto *scheme=context->getColorScheme();
        auto *settings=context->getIntf()->mainSettings;const auto path=settings->fileName();
        check(QFileInfo(path).absoluteFilePath().startsWith(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).absolutePath()+QLatin1Char('/')),"theme fixture is fenced to its owned Qt INI");
        check(preferences->setValue(QStringLiteral("theme"),2),"checked General service commits Dark before widget tests");
        SPrefsPanel panel(context->getIntf(),nullptr,SPrefsInterface);panel.show();QTest::qWait(100);
        auto *combo=panel.findChild<QComboBox *>(QStringLiteral("colorSchemeComboBox"));
        check(combo&&combo->isVisible()&&combo->currentIndex()==2,"actual legacy Interface panel restores Dark in its combo");
        const auto before=bytes(path);
        {
            ReadLock lock(path);check(lock.handle!=INVALID_HANDLE_VALUE,"owned Qt INI replacement is blocked for rejected combo save");
            combo->setFocus();QTest::keyClick(combo,Qt::Key_Up);QTest::qWait(50);
            check(scheme->currentIndex()==2&&combo->currentIndex()==2,"failed actual combo input returns widget and native theme to accepted Dark");
            check(bytes(path)==before&&!preferences->lastError().isEmpty(),"failed combo save preserves disk and reports checked save error");
            lock.release();
        }
        QTest::keyClick(combo,Qt::Key_Up);QTest::qWait(50);
        check(scheme->currentIndex()==1&&combo->currentIndex()==1&&preferences->lastError().isEmpty(),"same combo keyboard input retries successfully after storage recovers");
        check(preferences->setValue(QStringLiteral("theme"),2)&&combo->currentIndex()==2,"General theme choice refreshes the existing legacy combo");
        CheckableListMenu menu(QStringLiteral("Owned color scheme"),scheme,QActionGroup::ExclusionPolicy::Exclusive,&panel);
        const auto actions=menu.actions();check(actions.size()==3&&actions[2]->isChecked(),"actual checkable scheme menu restores Dark");
        const auto menuBefore=bytes(path);
        {
            ReadLock lock(path);check(lock.handle!=INVALID_HANDLE_VALUE,"owned Qt INI replacement is blocked for rejected menu save");
            menu.popup(panel.mapToGlobal(QPoint(30,30)));QTest::qWait(60);
            check(menu.isVisible(),"actual legacy scheme menu opens before rejected pointer input");
            QTest::mouseClick(&menu,Qt::LeftButton,Qt::NoModifier,menu.actionGeometry(actions[1]).center());QTest::qWait(50);
            check(actions[2]->isChecked()&&!actions[1]->isChecked()&&scheme->currentIndex()==2,"failed menu pointer choice restores the QActionGroup accepted check mark");
            check(bytes(path)==menuBefore&&!preferences->lastError().isEmpty(),"failed menu save preserves disk and checked error");
            lock.release();
        }
        menu.popup(panel.mapToGlobal(QPoint(30,30)));QTest::qWait(60);
        QTest::mouseClick(&menu,Qt::LeftButton,Qt::NoModifier,menu.actionGeometry(actions[1]).center());QTest::qWait(50);
        check(actions[1]->isChecked()&&!actions[2]->isChecked()&&scheme->currentIndex()==1&&combo->currentIndex()==1,"same menu pointer choice retries and updates both actual legacy widgets");
        settings->sync();QSettings fresh(path,QSettings::IniFormat);fresh.sync();
        check(fresh.value(QStringLiteral("MainWindow/color-scheme-index")).toInt()==1&&preferences->lastError().isEmpty(),"successful widget retry is physically saved as Light");
        menu.hide();panel.close();report.insert("passed",true);save();
        DialogsProvider::getInstance()->quit();
    }catch(const std::exception &error){report.insert("passed",false);report.insert("error",error.what());save();DialogsProvider::getInstance()->quit();}
}
void findWindow(int attempts)
{
    QList<QObject *> pending;QSet<QObject *> seen;
    for(auto *candidate:QGuiApplication::allWindows())if(auto *quick=qobject_cast<QQuickWindow *>(candidate))pending.append(quick->contentItem());
    while(!pending.isEmpty()){
        auto *object=pending.takeLast();if(!object||seen.contains(object))continue;seen.insert(object);pending.append(object->children());
        if(auto *item=qobject_cast<QQuickItem *>(object)){
            for(auto *child:item->childItems())pending.append(child);
            if(object->objectName()==QStringLiteral("juxtopposedDialogsHost")){
                auto *engine=qmlEngine(object);context=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;window=item->window();
            }
        }
    }
    if(context&&window){
        const auto gate=qEnvironmentVariable("VLC_OWNED_UI_START_GATE");
        if(gate.isEmpty()||QFile::exists(gate)){QTimer::singleShot(150,context,run);return;}
        report.insert("status","awaiting-owned-window-inspection");save();
    }
    if(attempts>0)QTimer::singleShot(250,qApp,[attempts]{findWindow(attempts-1);});
    else {report.insert("passed",false);report.insert("error","owned window/gate unavailable");save();}
}
void start(){if(!qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))QTimer::singleShot(800,qApp,[]{findWindow(1200);});}
}
Q_COREAPP_STARTUP_FUNCTION(start)
