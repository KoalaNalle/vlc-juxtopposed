// Private owned-window test TU. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/extensions_preferences_model.hpp"
#include "network/addonsmodel.hpp"
#include "util/csdbuttonmodel.hpp"
#include "dialogs/extensions/extensions_manager.hpp"
#include "menus/menus.hpp"
#include <QMenu>
#include <QListView>
#include <QPointer>
#include <QAction>
#include <QDeadlineTimer>
#include <QFileInfo>
#include <memory>
#include <functional>
#include <vlc_extensions.h>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include "dialogs/dialogs_provider.hpp"
#include "dialogs/plugins/plugins.hpp"
#include <vlc_configuration.h>
#include <vlc_interface.h>
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QWidget>
#include <windows.h>
#include <dbghelp.h>
#include <stdexcept>
#include <stdlib.h>

namespace {
using OwnedDumpWriter=BOOL(WINAPI*)(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,const PMINIDUMP_EXCEPTION_INFORMATION,const PMINIDUMP_USER_STREAM_INFORMATION,const PMINIDUMP_CALLBACK_INFORMATION);
OwnedDumpWriter ownedDumpWriter=nullptr;
std::wstring ownedDumpPath;
volatile LONG ownedDumpWritten=0,ownedShutdownStarted=0;
PVOID ownedExceptionHandler=nullptr;
LPTOP_LEVEL_EXCEPTION_FILTER previousExceptionFilter=nullptr;
LONG CALLBACK captureOwnedFault(PEXCEPTION_POINTERS info)
{
    const auto code=info->ExceptionRecord->ExceptionCode;
    const bool critical=code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_ILLEGAL_INSTRUCTION||code==EXCEPTION_STACK_OVERFLOW||code==0xc000041d||code==0xc0000409;
    const bool shutdownThrow=InterlockedCompareExchange(&ownedShutdownStarted,0,0)&&(code==0xe06d7363||code==0x20474343);
    if((critical||shutdownThrow)&&ownedDumpWriter&&InterlockedCompareExchange(&ownedDumpWritten,1,0)==0){
        const auto file=CreateFileW(ownedDumpPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE){
            MINIDUMP_EXCEPTION_INFORMATION exception{GetCurrentThreadId(),info,FALSE};
            ownedDumpWriter(GetCurrentProcess(),GetCurrentProcessId(),file,MINIDUMP_TYPE(MiniDumpNormal|MiniDumpWithThreadInfo),&exception,nullptr,nullptr);
            CloseHandle(file);
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
void installOwnedFaultCapture()
{
    const auto library=LoadLibraryW(L"dbghelp.dll");
    if(!library)return;
    ownedDumpWriter=reinterpret_cast<OwnedDumpWriter>(GetProcAddress(library,"MiniDumpWriteDump"));
    ownedDumpPath=QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("native-fault.dmp").toStdWString();
    ownedExceptionHandler=AddVectoredExceptionHandler(1,captureOwnedFault);
    previousExceptionFilter=SetUnhandledExceptionFilter(captureOwnedFault);
    QObject::connect(qApp,&QCoreApplication::aboutToQuit,qApp,[]{
        if(ownedExceptionHandler)RemoveVectoredExceptionHandler(ownedExceptionHandler);
        SetUnhandledExceptionFilter(previousExceptionFilter);
    });
}
QJsonArray checks;
MainCtx *context = nullptr;
QQuickWindow *ownedWindow = nullptr;
QJsonObject report;
QList<QObject*> objects()
{
    QList<QObject*> pending, result;
    for (auto *window : QGuiApplication::allWindows()) {
        auto *quick=qobject_cast<QQuickWindow*>(window);
        if (quick) pending.append(quick->contentItem());
    }
    QSet<QObject*> visited;
    while (!pending.isEmpty()) {
        auto *object=pending.takeLast();
        if (!object || visited.contains(object)) continue;
        visited.insert(object); result.append(object); pending.append(object->children());
        if (auto *item=qobject_cast<QQuickItem*>(object))
            for (auto *child:item->childItems()) pending.append(child);
    }
    return result;
}
QObject *find(const char *name)
{
    for (auto *object:objects()) if (object->objectName()==QLatin1String(name)) return object;
    return nullptr;
}
void check(bool ok,const char *name)
{
    checks.append(QJsonObject{{"name",QString::fromUtf8(name)},{"passed",ok}});
    QFile progress(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("progress.json"));
    if(progress.open(QIODevice::WriteOnly))progress.write(QJsonDocument(QJsonObject{{"terminal",false},{"checks",checks}}).toJson());
    if (!ok) throw std::runtime_error(name);
}
void settle(int ms=120) { QTest::qWait(ms); }
void click(const char *name)
{
    auto *item=qobject_cast<QQuickItem*>(find(name));
    if (!item || !item->isVisible() || !item->isEnabled()) {
        QJsonObject details{{"name",QString::fromUtf8(name)},{"exists",item!=nullptr}};
        if(item){details.insert("visible",item->isVisible());details.insert("enabled",item->isEnabled());}
        if(auto *settings=find("juxtopposedGlobalSettings")){
            details.insert("settingsVisible",settings->property("visible").toBool());
            details.insert("categoryPages",QJsonValue::fromVariant(settings->property("categoryPages")));
        }
        report.insert("failedTarget",details);
        if(ownedWindow)ownedWindow->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("failed-window.png"));
    }
    check(item && item->isVisible() && item->isEnabled(),name);
    ownedWindow=item->window();
    check(ownedWindow!=nullptr,"target belongs to an owned QQuickWindow");
    ownedWindow->requestActivate();item->forceActiveFocus(Qt::TabFocusReason);settle(60);
    auto point=item->mapToScene(QPointF(item->width()/2,item->height()/2)).toPoint();
    QTest::mouseClick(ownedWindow,Qt::LeftButton,Qt::NoModifier,point);settle();
}
void type(const char *name,const QString &value)
{
    click(name);QTest::keyClick(ownedWindow,Qt::Key_A,Qt::ControlModifier);
    if(value.isEmpty())QTest::keyClick(ownedWindow,Qt::Key_Backspace);
    else {
        QKeyEvent press(QEvent::KeyPress,0,Qt::NoModifier,value);
        QCoreApplication::sendEvent(ownedWindow,&press);
    }
    QTest::keyClick(ownedWindow,Qt::Key_Return);settle();
}
void finish(bool passed,const QString &error={})
{
    report.insert("passed",passed);report.insert("error",error);report.insert("checks",checks);
    report.insert("ownedWindowOnly",true);report.insert("ordinaryGuiContainsQa",false);
    report.insert("fullBetaAccepted",false);report.insert("windowsDpiMatrixAccepted",false);
    QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));
    if(output.open(QIODevice::WriteOnly))output.write(QJsonDocument(report).toJson());
    _flushall();
    QTimer::singleShot(200,qApp,[]{
        InterlockedExchange(&ownedShutdownStarted,1);
        if(context)libvlc_Quit(vlc_object_instance(context->getIntf()));else QCoreApplication::quit();
    });
}
void until(const std::function<bool()> &ready,const char *name,int timeout=8000)
{
    QDeadlineTimer end(timeout);
    while(!ready()&&!end.hasExpired())QTest::qWait(20);
    check(ready(),name);settle(80);
}
QVariantMap row(AddonsModel *model,int index){return model->getDataAt(index);}
QList<QVariantMap> rows(AddonsModel *model)
{
    QList<QVariantMap> result;
    for(int i=0;i<model->rowCount();++i)result.append(row(model,i));
    return result;
}
QString managedKey(AddonsModel *model)
{
    for(const auto &value:rows(model))if(value.value("manageable").toBool())return value.value("selectionKey").toString();
    return {};
}
int indexFor(AddonsModel *model,const QString &key){for(int i=0;i<model->rowCount();++i)if(row(model,i).value("selectionKey").toString()==key)return i;return -1;}
QByteArray bytes(const QString &path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{};}
void picker(bool cancel,const QString &path)
{
    bool seen=false,prepared=false,timedOut=false;
    QPointer<QFileDialog> current;
    QDeadlineTimer deadline(12000);
    QTimer timer;timer.setInterval(150);
    QObject::connect(&timer,&QTimer::timeout,qApp,[&]{
        if(!current)for(auto *widget:QApplication::topLevelWidgets())
            if(auto *dialog=qobject_cast<QFileDialog*>(widget);dialog&&dialog->isVisible()){current=dialog;break;}
        if(deadline.hasExpired()){
            timedOut=true;if(current)current->reject();return;
        }
        if(!current||!current->isVisible())return;
        if(!seen){
            seen=true;report.insert("lastPicker",cancel?QStringLiteral("cancel"):QStringLiteral("accept"));
            current->grab().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(cancel?"picker-cancel.png":"picker-accept.png"));
        }
        if(cancel){current->reject();return;}
        if(!prepared){
            current->setDirectory(QFileInfo(path).absolutePath());
            if(auto *focus=QApplication::focusWidget();focus&&current->isAncestorOf(focus))focus->clearFocus();
            current->selectFile(path);prepared=true;return;
        }
        const auto selected=current->selectedFiles();
        report.insert("lastPickerSelection",QJsonArray::fromStringList(selected));
        if(selected.size()==1&&QFileInfo(selected.first()).absoluteFilePath()==QFileInfo(path).absoluteFilePath())
            QMetaObject::invokeMethod(current,"accept",Qt::QueuedConnection);
        else {
            if(auto *focus=QApplication::focusWidget();focus&&current->isAncestorOf(focus))focus->clearFocus();
            current->selectFile(path);
        }
    });
    timer.start();click("extensionsAdd");timer.stop();
    check(seen,"Add opens the real owned QFileDialog");
    check(!timedOut,"owned file chooser accepts or cancels within its bounded wait");
}
void run()
{
    try{
        check(context&&ownedWindow,"real MainCtx and native window ready");
        ownedWindow->resize(1500,1000);ownedWindow->requestActivate();settle();
        const auto ownedHwnd=reinterpret_cast<HWND>(ownedWindow->winId());
        if(context->useClientSideDecoration())for(auto *button:context->csdButtonModel()->windowCSDButtons()){
            const auto rect=button->rect();if(rect.isEmpty())continue;
            POINT point{qRound(rect.center().x()*ownedWindow->devicePixelRatio()),qRound(rect.center().y()*ownedWindow->devicePixelRatio())};
            ClientToScreen(ownedHwnd,&point);
            const auto hit=SendMessageW(ownedHwnd,WM_NCHITTEST,0,MAKELPARAM(point.x,point.y));
            const auto expected=button->type()==CSDButton::Close?HTCLOSE:button->type()==CSDButton::Minimize?HTMINBUTTON:button->type()==CSDButton::MaximizeRestore?HTMAXBUTTON:0;
            if(expected)check(hit==expected,"native caption button hit testing still works while GUI is alive");
        }
        const auto lateEventFile=QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("late-window-event.json");
        QObject::connect(context,&QObject::destroyed,qApp,[ownedHwnd,lateEventFile]{
            DWORD pid=0;GetWindowThreadProcessId(ownedHwnd,&pid);
            const bool alive=IsWindow(ownedHwnd)&&pid==GetCurrentProcessId();
            if(alive){RECT rect{};GetWindowRect(ownedHwnd,&rect);SendMessageW(ownedHwnd,WM_NCHITTEST,0,MAKELPARAM(rect.left+5,rect.top+5));}
            QFile file(lateEventFile);if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"windowAlive",alive},{"ownedLateHitTestSent",alive}}).toJson());
        });
        auto *service=context->getExtensionsPreferences();check(service!=nullptr,"MainCtx owns Extensions service");
        auto *model=service->addons();check(model!=nullptr,"service owns actual AddonsModel");
        auto *extensions=ExtensionsManager::getInstance(context->getIntf());
        report.insert("startupEngineLoaded",extensions->isLoaded());
        const auto mode=qEnvironmentVariable("VLC_OWNED_UI_MODE");
        report.insert("startupPolicy",config_GetInt("qt-autoload-extensions")!=0);
        if(mode=="edit"||mode=="startup-on")check(extensions->isLoaded(),"saved On loads installed extension metadata before opening any menu");
        if(mode=="reopen"||mode=="override-off")check(!extensions->isLoaded(),"effective startup Off leaves extension metadata unloaded");
        check(!model->available(),"add-on catalog remains lazy before first settings use");
        DialogsProvider::getInstance()->showGlobalSettingsDialog();settle(200);click("globalCategoryextensions");
        auto *page=find("juxtopposedExtensionsSettings");check(page!=nullptr,"original Extensions page mounts");
        check(qvariant_cast<QObject*>(page->property("service"))==service&&qvariant_cast<QObject*>(page->property("addons"))==model,"compiled page binds real controller and model");
        check(model->available(),"first page use initializes actual catalog manager");
        until([&]{return !model->loading();},"actual catalog cache becomes ready");
        ownedWindow->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("initial-window.png"));
        const auto package=qEnvironmentVariable("VLC_OWNED_ADDONS_PACKAGE");
        if(mode=="empty"){
            check(model->getCount()==0&&model->lastError().isEmpty(),"fresh native profile renders original empty design");
            check(service->values().value("loadOnStartup").toBool(),"original startup control defaults On");
            QJsonArray geometry;
            for(auto *object:objects())if(auto *item=qobject_cast<QQuickItem*>(object);item&&item->isVisible()){
                const auto image=item->property("source").toUrl().toString();
                if(!image.contains(QStringLiteral("extensions-settings-")))continue;
                const auto point=item->mapToScene(QPointF(0,0));
                geometry.append(QJsonObject{{"object",item->objectName()},{"source",image},{"width",item->width()},{"height",item->height()},{"scale",item->scale()},{"x",point.x()},{"y",point.y()},{"status",item->property("status").toInt()}});
            }
            report.insert("originalAssets",geometry);check(geometry.size()==16,"all original visible SVG slots render");
            finish(true);return;
        }
        const auto userdata=qEnvironmentVariable("VLC_USERDATA_PATH");
        const auto installed=QDir(userdata).filePath("lua/extensions/owned-beta-fixture.lua");
        if(mode=="live"){
            click("extensionsFindMore");
            until([&]{return !model->busy();},"actual Find more completes",30000);
            report.insert("liveCatalogError",model->lastError());report.insert("liveCatalogCount",int(model->getCount()));
            check(model->lastError().isEmpty(),"actual production catalog loads successfully");
            finish(true);return;
        }
        if(mode=="reopen"){
            check(!service->values().value("loadOnStartup").toBool(),"startup Off persists in real saved preferences");
            check(QFileInfo::exists(installed),"installed package files persist across process restart");
            until([&]{return !managedKey(model).isEmpty();},"native catalog restores installed managed entry");
            QMenu menu;VLCMenuBar::ExtensionsMenu(context->getIntf(),&menu);
            check(!extensions->isLoaded(),"saved Off prevents actual native extension menu loading");
            click("extensionsLoadOnStartup");check(config_GetInt("qt-autoload-extensions"),"startup On saves through actual checkbox");
            finish(true);return;
        }
        if(mode=="override-off"){
            check(service->values().value("loadOnStartup").toBool(),"command-line Off preserves saved On default");
            QMenu menu;VLCMenuBar::ExtensionsMenu(context->getIntf(),&menu);
            check(!extensions->isLoaded(),"native menu also respects command-line startup Off");
            finish(true);return;
        }
        if(mode=="startup-on"){
            check(service->values().value("loadOnStartup").toBool(),"saved startup On persists");
            QMenu menu;VLCMenuBar::ExtensionsMenu(context->getIntf(),&menu);
            check(extensions->isLoaded(),"saved On loads real extension metadata through native consumer");
            finish(true);return;
        }
        until([&]{return model->getCount()==2&&!row(model,0).value("selectionKey").toString().isEmpty()&&!row(model,1).value("selectionKey").toString().isEmpty();},"two actual manual scripts appear");
        check(row(model,0).value("selectionKey")!=row(model,1).value("selectionKey"),"manual nil-UUID rows have distinct stable keys");
        model->setSelectedKey(row(model,0).value("selectionKey").toString());
        check(!model->canRemove()&&!model->canInstall(),"manual scripts cannot enter managed mutation routes");
        click("extensionsList");QTest::keyClick(ownedWindow,Qt::Key_End);settle();
        check(model->selectedKey()==row(model,model->rowCount()-1).value("selectionKey").toString(),"End selects last actual row");
        QTest::keyClick(ownedWindow,Qt::Key_Home);settle();
        check(model->selectedKey()==row(model,0).value("selectionKey").toString(),"Home selects first actual row");
        const auto config=qEnvironmentVariable("VLC_OWNED_UI_CONFIG");
        const auto originalConfig=bytes(config);
        check(!service->setValue("loadOnStartup",QStringLiteral("false")),"startup API rejects nonboolean values");
        check(bytes(config)==originalConfig,"rejected startup value never writes config");
        service->reload();
        {
            const auto wideConfig=config.toStdWString();HANDLE cfgLock=CreateFileW(wideConfig.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
            check(cfgLock!=INVALID_HANDLE_VALUE,"real Windows handle denies startup config publication");
            click("extensionsLoadOnStartup");
            check(config_GetInt("qt-autoload-extensions")&&service->values().value("loadOnStartup").toBool()&&!service->lastError().isEmpty(),"failed checkbox save rolls back native and visible state");
            check(bytes(config)==originalConfig,"failed checkbox save preserves original config bytes");CloseHandle(cfgLock);
            click("extensionsLoadOnStartup");check(!config_GetInt("qt-autoload-extensions"),"unlocked startup checkbox retry saves Off");
            click("extensionsLoadOnStartup");check(config_GetInt("qt-autoload-extensions"),"startup checkbox restores On for current native consumer test");
        }
        picker(true,package);check(!model->busy()&&!QFileInfo::exists(installed),"cancelled chooser performs no install");
        picker(false,package);
        until([&]{return !model->busy()&&QFileInfo::exists(installed)&&!managedKey(model).isEmpty();},"accepted Unicode VLP installs through native manager");
        check(model->lastError().isEmpty(),"actual local package reports success");
        check(find("extensionsStatus")->property("text").toString()==model->statusText(),"visible add-on result replaces earlier saved-preference timing");
        const auto key=managedKey(model);check(!key.isEmpty(),"installed package has stable managed key");
        model->setSelectedKey(key);check(model->canRemove()&&!model->canInstall(),"installed selection enables Remove only");
        const auto priorIndex=indexFor(model,key);model->setSortOrder(Qt::DescendingOrder);until([&]{return !model->loading()&&indexFor(model,key)>=0&&indexFor(model,key)!=priorIndex;},"descending order actually moves selected row");
        check(model->selectedKey()==key&&model->canRemove(),"sorting preserves exact action identity");
        const int selectedRow=indexFor(model,key);check(selectedRow>0,"selected sorted row has an earlier neighbor");
        qobject_cast<QQuickItem*>(find("extensionsList"))->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyClick(ownedWindow,Qt::Key_Up);settle();
        check(model->selectedKey()==row(model,selectedRow-1).value("selectionKey").toString(),"keyboard Up moves from stable selection after sorting resets the view");
        model->setSelectedKey(key);
        type("extensionsSearch",QStringLiteral("Alpha"));
        until([&]{return model->getCount()==1&&!model->loading();},"actual local search filters manual script");
        check(model->selectedKey().isEmpty()&&!model->canRemove(),"filtering clears a hidden action selection");
        type("extensionsSearch",QString{});until([&]{return model->getCount()==3&&!model->loading();},"cleared search restores actual items");
        type("extensionsSearch",QStringLiteral("Zulu"));until([&]{return model->getCount()==1&&!model->loading();},"second keyboard query filters actual items");
        QTest::keyClick(ownedWindow,Qt::Key_Escape);settle();
        until([&]{return model->getCount()==3&&!model->loading();},"Escape clears local search through the original field handler");
        check(find("juxtopposedGlobalSettings")->property("visible").toBool(),"clearing search with Escape keeps settings open");
        model->setSelectedKey(key);
        const auto originalFile=bytes(installed);
        const auto wide=installed.toStdWString();HANDLE lock=CreateFileW(wide.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        check(lock!=INVALID_HANDLE_VALUE,"real Windows handle denies script deletion");
        click("extensionsRemove");until([&]{return !model->busy();},"locked Remove completes with native failure");
        check(!model->lastError().isEmpty()&&bytes(installed)==originalFile&&model->canRemove()&&model->statusText().isEmpty(),"failed removal preserves file/retry action and clears stale progress");
        CloseHandle(lock);click("extensionsRemove");until([&]{return !model->busy()&&!QFileInfo::exists(installed);},"unlocked Remove retries successfully");
        check(model->lastError().isEmpty()&&model->canInstall(),"removed selected entry enables actual Add");
        click("extensionsAdd");until([&]{return !model->busy()&&QFileInfo::exists(installed);},"Add installs selected catalog entry without chooser");
        check(bytes(installed)==originalFile,"selected reinstall preserves exact fixture bytes");
        const auto catalog=QDir(userdata).filePath("catalog.xml");const auto catalogBefore=bytes(catalog);const auto wideCatalog=catalog.toStdWString();
        HANDLE catalogLock=CreateFileW(wideCatalog.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        check(catalogLock!=INVALID_HANDLE_VALUE,"real Windows handle denies catalog replacement");
        click("extensionsRemove");until([&]{return !model->busy();},"storage completes despite locked catalog");
        check(!QFileInfo::exists(installed)&&model->catalogSaveNeeded()&&!model->lastError().isEmpty()&&model->statusText().isEmpty(),"actual catalog failure separates completed file work from pending save");
        check(bytes(catalog)==catalogBefore,"failed catalog publication preserves original bytes");
        click("extensionsRetryCatalog");check(model->catalogSaveNeeded()&&!model->lastError().isEmpty(),"locked catalog retry remains available");
        CloseHandle(catalogLock);click("extensionsRetryCatalog");
        check(!model->catalogSaveNeeded()&&model->lastError().isEmpty()&&!QFileInfo::exists(installed),"unlocked UI retry saves catalog without repeating storage");
        click("extensionsAdd");until([&]{return !model->busy()&&QFileInfo::exists(installed);},"selected Add restores package after catalog recovery");
        check(bytes(installed)==originalFile,"catalog recovery preserves exact selected package bytes");
        const auto *sameModel=model;click("globalCategoryrecording");click("globalCategoryextensions");
        check(service->addons()==sameModel&&model->selectedKey()==key,"page navigation retains manager and selection");
        {
            auto clone=std::make_unique<AddonsModel>();clone->setCtx(context);clone->setCtx(nullptr);clone->setCtx(context);
            until([&]{if(clone->loading()||clone->getCount()!=3)return false;QSet<QString> values;for(const auto &v:rows(clone.get()))values.insert(v.value("selectionKey").toString());return values.size()==3&&!values.contains(QString{});},"obsolete callback generations do not repopulate reset model");
            auto a=rows(clone.get());QSet<QString> keys;for(const auto &v:a)keys.insert(v.value("selectionKey").toString());
            check(keys.size()==3,"generation reset preserves distinct current row identities");
        }
        click("globalSettingsClose");DialogsProvider::getInstance()->pluginDialog();settle();
        check(DialogsProvider::getInstance()->pluginDialogVisible(),"existing native Plugins route remains functional");
        bool shared=false;
        for(auto *widget:QApplication::topLevelWidgets())if(auto *dialog=qobject_cast<PluginDialog*>(widget);dialog&&dialog->isVisible()){
            for(auto *view:dialog->findChildren<QListView*>())if(view->model()==model)shared=true;
            check(QMetaObject::invokeMethod(dialog,"close"),"owned native Plugins window closes through its native slot");
        }
        check(shared,"native Plugins view shares exact application AddonsModel");
        settle();DialogsProvider::getInstance()->showGlobalSettingsDialog();settle();click("globalCategoryextensions");
        check(service->addons()==model&&model->selectedKey()==key,"closing native Plugins preserves application manager and selection");
        QMenu menu;VLCMenuBar::ExtensionsMenu(context->getIntf(),&menu);check(extensions->isLoaded(),"existing native extension policy loads actual module");
        extension_t *active=nullptr;auto *engine=extensions->getManager();
        vlc_mutex_lock(&engine->lock);
        extension_t *candidate;ARRAY_FOREACH(candidate,engine->extensions)if(QString::fromUtf8(candidate->psz_title)==QStringLiteral("Owned startup marker"))active=candidate;
        vlc_mutex_unlock(&engine->lock);
        check(active!=nullptr,"actual Lua engine discovers owned startup fixture");
        QAction *activate=nullptr;for(auto *action:menu.actions())if(action->text().contains(QStringLiteral("Owned startup marker")))activate=action;
        check(activate!=nullptr,"actual native menu exposes owned extension action");activate->trigger();
        until([&]{return extension_IsActivated(engine,active);},"actual Lua extension activates through native menu action");
        click("extensionsLoadOnStartup");check(!config_GetInt("qt-autoload-extensions"),"startup Off saved through original checkbox");
        check(extensions->getManager()==engine&&extension_IsActivated(engine,active),"saving startup Off preserves the active Lua extension");
        check(extensions->isLoaded(),"saving Off does not unload currently loaded extension menus");
        click("extensionsAdvanced");settle();check(DialogsProvider::getInstance()->prefsDialogVisible(),"Advanced opens actual native expert preferences");
        for(auto *widget:QApplication::topLevelWidgets())if(auto *dialog=qobject_cast<QDialog*>(widget);dialog&&dialog->isVisible())dialog->reject();
        settle();check(!DialogsProvider::getInstance()->prefsDialogVisible(),"native Advanced closes normally");
        finish(true);
    }catch(const std::exception &error){finish(false,QString::fromUtf8(error.what()));}
}
void waitForWindow(int attempts)
{
    auto *host=qobject_cast<QQuickItem*>(find("juxtopposedDialogsHost"));
    if (host && host->window() && host->isVisible()) {
        auto *engine=qmlEngine(host);
        auto *candidate=engine ? engine->singletonInstance<MainCtx*>("VLC.MainInterface","MainCtx") : nullptr;
        if(candidate && find("juxtopposedGlobalSettings")) {
            context=candidate;ownedWindow=host->window();
            QTimer::singleShot(250,qApp,run);return;
        }
    }
    if (attempts>0) QTimer::singleShot(250,qApp,[attempts]{waitForWindow(attempts-1);});
    else {
        QJsonArray inventory;
        for(auto *object:objects())if(!object->objectName().isEmpty())inventory.append(QJsonObject{{"name",object->objectName()},{"class",QString::fromLatin1(object->metaObject()->className())}});
        report.insert("startupObjects",inventory);
        finish(false,QStringLiteral("Actual settings host did not become ready"));
    }
}
void startOwnedJourney()
{
    if (qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT")) return;
    _set_error_mode(_OUT_TO_STDERR);
    installOwnedFaultCapture();
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer::singleShot(1000,qApp,[]{waitForWindow(120);});
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedJourney)
