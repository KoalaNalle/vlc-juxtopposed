// SPDX-License-Identifier: GPL-2.0-or-later
// Private in-process QtTest driver. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "qt.hpp"
#include "dialogs/dialogs_provider.hpp"
#include "dialogs/conversion_controller.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/compositor.hpp"
#include "maininterface/history_media_source.hpp"
#include "maininterface/sidebar_pins_model.hpp"
#include "maininterface/privacy_preferences_model.hpp"
#include "maininterface/general_preferences_model.hpp"
#include "maininterface/controls_preferences_model.hpp"
#include "maininterface/player_profiles_model.hpp"
#include "maininterface/player_profile_layout.hpp"
#include "maininterface/playback_preferences_model.hpp"
#include "maininterface/audio_preferences_model.hpp"
#include "maininterface/interface_preferences_model.hpp"
#include "maininterface/video_preferences_model.hpp"
#include "maininterface/subtitles_preferences_model.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhomeactivity.hpp"
#include "medialibrary/mlcurrentmedia.hpp"
#include "medialibrary/mlplaylistmodel.hpp"
#include "medialibrary/mlrecentmediamodel.hpp"
#include "medialibrary/mlhelper.hpp"
#include "player/player_controller.hpp"
#include "playlist/playlist_controller.hpp"
#include <QLockFile>
#include "player/control_list_model.hpp"
#include "player/player_controlbar_model.hpp"
#include <QJSValue>
#include "util/navigation_history.hpp"
#include "util/qt_dirs.hpp"
#include "util/csdbuttonmodel.hpp"
#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <vlc_interface.h>
#include <vlc_player.h>
#include <vlc_playlist.h>
#include <QApplication>
#include <QAccessible>
#include <QAbstractEventDispatcher>
#include <QImage>
#include <QtQuick/private/qquickpixmap_p.h>
#include <QtQuick/private/qquickpixmapcache_p.h>
#include <QAbstractNativeEventFilter>
#include <QThread>
#include <atomic>
#include <QAction>
#include <QFileInfo>
#include <QQmlPropertyMap>
#include <vlc_input_item.h>
#include <vlc_configuration.h>
#include <vlc_aout.h>
#include <cmath>
#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QDataStream>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMetaMethod>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>
#include <QSaveFile>
#include <QSignalSpy>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QWindow>
#include <QWheelEvent>
#include <QVector3D>
#include <windows.h>
#include <functional>
#include <memory>
#include <stdexcept>
#include <stdlib.h>

namespace {
MainCtx *context = nullptr;
QQuickWindow *window = nullptr;
QJsonArray checks;
QJsonObject report;
SidebarPinsModel *pins() { return context->getSidebarPins(); }
PlayerController *player() { return context->getIntf()->p_mainPlayerController; }
QQuickItem *named(const QString &);
NavigationHistory *history() { return qmlEngine(named(QStringLiteral("juxtopposedDialogsHost")))->singletonInstance<NavigationHistory *>("VLC.MainInterface","History"); }
QList<QObject *> objects()
{
    QList<QObject *> pending, found;
    for (auto *candidate : QGuiApplication::allWindows())
        if (auto *quick = qobject_cast<QQuickWindow *>(candidate)) pending.append(quick->contentItem());
    QSet<QObject *> seen;
    while (!pending.isEmpty()) {
        auto *item = pending.takeLast();
        if (!item || seen.contains(item)) continue;
        seen.insert(item); found.append(item); pending.append(item->children());
        if (auto *quick = qobject_cast<QQuickItem *>(item))
            for (auto *child : quick->childItems()) pending.append(child);
    }
    return found;
}
QQuickItem *named(const QString &name)
{
    for (auto *object : objects()) if (object->objectName() == name)
        if (auto *item = qobject_cast<QQuickItem *>(object); item && item->isVisible()) return item;
    return nullptr;
}
void check(bool ok, const char *name)
{
    checks.append(QJsonObject{{"name", QString::fromUtf8(name)}, {"passed", ok}});
    QFile progress(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("progress.json"));
    if (progress.open(QIODevice::WriteOnly)) progress.write(QJsonDocument(QJsonObject{{"checks",checks}}).toJson());
    if (!ok) throw std::runtime_error(name);
}
void wait(const std::function<bool()> &ready, const char *name, int timeout = 10000)
{
    QDeadlineTimer deadline(timeout);
    while (!ready() && !deadline.hasExpired()) QTest::qWait(20);
    check(ready(), name); QTest::qWait(60);
}
QVariant expression(QQuickItem *item, const QString &code)
{
    check(item != nullptr, "owned QML target exists");
    QQmlExpression evaluate(qmlContext(item), item, code);
    const auto value = evaluate.evaluate();
    if (evaluate.hasError()) report.insert("failedQmlOperation", evaluate.error().toString());
    check(!evaluate.hasError(), "owned QML operation succeeds"); return value;
}
QQuickItem *pinRow(int index)
{
    auto *list = named(QStringLiteral("juxtopposedSidebarPinList"));
    check(list != nullptr, "actual pin ListView exists");
    expression(list, QStringLiteral("positionViewAtIndex(%1, ListView.Contain); forceLayout()").arg(index));
    QTest::qWait(100);
    auto *row = named(QStringLiteral("juxtopposedSidebarPin%1").arg(index));
    check(row != nullptr, "real pin delegate materializes in viewport"); return row;
}
void revealFooter()
{
    expression(named(QStringLiteral("juxtopposedSidebarPinList")), QStringLiteral("positionViewAtEnd(); forceLayout()"));
    QTest::qWait(100);
}
void click(QQuickItem *item)
{
    check(item && item->isVisible() && item->isEnabled(), "actual pointer action is enabled");
    item->forceActiveFocus(Qt::TabFocusReason); window->requestActivate(); QTest::qWait(100);
    const auto point = item->mapToScene(item->boundingRect().center());
    check(QRectF(QPointF{}, QSizeF(window->size())).contains(point), "actual pointer target stays in owned window");
    for (auto *parent = item->parentItem(); parent; parent = parent->parentItem())
        if (parent->clip()) check(parent->boundingRect().contains(parent->mapFromScene(point)), "actual pointer target is inside clipped viewport");
    QTest::mouseMove(window, point.toPoint()); QTest::qWait(350);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point.toPoint()); QTest::qWait(100);
}
QMenu *menu()
{
    for (auto *widget : QApplication::topLevelWidgets())
        if (auto *candidate = qobject_cast<QMenu *>(widget); candidate && candidate->isVisible()) return candidate;
    return nullptr;
}
void chooseMenu(const QString &text)
{
    wait([] { return menu(); }, "actual native menu opens");
    auto *popup = menu(); QAction *action = nullptr;
    for (auto *entry : popup->actions()) if (entry->text() == text) action = entry;
    check(action && action->isEnabled(), "requested native menu action exists");
    QTest::mouseClick(popup, Qt::LeftButton, Qt::NoModifier, popup->actionGeometry(action).center());
    wait([] { return !menu(); }, "native menu closes after actual pointer selection");
}
void addMenu(const QString &text)
{ revealFooter(); click(named(QStringLiteral("juxtopposedSidebarAdd"))); chooseMenu(text); }
void pinMenu(int row, const QString &text)
{
    const bool wasActive=window->isActive();
    window->requestActivate();
    wait([]{return window->isActive();},"owned native window is active before its keyboard pin-menu input");
    QPointer<QQuickItem> item=pinRow(row);
    item->forceActiveFocus(Qt::TabFocusReason);
    wait([&]{return item && item->hasActiveFocus();},"real native pin retains stable focus before its context-menu chord");
    auto *navigation=named(QStringLiteral("juxtopposedNavigation"));
    check(navigation,"actual pin-menu navigation owner exists");
    int signal=-1;
    for(int index=0;index<navigation->metaObject()->methodCount();++index){
        const auto method=navigation->metaObject()->method(index);
        if(method.methodType()==QMetaMethod::Signal && method.name()=="pinMenuRequested"){signal=index;break;}
    }
    check(signal>=0,"actual native pin-menu request signal is discoverable");
    QSignalSpy requests(navigation,navigation->metaObject()->method(signal));
    check(requests.isValid(),"actual pin-menu request signal can be observed");
    struct KeyObserver : QObject {
        QJsonArray events;
        KeyObserver() { qApp->installEventFilter(this); }
        ~KeyObserver() { qApp->removeEventFilter(this); }
        bool eventFilter(QObject *receiver,QEvent *event) override {
            if (event->type()==QEvent::KeyPress || event->type()==QEvent::KeyRelease || event->type()==QEvent::ShortcutOverride) {
                const auto *key=static_cast<QKeyEvent *>(event);
                events.append(QJsonObject{{"event",int(event->type())},{"receiver",receiver->objectName()},
                    {"receiverClass",QString::fromLatin1(receiver->metaObject()->className())},
                    {"key",key->key()},{"modifiers",int(key->modifiers())}});
            }
            return false;
        }
    } keyObserver;
    const auto visualBefore=item->property("visualFocus").toBool();
    const auto reasonBefore=item->property("focusReason").toInt();
    QTest::keyClick(window,Qt::Key_F10,Qt::ShiftModifier);QTest::qWait(80);
    auto attempts=report.value("pinMenuInputAttempts").toArray();
    attempts.append(QJsonObject{{"row",row},{"windowWasActive",wasActive},{"windowActiveAfterKey",window->isActive()},
        {"targetAlive",!item.isNull()},{"targetFocusedAfterKey",item && item->hasActiveFocus()},
        {"focusAfterKey",window->activeFocusItem()?window->activeFocusItem()->objectName():QString()},
        {"requestSignals",requests.count()},{"menuVisible",menu()!=nullptr},{"canRetry",pins()->canRetry()},{"pinsBusy",pins()->busy()},
        {"visualFocusBefore",visualBefore},{"focusReasonBefore",reasonBefore},
        {"visualFocusAfter",item && item->property("visualFocus").toBool()},
        {"focusWindowIsMain",QGuiApplication::focusWindow()==window},
        {"activePopupWidget",QApplication::activePopupWidget()!=nullptr},
        {"keyboardGrabber",QWidget::keyboardGrabber()!=nullptr},{"keyEvents",keyObserver.events}});
    report.insert("pinMenuInputAttempts",attempts);
    check(requests.count()==1,"owned keyboard context-menu chord reaches the actual pin request signal once");
    chooseMenu(text);
}
void filePicker(const QString &path, bool cancel = false)
{
    QPointer<QFileDialog> dialog;
    wait([&] {
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *candidate = qobject_cast<QFileDialog *>(widget); candidate && candidate->isVisible()) dialog = candidate;
        return !dialog.isNull();
    }, "actual owned file chooser opens");
    check(dialog->windowHandle()->transientParent() == window, "pin chooser has actual main-window ownership");
    if (cancel) { dialog->reject(); QTest::qWait(100); return; }
    dialog->setDirectory(QFileInfo(path).absolutePath());
    if (auto *focus = QApplication::focusWidget(); focus && dialog->isAncestorOf(focus)) focus->clearFocus();
    dialog->selectFile(path);
    wait([&] {
        const auto selected = dialog->selectedFiles();
        return selected.size() == 1 && QFileInfo(selected.first()).absoluteFilePath() == QFileInfo(path).absoluteFilePath();
    }, "actual chooser selects exact owned Unicode target");
    QMetaObject::invokeMethod(dialog, "accept", Qt::QueuedConnection);
    wait([&] { return !dialog || !dialog->isVisible(); }, "actual chooser accepts asynchronously");
}
void home()
{
    if (context->isPlayerView()) context->setPlayerView(false);
    history()->push({QStringLiteral("home")}, {}, Qt::OtherFocusReason);
    wait([] { return named(QStringLiteral("homeSectioncontinueWatching")) || named(QStringLiteral("juxtopposedSidebarAdd")); }, "actual Home route loads");
}
QString key(int row) { return pins()->get(row).value(QStringLiteral("key")).toString(); }
int kindRow(const QString &kind)
{ for (int i = 0; i < pins()->count(); ++i) if (pins()->get(i).value(QStringLiteral("kind")).toString() == kind) return i; return -1; }
QJsonArray snapshot()
{ QJsonArray result; for (int i = 0; i < pins()->count(); ++i) result.append(QJsonObject::fromVariantMap(pins()->get(i))); return result; }
void capture(const QString &name)
{ check(window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(name)), "owned actual-window image saved"); }
void seedNativeCollections()
{
    auto *library = context->getMediaLibrary();
    const auto folder = QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"));
    library->addFolder(folder);
    wait([&] { return library->idle() && library->homeActivity()->mediaCount() == 3; }, "real native library indexes three generated files", 30000);
    struct Probe { bool ready = false; MLItemId playlist, artist; };
    auto result = std::make_shared<Probe>();
    const auto mrl = QUrl::fromLocalFile(QDir(folder.toLocalFile()).filePath(QStringLiteral("first.flac"))).toEncoded();
    library->runOnMLThread<Probe>(library, [mrl](vlc_medialibrary_t *ml, Probe &probe) {
        ml_unique_ptr<vlc_ml_media_t> media(vlc_ml_get_media_by_mrl(ml, mrl.constData()));
        ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_playlist_create(ml, "Owned pins mix Ää"));
        if (!media || !playlist || media->i_subtype != VLC_ML_MEDIA_SUBTYPE_ALBUMTRACK) return;
        const int64_t id = media->i_id;
        if (vlc_ml_playlist_append(ml, playlist->i_id, &id, 1) != VLC_SUCCESS) return;
        probe.playlist = {playlist->i_id, VLC_ML_PARENT_PLAYLIST};
        probe.artist = {media->album_track.i_artist_id, VLC_ML_PARENT_ARTIST};
    }, [result](quint64, Probe &probe) { *result = probe; result->ready = true; });
    wait([&] { return result->ready; }, "genuine playlist and tagged artist resolve on native worker");
    check(result->playlist.id > 0 && result->artist.id > 0, "native collection identities are real and typed");
    // These two creation requests deliberately use the real controller seam.
    // URI/Home creation and subsequent collection activation use actual UI.
    check(pins()->pinItem(result->playlist), "native playlist pin request is accepted");
    check(pins()->pinItem(result->artist), "native artist pin request is accepted in issue order");
    check(pins()->pinUrl(folder, QStringLiteral("source"), QStringLiteral("Owned source Ää")), "source request queues behind pending native collection resolution");
    wait([] { return !pins()->busy() && pins()->count() == 4; }, "queued heterogeneous additions persist in request order");
    check(kindRow(QStringLiteral("playlist")) == 1 && kindRow(QStringLiteral("artist")) == 2 && kindRow(QStringLiteral("source")) == 3, "worker completion does not reorder pin requests");
    library->addAndPlay(QUrl::fromEncoded(mrl));
    wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING && player()->getUrl().fileName() == QStringLiteral("first.flac"); }, "real tagged audio plays before Home pinning", 15000);
    auto *native=context->getIntf()->p_player;
    vlc_player_Lock(native); vlc_player_Stop(native); vlc_player_Unlock(native);
    wait([] { return player()->getPlayingState()==PlayerController::PLAYING_STATE_STOPPED; }, "seed audio completes through native stop before history expectations");
    library->addAndPlay(QUrl::fromEncoded(mrl));
    wait([] { return player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING; }, "native seed audio restarts after history commit",15000);
    home(); library->homeActivity()->refresh();
    wait([] { return named(QStringLiteral("homeaudio0")); }, "actual Home listening history supplies a real media card", 15000);
    auto *card = named(QStringLiteral("homeaudio0")); card->forceActiveFocus(Qt::TabFocusReason); QTest::qWait(100);
    QTest::keyClick(window, Qt::Key_F10, Qt::ShiftModifier); chooseMenu(QStringLiteral("Pin to sidebar"));
    wait([] { return !pins()->busy() && pins()->count() == 5; }, "actual Home context action pins real media");
}
vlc_tick_t nativeTime()
{
    auto *native=context->getIntf()->p_player;
    vlc_player_Lock(native); const auto time=vlc_player_GetTime(native); vlc_player_Unlock(native); return time;
}
void exerciseRoutes(bool nativeCollections)
{
    const auto mediaRow = kindRow(QStringLiteral("media")); check(mediaRow >= 0, "persisted media pin exists");
    click(pinRow(mediaRow));
    wait([] { return player()->getUrl().fileName() == QStringLiteral("first.flac") && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; }, "actual pin starts native audio playback", 15000);
    auto *input = player()->getInput(); input_item_Hold(input);
    std::unique_ptr<input_item_t, decltype(&input_item_Release)> hold(input, &input_item_Release);
    home();
    wait([] { return nativeTime() > VLC_TICK_FROM_MS(200); }, "real audio clock advances before pin navigation");
    auto previousTime=nativeTime();
    for (const auto &kind : {QStringLiteral("folder"), QStringLiteral("source")}) {
        click(pinRow(kindRow(kind)));
        wait([] { return history()->viewPath() == QStringList{QStringLiteral("network"),QStringLiteral("browse")}; }, "actual folder/source pin enters native browser route");
        bool populated = false;
        wait([&] {
            for (auto *object : objects()) if (QByteArray(object->metaObject()->className()).contains("NetworkMediaModel"))
                if (auto *model = qobject_cast<QAbstractItemModel *>(object); model && model->rowCount() >= 3
                    && QFileInfo(object->property("url").toUrl().toLocalFile()).canonicalFilePath() == QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).canonicalPath()) populated = true;
            return populated;
        }, "native pinned browser enumerates genuine owned files", 15000);
        check(player()->getInput() == input && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING && nativeTime() > previousTime, "browsing pin preserves exact playing primary input and advancing clock"); previousTime=nativeTime(); home();
    }
    auto *native=context->getIntf()->p_player;
    vlc_player_Lock(native); vlc_player_Pause(native); vlc_player_Unlock(native);
    wait([] { return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED; }, "primary pauses before native collection routes");
    QTest::qWait(300); const auto pausedTime=nativeTime(); const auto pausedPosition=player()->getPosition();
    for (const auto &kind : {QStringLiteral("playlist"), QStringLiteral("artist")}) {
        const auto row = kindRow(kind);
        QObject *navigationObject=nullptr;
        for (auto *object : objects()) if (QByteArray(object->metaObject()->className()).contains("JuxtopposedNavigation")) { navigationObject=object; break; }
        check(navigationObject!=nullptr,"owned native navigation signal source exists");
        QSignalSpy requested(navigationObject,SIGNAL(pinRequested(QString,int)));
        QSignalSpy resolved(pins(),&SidebarPinsModel::collectionRequested);
        QJsonArray navigationTrace; QObject traceOwner;
        QObject::connect(history(),&NavigationHistory::viewPathChanged,&traceOwner,[&](const QStringList &path){navigationTrace.append(QJsonObject{{"signal","viewPathChanged"},{"path",QJsonArray::fromStringList(path)}});});
        QObject::connect(history(),&NavigationHistory::navigate,&traceOwner,[&](Qt::FocusReason){navigationTrace.append(QJsonObject{{"signal","navigate"},{"path",QJsonArray::fromStringList(history()->viewPath())}});});
        click(pinRow(row));
        if (nativeCollections) {
            const QStringList expected = kind == QStringLiteral("playlist")
                ? QStringList{QStringLiteral("playlists"),QStringLiteral("list")}
                : QStringList{QStringLiteral("music"),QStringLiteral("artists"),QStringLiteral("albums")};
            QDeadlineTimer deadline(10000);
            while(history()->viewPath()!=expected&&!deadline.hasExpired())QTest::qWait(20);
            auto traces=report.value("collectionActivationTrace").toArray();
            traces.append(QJsonObject{{"kind",kind},{"pinRequests",requested.count()},{"resolvedRequests",resolved.count()},{"serviceError",pins()->error()},{"trace",navigationTrace}});
            report.insert("collectionActivationTrace",traces);
            check(history()->viewPath()==expected,"actual native collection pin opens its correct typed route");
            const auto descriptor = pins()->get(row);
            const MLItemId expectedId(descriptor.value(QStringLiteral("nativeId")).toString().toLongLong(),
                kind == QStringLiteral("playlist") ? VLC_ML_PARENT_PLAYLIST : VLC_ML_PARENT_ARTIST);
            const auto property = kind == QStringLiteral("playlist") ? QStringLiteral("parentId") : QStringLiteral("artistId");
            check(history()->viewProp()->value(property).value<MLItemId>() == expectedId, "actual route preserves exact native collection ID and parent type");
            wait([&] {
                for (auto *object : objects()) if (object->inherits(kind == QStringLiteral("playlist") ? "MLPlaylistModel" : "MLAlbumModel")
                    && object->property("parentId").value<MLItemId>() == expectedId && object->property("count").toInt() == 1) return true;
                return false;
            }, "actual pinned collection displays genuine native content", 15000);
        } else wait([] { return !pins()->error().isEmpty(); }, "unavailable collection reports failure without wrong navigation");
        check(player()->getInput() == input && player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED && nativeTime()==pausedTime && player()->getPosition()==pausedPosition, "collection pin preserves exact paused input native clock and position");
        if (!nativeCollections) { check(history()->viewPath().first() == QStringLiteral("home"), "unavailable collection leaves current route unchanged"); pins()->dismissError(); }
        home();
    }
}
QAbstractItemModel *recentMediaModel()
{
    for (auto *object : objects())
        if (object->objectName() == QStringLiteral("juxtopposedRecentMediaModel"))
            return qobject_cast<QAbstractItemModel *>(object);
    return nullptr;
}
QQuickItem *recentCard(const QString &viewName, int row)
{
    auto *view = named(viewName);
    if (!view) return nullptr;
    for (auto *object : objects())
        if (auto *item = qobject_cast<QQuickItem *>(object); item && view->isAncestorOf(item)
            && item->objectName() == QStringLiteral("homeCollectionItem%1").arg(row)) return item;
    return nullptr;
}
int recentRow(const QUrl &url)
{
    auto *model = recentMediaModel();
    if (!model) return -1;
    for (int row = 0; row < model->rowCount(); ++row)
        if (model->data(model->index(row, 0), MLMediaModel::MEDIA_MRL).toUrl() == url) return row;
    return -1;
}
QQuickItem *revealRecentMedia(const QUrl &url)
{
    int row = -1;
    wait([&] { row = recentRow(url); return row >= 0; }, "real requested URL resolves in native Recent model", 15000);
    auto *view = named(QStringLiteral("juxtopposedRecentMediaView"));
    check(view != nullptr, "actual native Recent media view exists");
    QQuickItem *grid = nullptr;
    for (auto *object : objects())
        if (auto *item = qobject_cast<QQuickItem *>(object); item && view->isAncestorOf(item) && item->inherits("QQuickGridView")) { grid = item; break; }
    check(grid != nullptr, "actual Recent GridView resolves without private QML identifiers");
    expression(grid, QStringLiteral("positionViewAtIndex(%1, 0); forceLayout()").arg(row));
    wait([&] { return recentCard(QStringLiteral("juxtopposedRecentMediaView"), row) != nullptr; }, "genuine requested Recent card materializes");
    return recentCard(QStringLiteral("juxtopposedRecentMediaView"), row);
}
void sidebarRoute(const QString &key)
{
    expression(named(QStringLiteral("juxtopposedSidebarPinList")), QStringLiteral("positionViewAtBeginning(); forceLayout()"));
    QTest::qWait(100);
    click(named(QStringLiteral("juxtopposedNavigation") + key));
}
QString cleanActionText(QAction *action)
{
    auto text = action->text(); text.remove(QLatin1Char('&')); return text;
}
void toggleNavigation()
{
    if (auto *toggle = named(QStringLiteral("juxtopposedStripnavigation"))) { click(toggle); return; }
    click(named(QStringLiteral("juxtopposedStripmenu")));
    wait([] { return menu(); }, "actual original-strip global menu opens");
    auto *popup = menu(); QAction *view = nullptr;
    for (auto *action : popup->actions()) if (cleanActionText(action) == QStringLiteral("View")) view = action;
    check(view && view->menu(), "native global View submenu exists");
    popup->setActiveAction(view); QTest::keyClick(popup, Qt::Key_Right);
    auto *submenu = view->menu(); wait([&] { return submenu->isVisible(); }, "actual keyboard action opens native View submenu");
    QAction *navigation = nullptr;
    for (auto *action : submenu->actions()) if (cleanActionText(action) == QStringLiteral("Navigation Menu")) navigation = action;
    check(navigation && navigation->isCheckable() && navigation->isEnabled(), "native View menu exposes a genuine checked navigation action");
    submenu->setActiveAction(navigation); QTest::keyClick(submenu, Qt::Key_Return);
    wait([] { return !menu(); }, "native navigation selection dismisses its real menu");
}
void verifySidebarIcons();
void exerciseRecent(bool nativeCollections)
{
    home();
    verifySidebarIcons();
    auto *navigation = named(QStringLiteral("juxtopposedNavigation"));
    check(navigation != nullptr, "actual redesigned sidebar exists");
    const auto scale = navigation->property("uiScale").toReal();
    const auto expectedWidth = (navigation->property("expanded").toBool() ? 217 : 50) * scale;
    check(qAbs(navigation->width() - expectedWidth) < 0.1, "actual sidebar width uses original217/50 logical geometry");
    for (const auto &key : {QStringLiteral("home"), QStringLiteral("network"), QStringLiteral("playlists"), QStringLiteral("recent")}) {
        wait([&] { auto *art = named(QStringLiteral("juxtopposedNavigationArt") + key); return art && art->property("status").toInt() == 1; }, "genuine original route SVG loads through native image provider");
        auto *art = named(QStringLiteral("juxtopposedNavigationArt") + key);
        check(art->width() == 24 && art->height() == 24 && qAbs(art->scale() - scale) < 0.01, "original route SVG retains intrinsic24px and scaled wrapper geometry");
    }
    auto *toggleInput = player()->getInput(); const auto toggleTime = nativeTime(); const auto togglePosition = player()->getPosition();
    if (!navigation->property("expanded").toBool()) {
        toggleNavigation();
        wait([&] { return navigation->property("expanded").toBool(); }, "actual toggle expands original sidebar");
    }
    expression(named(QStringLiteral("juxtopposedSidebarPinList")), QStringLiteral("positionViewAtBeginning(); forceLayout()"));
    check(qAbs(navigation->width() - 217 * scale) < 0.1, "actual expanded sidebar is217 logical pixels");
    QTest::mouseMove(window, QPoint(window->width() - 20, 120)); QTest::qWait(30);
    capture(QStringLiteral("native-sidebar-expanded.png"));
    toggleNavigation();
    wait([&] { return !navigation->property("expanded").toBool() && qAbs(navigation->width() - 50 * scale) < 0.1; }, "actual toggle produces original50px compact rail");
    QTest::mouseMove(window, QPoint(window->width() - 20, 120)); QTest::qWait(30);
    capture(QStringLiteral("native-sidebar-compact.png"));
    toggleNavigation();
    wait([&] { return navigation->property("expanded").toBool() && qAbs(navigation->width() - 217 * scale) < 0.1; }, "actual toggle restores expanded sidebar geometry");
    check(player()->getInput() == toggleInput && player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED
        && nativeTime() == toggleTime && player()->getPosition() == togglePosition, "actual sidebar toggles preserve exact paused input native time and position");
    auto *input = player()->getInput();
    auto pausedTime = nativeTime();
    auto pausedPosition = player()->getPosition();
    check(player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED, "Recent journey begins with genuine paused primary playback");
    history()->push({QStringLiteral("network")}, {}, Qt::OtherFocusReason);
    sidebarRoute(QStringLiteral("home"));
    wait([] { return history()->viewPath().first() == QStringLiteral("home"); }, "actual Home sidebar button returns from Network");
    check(player()->getInput() == input && nativeTime() == pausedTime && player()->getPosition() == pausedPosition, "Home sidebar navigation preserves paused primary input and both clocks");
    auto *recent = named(QStringLiteral("juxtopposedNavigationrecent"));
    check(recent != nullptr, "actual Recent route button exists");
    if (!context->hasMediaLibrary()) {
        check(recent->isEnabled(), "Recent remains available with Library browsing disabled");
        report.insert("recentLibraryOffAccepted", false);
        report.insert("recentButtonEnabledWithoutLibrary", recent->isEnabled());
        return;
    }
    check(recent->isEnabled(), "actual Recent route is available with the native Library frontend");
    if (!nativeCollections) return;
    sidebarRoute(QStringLiteral("recent"));
    const QStringList mediaPath{QStringLiteral("recent"), QStringLiteral("media")};
    const QStringList playlistPath{QStringLiteral("recent"), QStringLiteral("playlists")};
    wait([&] { return history()->viewPath() == mediaPath && named(QStringLiteral("juxtopposedRecentPage")); }, "actual Recent sidebar opens native media history leaf");
    wait([] { return recentMediaModel() != nullptr; }, "actual native Recent model exists");
    wait([] { auto *model = recentMediaModel(); if (!model) return false; model->rowCount(); return !model->property("loading").toBool(); }, "native Recent count query completes before empty-state expectation", 15000);
    const auto localAudio = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    if (recentMediaModel()->rowCount() == 0) {
        check(qEnvironmentVariable("VLC_OWNED_UI_MODE") == QStringLiteral("reopen"), "only explicitly cleared cold history needs a new playback commit");
        capture(QStringLiteral("native-recent-cleared-empty.png"));
        auto *native = context->getIntf()->p_player;
        vlc_player_Lock(native); vlc_player_Stop(native); vlc_player_Unlock(native);
        wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_STOPPED; }, "new cold playback ends through normal native stop");
        wait([] { auto *model = recentMediaModel(); return model && model->rowCount() > 0; }, "actual Recent empty state updates after native playback commit", 15000);
        context->getMediaLibrary()->addAndPlay(localAudio);
        wait([&] { return player()->getUrl() == localAudio && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; }, "cold local audio restarts normally after history commit", 15000);
        vlc_player_Lock(native); vlc_player_Pause(native); vlc_player_Unlock(native);
        wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED; }, "fresh cold primary pauses before retained-navigation checks");
        QTest::qWait(250);
        input = player()->getInput(); pausedTime = nativeTime(); pausedPosition = player()->getPosition();
        home(); sidebarRoute(QStringLiteral("recent"));
        wait([&] { return history()->viewPath() == mediaPath && recentMediaModel(); }, "Recent reloads with the genuinely committed cold history");
    }
    wait([] { auto *model = recentMediaModel(); return model && model->rowCount() > 0; }, "Recent media is populated from real native history", 15000);
    capture(QStringLiteral("native-recent-media.png"));
    click(named(QStringLiteral("juxtopposedRecentPlaylistsTab")));
    wait([&] { auto *page = named(QStringLiteral("juxtopposedRecentPage")); return history()->viewPath() == playlistPath && page && page->property("showPlaylists").toBool(); }, "actual Recent playlist tab has its own native history leaf");
    wait([] { auto *view = named(QStringLiteral("juxtopposedRecentPlaylistsView")); return view && view->property("count").toInt() > 0; }, "Recent playlists come from actual successful native visits", 15000);
    check(player()->getInput() == input && nativeTime() == pausedTime && player()->getPosition() == pausedPosition, "Recent tabs preserve exact paused input and position");
    capture(QStringLiteral("native-recent-playlists.png"));
    click(recentCard(QStringLiteral("juxtopposedRecentPlaylistsView"), 0));
    wait([] { return history()->viewPath() == QStringList{QStringLiteral("playlists"), QStringLiteral("list")}; }, "actual Recent playlist row opens its native collection");
    const auto descriptor = pins()->get(kindRow(QStringLiteral("playlist")));
    check(history()->viewProp()->value(QStringLiteral("parentId")).value<MLItemId>() == MLItemId(descriptor.value(QStringLiteral("nativeId")).toString().toLongLong(), VLC_ML_PARENT_PLAYLIST), "Recent playlist activation preserves genuine typed native ID");
    history()->previous();
    wait([&] { auto *page = named(QStringLiteral("juxtopposedRecentPage")); return history()->viewPath() == playlistPath && page && page->property("showPlaylists").toBool(); }, "Back restores Recent playlist tab rather than switching to media");
    history()->next();
    wait([] { return history()->viewPath() == QStringList{QStringLiteral("playlists"), QStringLiteral("list")}; }, "Forward restores native playlist destination");
    history()->previous();
    wait([&] { return history()->viewPath() == playlistPath && named(QStringLiteral("juxtopposedRecentMediaTab")); }, "Back returns to the same Recent leaf again");
    click(named(QStringLiteral("juxtopposedRecentMediaTab")));
    wait([&] { return history()->viewPath() == mediaPath && recentCard(QStringLiteral("juxtopposedRecentMediaView"), 0); }, "actual Media tab restores native history cards");
    click(revealRecentMedia(localAudio));
    wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING && player()->getUrl().fileName() == QStringLiteral("first.flac"); }, "actual Recent media card starts genuine local audio", 15000);
    const QUrl stream(qEnvironmentVariable("VLC_OWNED_RECENT_URL"));
    if (!stream.isEmpty()) {
        check(stream.scheme() == QStringLiteral("http") && stream.host() == QStringLiteral("127.0.0.1"), "Recent stream probe is confined to its owned loopback server");
        context->getMediaLibrary()->addAndPlay(stream);
        wait([&] { return player()->getUrl() == stream && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; }, "genuine owned HTTP media plays through native controller", 15000);
        auto *native = context->getIntf()->p_player;
        wait([&] { vlc_player_Lock(native); const auto *stats = vlc_player_GetStatistics(native); const bool decoded = stats && stats->i_decoded_audio > 0; vlc_player_Unlock(native); return decoded; }, "owned HTTP stream produces actual decoded audio", 15000);
        vlc_player_Lock(native); vlc_player_Stop(native); vlc_player_Unlock(native);
        wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_STOPPED; }, "owned stream stops normally before history expectations");
        home(); sidebarRoute(QStringLiteral("recent"));
        wait([&] {
            auto *model = recentMediaModel();
            if (!model) return false;
            for (int row = 0; row < model->rowCount(); ++row)
                if (model->data(model->index(row, 0), MLMediaModel::MEDIA_MRL).toUrl() == stream) return true;
            return false;
        }, "Recent contains the real decoded HTTP URL from native history", 15000);
        auto *streamCard = revealRecentMedia(stream);
        check(streamCard->property("subtitle").toString().contains(QStringLiteral("127.0.0.1")), "Recent stream row displays its password-free source location");
        click(streamCard);
        wait([&] { return player()->getUrl() == stream && player()->getPlayingState() == PlayerController::PLAYING_STATE_PLAYING; }, "actual Recent stream card reopens its exact HTTP address", 15000);
        report.insert("recentDecodedHttpReopen", true);
        capture(QStringLiteral("native-recent-stream-player.png"));
    }
    home();
}
void exerciseHistoryOnly(const QString &mode)
{
    check(!context->hasMediaLibrary() && !context->getMediaLibrary(), "Library OFF keeps the normal Qt browsing frontend absent");
    check(!vlc_ml_instance_get(context->getIntf()) && !var_InheritBool(context->getIntf(), "media-library"), "history access leaves the native Library instance and saved option OFF");
    auto *service = context->getPlaybackHistory();
    check(service && service->available() && !service->source() && !service->busy(), "startup capability is available without opening a history frontend");
    const auto database = QDir(qEnvironmentVariable("VLC_USERDATA_PATH")).filePath(QStringLiteral("ml/ml.db"));
    check(QFileInfo(database).absoluteFilePath().startsWith(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).absolutePath() + QLatin1Char('/')), "history probes are fenced to their owned E profile");
    if (mode == QStringLiteral("history-off-shutdown")) {
        check(!QFileInfo::exists(database), "shutdown probe starts without a database");
        {
            HistoryMediaSource opening(context->getIntf(), context->getIntf()->p_mainPlaylistController, nullptr);
            opening.open();
            check(opening.busy() && !opening.source(), "actual async opening is pending before immediate owner destruction");
        }
        QTest::qWait(100);
        check(!service->source() && !service->busy(), "discarded opening completion never updates the app history frontend");
        report.insert("historyOpeningShutdownAccepted", true);
        return;
    }
    const bool optout = mode == QStringLiteral("history-off-optout");
    HANDLE locked = INVALID_HANDLE_VALUE;
    if (optout) {
        check(!QFileInfo::exists(database), "opted-out startup and sidebar availability create no database");
        check(!var_InheritBool(context->getIntf(), "save-recentplay") && !var_InheritBool(context->getIntf(), "remember-playback-pos"), "both automatic history and position saving are explicitly disabled");
    } else {
        check(QFileInfo::exists(database), "locked retry uses a real previously populated owned database");
        locked = CreateFileW(reinterpret_cast<LPCWSTR>(database.utf16()), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        check(locked != INVALID_HANDLE_VALUE, "sidebar capability leaves the existing history database unopened");
    }
    home(); sidebarRoute(QStringLiteral("recent"));
    const QStringList mediaPath{QStringLiteral("recent"), QStringLiteral("media")};
    wait([&] { return history()->viewPath() == mediaPath && named(QStringLiteral("juxtopposedRecentPage")); }, "Library-OFF Recent opens its genuine media leaf");
    if (!optout) {
        const bool failed = QTest::qWaitFor([&] { return !service->busy() && !service->error().isEmpty(); }, 15000);
        CloseHandle(locked);
        check(failed && !service->source(), "real locked database reports history-open failure without enabling browsing");
        auto *view = named(QStringLiteral("juxtopposedRecentMediaView"));
        check(view && view->property("errorText").toString() == service->error(), "native open failure is visible in the actual Recent surface");
        QQuickItem *retry = nullptr;
        for (auto *object : objects())
            if (auto *item = qobject_cast<QQuickItem *>(object); item && item->isVisible() && item->property("text").toString() == QStringLiteral("Retry")) retry = item;
        capture(QStringLiteral("native-recent-history-locked.png"));
        click(retry);
    }
    wait([&] { return service->source() && !service->busy(); }, "explicit Recent open or actual Retry obtains the shared native history backend", 15000);
    auto *source = service->source();
    check(service->error().isEmpty() && source->historyOnly() && source->idle() && !source->homeActivity(), "history frontend has no Home activity or background indexing state");
    check(!context->getMediaLibrary() && !vlc_ml_instance_get(context->getIntf()), "opening Recent never creates a browsing frontend");
    check(!named(QStringLiteral("juxtopposedRecentPlaylistsTab"))->isEnabled(), "playlist browsing stays disabled while Library is OFF");
    wait([] { auto *model = recentMediaModel(); if (!model) return false; model->rowCount(); return !model->property("loading").toBool(); }, "history-only model finishes its real initial read", 15000);
    auto *native = context->getIntf()->p_player;
    auto stop = [&] {
        vlc_player_Lock(native); vlc_player_Stop(native); vlc_player_Unlock(native);
        wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_STOPPED; }, "history-only primary stops normally");
    };
    auto decoded = [&] {
        wait([&] { vlc_player_Lock(native); const auto *stats = vlc_player_GetStatistics(native); const bool ok = stats && stats->i_decoded_audio > 0; vlc_player_Unlock(native); return ok; }, "history-only playback produces genuine decoded audio", 15000);
    };
    const auto local = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    source->addAndPlay(local); decoded(); stop();
    if (optout) {
        QTest::qWait(500);
        check(recentMediaModel()->rowCount() == 0, "explicitly opened Recent remains empty after opted-out decoded playback");
        report.insert("historyOptoutAccepted", true);
        return;
    }
    wait([&] { return recentRow(local) >= 0; }, "native local playback updates the Library-OFF Recent model", 15000);
    click(revealRecentMedia(local)); decoded();
    vlc_player_Lock(native); vlc_player_Pause(native); vlc_player_Unlock(native);
    wait([] { return player()->getPlayingState() == PlayerController::PLAYING_STATE_PAUSED; }, "history-only reopened local media pauses normally");
    auto input = player()->getInput(); const auto pausedTime = nativeTime(); const auto position = player()->getPosition();
    home(); sidebarRoute(QStringLiteral("recent"));
    wait([&] { return history()->viewPath() == mediaPath && recentMediaModel(); }, "history-only Recent returns through the real sidebar");
    check(player()->getInput() == input && nativeTime() == pausedTime && player()->getPosition() == position, "history-only navigation preserves the paused primary input and clocks");
    const QUrl stream(qEnvironmentVariable("VLC_OWNED_RECENT_URL"));
    check(stream.scheme() == QStringLiteral("http") && stream.host() == QStringLiteral("127.0.0.1"), "history-only stream is confined to the owned loopback server");
    source->addAndPlay(stream); decoded(); stop();
    home(); sidebarRoute(QStringLiteral("recent"));
    click(revealRecentMedia(stream)); decoded();
    check(player()->getUrl() == stream, "actual Library-OFF Recent card reopens its exact saved HTTP source");
    home(); sidebarRoute(QStringLiteral("recent"));
    auto *privacy = context->getPrivacyPreferences();
    check(privacy->clearHistory(), "coordinated history clear remains available with the history-only frontend");
    wait([&] { return !privacy->busy(); }, "history-only coordinated clear finishes", 15000);
    check(privacy->lastError().isEmpty(), "history-only coordinated clear reports success");
    wait([] { auto *model = recentMediaModel(); return model && model->rowCount() == 0 && !model->property("loading").toBool(); }, "history-only Recent clears live through native events", 15000);
    stop(); QTest::qWait(500);
    check(recentMediaModel()->rowCount() == 0, "stopping the cleared current input cannot resurrect Recent entries");
    check(!context->hasMediaLibrary() && !vlc_ml_instance_get(context->getIntf()), "complete history-only journey leaves Library browsing OFF");
    capture(QStringLiteral("native-recent-history-cleared.png"));
    report.insert("recentLibraryOffAccepted", true);
    report.insert("recentDecodedHttpReopen", true);
}
void verifySidebarIcons()
{
    QFile inventory(qEnvironmentVariable("VLC_OWNED_SIDEBAR_ASSETS"));
    check(inventory.open(QIODevice::ReadOnly), "approved original sidebar-icon inventory opens");
    const auto expected=QJsonDocument::fromJson(inventory.readAll()).object();
    auto *navigation=named(QStringLiteral("juxtopposedNavigation"));check(navigation,"native sidebar exists for original-icon geometry");
    const auto scale=navigation->property("uiScale").toDouble();
    for(const auto &key:{QStringLiteral("video"),QStringLiteral("music")}) {
        const auto info=expected.value(key).toObject();
        wait([&]{auto *art=named(QStringLiteral("juxtopposedNavigationArt")+key);return art&&art->property("status").toInt()==1;},"approved Videos/Music original SVG loads in the actual native route");
        auto *art=named(QStringLiteral("juxtopposedNavigationArt")+key);
        check(art->width()==24&&art->height()==24&&qAbs(art->scale()-scale)<0.01&&!art->property("mirror").toBool(),"original sidebar icon retains intrinsic24px geometry scale and orientation");
        check(qAbs(art->mapToItem(art->parentItem(),art->boundingRect().center()).x()-art->parentItem()->width()/2)<0.1,"original sidebar artwork is centered in its native design slot");
        QFile embedded(QStringLiteral(":/juxtopposed/")+info.value(QStringLiteral("file")).toString());
        check(embedded.open(QIODevice::ReadOnly),"approved Videos/Music SVG is embedded in the loaded native resource");
        const auto bytes=embedded.readAll();
        check(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()==info.value(QStringLiteral("sha256")).toString().toLatin1(),"embedded sidebar artwork bytes match the exact approved Figma export");
        QXmlStreamReader xml(bytes);check(xml.readNextStartElement()&&xml.attributes().value(QStringLiteral("width")).toInt()==24&&xml.attributes().value(QStringLiteral("height")).toInt()==24,"original sidebar SVG root dimensions stay unchanged");
    }
    report.insert("approvedSidebarIconsAccepted",true);
}
void exercisePixmapCache()
{
    auto *engine=qmlEngine(named(QStringLiteral("juxtopposedDialogsHost")));check(engine,"actual GUI QML engine exists for cache lifecycle regression");
    auto *cache=QQuickPixmapCache::instance();auto *dispatcher=QAbstractEventDispatcher::instance();
    check(cache->thread()==QThread::currentThread()&&dispatcher,"actual image cache belongs to the GUI event thread");
    QImage image(8,8,QImage::Format_ARGB32);image.fill(QColor(80,120,160));
    const auto file=QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath(QStringLiteral("owned-cache.png"));
    check(image.save(file),"tiny owned cache fixture is saved in the private output directory");
    const auto url=QUrl::fromLocalFile(file);
    const auto samePixels=[&](const QQuickPixmap &pixmap) {
        return pixmap.image().convertToFormat(QImage::Format_ARGB32)==image;
    };
    QQuickPixmap::purgeCache();
    for(int cycle=0;cycle<2;++cycle) {
        QQuickPixmap pixmap(engine,url,QQuickPixmap::Cache);
        wait([&]{return pixmap.isReady()||pixmap.isError();},"real Qt image request finishes");
        const auto decoded=pixmap.image();
        report.insert("cacheDecodedImage",QJsonObject{{"status",static_cast<int>(pixmap.status())},{"error",pixmap.error()},
            {"format",static_cast<int>(decoded.format())},{"width",decoded.width()},{"height",decoded.height()},
            {"pixelsMatch",samePixels(pixmap)}});
        check(pixmap.isReady()&&samePixels(pixmap),"real Qt image cache loads the owned pixels");
        pixmap.clear();
        check(!dispatcher->registeredTimers(cache).isEmpty(),"releasing actual cached pixels starts its real expiry timer");
        QQuickPixmap::purgeCache();
        check(dispatcher->registeredTimers(cache).isEmpty(),"purging unused pixels stops the real cache expiry timer");
    }
    QQuickPixmap held(engine,url,QQuickPixmap::Cache);check(held.isReady(),"real referenced image exists before cache purge");
    QQuickPixmap::purgeCache();check(held.isReady()&&samePixels(held),"purging unused cache preserves an image still used by the GUI");
    held.clear();check(!dispatcher->registeredTimers(cache).isEmpty(),"releasing a retained image restarts normal cache expiration");
    QQuickPixmap::purgeCache();check(dispatcher->registeredTimers(cache).isEmpty(),"final native cache purge leaves no expiry timer registered");
    QQuickPixmap pending(engine,url,QQuickPixmap::Cache);
    check(pending.isReady(),"real cached image is ready for the application shutdown regression");
    pending.clear();
    check(!dispatcher->registeredTimers(cache).isEmpty(),"real unused-image expiry timer remains armed for application cleanup");
    report.insert("nativePixmapCacheShutdownTimerArmed",true);
    report.insert("nativePixmapCacheLifecycleAccepted",true);
}
void verifyTitleStrip(const QString &name)
{
    auto *strip = named(QStringLiteral("juxtopposedTitlebar"));
    check(strip && strip->isVisible(), "actual original shared title strip is visible");
    const auto scale = strip->property("uiScale").toDouble();
    check(qAbs(strip->height() - 40 * scale) < 0.1, "native shared strip has original40px scaled height");
    auto *logo = named(QStringLiteral("juxtopposedStriplogo"));
    auto *more = named(QStringLiteral("juxtopposedStripmenu"));
    check(logo && more, "original logo and more affordances exist in the real strip");
    check(qAbs(logo->mapToItem(strip, QPointF()).x() - 6 * scale) < 0.1
        && qAbs(more->mapToItem(strip, QPointF()).x() - 51 * scale) < 0.1,
        "native logo and menu match original6/51px slot positions");
    const bool playerContext = strip->property("playerContext").toBool();
    const QStringList baseKeys{QStringLiteral("logo"),QStringLiteral("menu"),QStringLiteral("minimize"),QStringLiteral("maximize"),QStringLiteral("fullscreen"),QStringLiteral("close")};
    QStringList keys = baseKeys;
    if (playerContext) keys.append({QStringLiteral("navigation"),QStringLiteral("separator"),QStringLiteral("sourceFolder")});
    for (const auto &key : keys) {
        wait([&] { auto *art = named(QStringLiteral("juxtopposedStripArt") + key); return art && art->property("status").toInt() == 1; }, "actual original title-strip SVG loads through native resources/provider");
        auto *art = named(QStringLiteral("juxtopposedStripArt") + key);
        const int intrinsic = key == QStringLiteral("menu") ? 18 : 24;
        check(art->width() == intrinsic && art->height() == intrinsic && qAbs(art->scale() - scale) < 0.01,
              "native SVG keeps original intrinsic18/24 and scaled wrapper dimensions");
        QFile embedded(QStringLiteral(":/juxtopposed/titlebar-") + key + QStringLiteral(".svg"));
        check(embedded.open(QIODevice::ReadOnly), "original title SVG exists in the actual loaded qrc");
        const auto bytes = embedded.readAll();
        QFile inventory(qEnvironmentVariable("VLC_OWNED_TITLEBAR_ASSETS")); check(inventory.open(QIODevice::ReadOnly), "owned original title-art inventory opens");
        const auto expected = QJsonDocument::fromJson(inventory.readAll()).object().value(key).toObject();
        check(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex() == expected.value(QStringLiteral("sha256")).toString().toLatin1(), "embedded title artwork bytes match the exact original export");
        QXmlStreamReader xml(bytes); check(xml.readNextStartElement() && xml.attributes().value(QStringLiteral("width")).toInt() == intrinsic && xml.attributes().value(QStringLiteral("height")).toInt() == intrinsic,
              "embedded original SVG root dimensions are unchanged");
    }
    auto *title = named(QStringLiteral("juxtopposedStripTitle"));
    check(title && qAbs(title->mapToItem(strip,title->boundingRect().center()).x() - strip->width()/2) <= 0.51,
          "actual native title stays centered within half a pixel");
    for (auto *button : context->csdButtonModel()->windowCSDButtons()) {
        QString key; int expectedHit = HTCLIENT;
        switch (button->type()) {
        case CSDButton::Minimize:key=QStringLiteral("minimize");expectedHit=HTMINBUTTON;break;
        case CSDButton::MaximizeRestore:key=QStringLiteral("maximize");expectedHit=HTMAXBUTTON;break;
        case CSDButton::Close:key=QStringLiteral("close");expectedHit=HTCLOSE;break;
        default:continue;
        }
        auto *item = named(QStringLiteral("juxtopposedStrip")+key); check(item,"actual native CSD button exists");
        const auto point=item->mapToScene(item->boundingRect().center());
        check(button->rect().contains(point.toPoint()), "CSD model rectangle covers the actual redesigned button");
        POINT screen{static_cast<LONG>(point.x()*window->devicePixelRatio()),static_cast<LONG>(point.y()*window->devicePixelRatio())};
        const auto hwnd=reinterpret_cast<HWND>(window->winId()); ClientToScreen(hwnd,&screen);
        check(SendMessageW(hwnd,WM_NCHITTEST,0,MAKELPARAM(screen.x,screen.y))==expectedHit,"owned native window hit-test retains correct minimize/maximize/close semantics");
    }
    capture(name);
}
class OwnedMoveObserver : public QAbstractNativeEventFilter
{
public:
    HWND owned=nullptr;
    int requested=0,entered=0,moving=0,exited=0;
    bool nativeEventFilter(const QByteArray &,void *message,qintptr *) override
    {
        const auto *event=static_cast<MSG *>(message);
        if(event->hwnd!=owned)return false;
        if(event->message==WM_SYSCOMMAND&&event->wParam==0xF012)++requested;
        if(event->message==WM_ENTERSIZEMOVE)++entered;
        if(event->message==WM_MOVING)++moving;
        if(event->message==WM_EXITSIZEMOVE)++exited;
        return false;
    }
};
void exerciseNativeTitleDrag(const std::function<bool()> &unchanged)
{
    window->setPosition(window->screen()->availableGeometry().topLeft()+QPoint(40,40));window->requestActivate();QTest::qWait(300);
    const auto hwnd=reinterpret_cast<HWND>(window->winId());
    DWORD processId=0;GetWindowThreadProcessId(hwnd,&processId);
    check(processId==GetCurrentProcessId(),"native drag target is owned by this exact test process");
    wait([&]{return GetForegroundWindow()==hwnd;},"only the owned VLC window is foreground before bounded native drag");
    auto *strip=named(QStringLiteral("juxtopposedTitlebar"));check(strip,"actual title strip exists for native pointer drag");
    const auto local=strip->mapToScene(QPointF(strip->width()/2,strip->height()/2));
    POINT start{static_cast<LONG>(local.x()*window->devicePixelRatio()),static_cast<LONG>(local.y()*window->devicePixelRatio())};ClientToScreen(hwnd,&start);
    check(GetAncestor(WindowFromPoint(start),GA_ROOT)==hwnd,"actual native drag start point belongs to the owned title strip window");
    RECT before{};GetWindowRect(hwnd,&before);
    OwnedMoveObserver observer;observer.owned=hwnd;qApp->installNativeEventFilter(&observer);
    auto phase=std::make_shared<std::atomic<int>>(0);
    auto *worker=QThread::create([hwnd,start,phase] {
        POINT saved{};GetCursorPos(&saved);bool pressed=false;
        auto release=[&] {if(pressed){INPUT event{};event.type=INPUT_MOUSE;event.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&event,sizeof(INPUT));pressed=false;}};
        QThread::msleep(100);
        if(GetForegroundWindow()!=hwnd){*phase=1;return;}
        SetCursorPos(start.x,start.y);QThread::msleep(100);
        if(GetForegroundWindow()!=hwnd||GetAncestor(WindowFromPoint(start),GA_ROOT)!=hwnd){*phase=2;return;}
        INPUT event{};event.type=INPUT_MOUSE;event.mi.dwFlags=MOUSEEVENTF_LEFTDOWN;
        if(SendInput(1,&event,sizeof(INPUT))!=1){*phase=3;SetCursorPos(saved.x,saved.y);return;}pressed=true;
        QThread::msleep(120);
        if(GetForegroundWindow()==hwnd){SetCursorPos(start.x+40,start.y+20);QThread::msleep(150);}
        if(GetForegroundWindow()==hwnd){SetCursorPos(start.x+80,start.y+40);QThread::msleep(150);}
        const bool stillOwned=GetForegroundWindow()==hwnd;release();QThread::msleep(120);
        // Always end this owned window's modal move loop, even if the OS rejected input.
        PostMessageW(hwnd,WM_CANCELMODE,0,0);
        if(stillOwned)SetCursorPos(saved.x,saved.y);
        *phase=stillOwned?4:5;
    });
    worker->start();
    const bool finished=QTest::qWaitFor([&]{return worker->isFinished();},4000);
    if(!finished)PostMessageW(hwnd,WM_CANCELMODE,0,0);
    const bool joined=worker->wait(2000);
    qApp->removeNativeEventFilter(&observer);
    if(joined)delete worker;
    QTest::qWait(200);RECT after{};GetWindowRect(hwnd,&after);
    report.insert("nativeTitleDrag",QJsonObject{{"workerFinished",finished},{"workerJoined",joined},{"phase",phase->load()},{"systemMoveRequests",observer.requested},{"nativeMoveEntered",observer.entered},{"nativeMoveMessages",observer.moving},{"nativeMoveExited",observer.exited},{"deltaX",static_cast<int>(after.left-before.left)},{"deltaY",static_cast<int>(after.top-before.top)},{"widthBefore",static_cast<int>(before.right-before.left)},{"heightBefore",static_cast<int>(before.bottom-before.top)},{"widthAfter",static_cast<int>(after.right-after.left)},{"heightAfter",static_cast<int>(after.bottom-after.top)}});
    check(finished&&joined&&phase->load()==4,"bounded owned native drag worker ends normally with its input released");
    check(observer.requested>0&&observer.entered>0&&observer.exited>0,"actual title-strip drag enters and leaves the real Windows system-move loop");
    check(qAbs(after.left-before.left)>15||qAbs(after.top-before.top)>15,"actual native title-strip pointer drag changes the owned window position");
    check(after.right-after.left==before.right-before.left&&after.bottom-after.top==before.bottom-before.top&&window->visibility()==QWindow::Windowed,"actual system drag preserves window size and normal state");
    check(unchanged(),"actual native title-strip drag preserves primary input and paused clocks");
    report.insert("nativeTitleDragAccepted",true);capture(QStringLiteral("native-titlebar-after-system-drag.png"));
}

void exerciseCompactNavigation(const std::function<bool()> &unchanged)
{
    verifySidebarIcons();
    QJsonArray sizes;
    for (const auto &entry : {QVector3D(320,360,1),QVector3D(480,360,1),QVector3D(640,480,1.5),QVector3D(640,480,2)}) {
        context->setIntfUserScaleFactor(entry.z()); QTest::qWait(100); window->resize(qMax(static_cast<int>(entry.x()),window->minimumWidth()),qMax(static_cast<int>(entry.y()),window->minimumHeight())); QTest::qWait(250);
        auto *list=named(QStringLiteral("juxtopposedSidebarPinList")); check(list,"actual compact player drawer has its real scrolling list");
        expression(list,QStringLiteral("positionViewAtBeginning(); forceLayout()"));
        auto *home=named(QStringLiteral("juxtopposedNavigationhome"));check(home,"actual drawer Home route exists");home->forceActiveFocus(Qt::TabFocusReason);
        for (const auto &key : {QStringLiteral("home"),QStringLiteral("video"),QStringLiteral("music"),QStringLiteral("network"),QStringLiteral("playlists"),QStringLiteral("recent")}) {
            if(key!=QStringLiteral("home"))QTest::keyClick(window,Qt::Key_Down);
            QTest::qWait(50);
            auto *button=named(QStringLiteral("juxtopposedNavigation")+key);check(button&&button->hasActiveFocus(),"actual compact drawer arrow keys focus the next native route");
            const auto rect=button->mapRectToItem(list,button->boundingRect());
            report.insert("lastCompactRoute",QJsonObject{{"route",key},{"width",window->width()},{"height",window->height()},{"scale",entry.z()},{"listHeight",list->height()},{"routeTop",rect.top()},{"routeBottom",rect.bottom()},{"contentY",list->property("contentY").toDouble()}});
            check(rect.top()>=-0.51&&rect.bottom()<=list->height()+0.51,"every keyboard-focused native route stays fully inside the short drawer viewport");
            check(unchanged(),"short-window drawer keyboard navigation preserves paused native input and clocks");
            if(key==QStringLiteral("playlists")&&entry.x()==320&&entry.z()==1)capture(QStringLiteral("native-player-short-playlists-visible.png"));
        }
        QTest::keyClick(window,Qt::Key_Down);
        auto *add=named(QStringLiteral("juxtopposedSidebarAdd"));check(add&&add->hasActiveFocus(),"actual compact drawer Add item remains keyboard reachable");
        const auto rect=add->mapRectToItem(list,add->boundingRect());check(rect.top()>=-0.51&&rect.bottom()<=list->height()+0.51,"actual compact drawer Add item scrolls fully into view");
        QTest::keyClick(window,Qt::Key_Space);wait([]{return menu()!=nullptr;},"compact keyboard Add item opens the real native chooser menu");QTest::keyClick(menu(),Qt::Key_Escape);wait([]{return menu()==nullptr;},"actual compact Add menu dismisses normally");
        check(unchanged(),"compact Add menu preserves paused native input and clocks");
        check(add->hasActiveFocus(),"native compact Add menu dismissal restores its actual opener focus");
        for (const auto &key : {QStringLiteral("recent"),QStringLiteral("playlists"),QStringLiteral("network"),QStringLiteral("music"),QStringLiteral("video"),QStringLiteral("home")}) {
            QTest::keyClick(window,Qt::Key_Up);QTest::qWait(50);auto *button=named(QStringLiteral("juxtopposedNavigation")+key);
            check(button&&button->hasActiveFocus(),"actual short drawer reverse arrow traversal reaches every native route");
            const auto visible=button->mapRectToItem(list,button->boundingRect());
            check(visible.top()>=-0.51&&visible.bottom()<=list->height()+0.51,"reverse-focused native route stays fully inside the compact viewport");
            check(unchanged(),"reverse drawer navigation preserves native volume mute input and paused clocks");
        }
        const QStringList tabOrder{QStringLiteral("home"),QStringLiteral("video"),QStringLiteral("music"),QStringLiteral("network"),QStringLiteral("playlists"),QStringLiteral("recent"),QStringLiteral("add")};
        const auto routeButton=[&](const QString &key) {
            return named(key==QStringLiteral("add") ? QStringLiteral("juxtopposedSidebarAdd") : QStringLiteral("juxtopposedNavigation")+key);
        };
        for (int direction : {1,-1}) {
            for (int index=direction>0?1:tabOrder.size()-2;index>=0&&index<tabOrder.size();index+=direction) {
                QTest::keyClick(window,direction>0?Qt::Key_Tab:Qt::Key_Backtab);QTest::qWait(60);
                auto *button=routeButton(tabOrder[index]);
                report.insert("lastCompactTab",QJsonObject{{"route",tabOrder[index]},{"direction",direction},{"width",window->width()},{"height",window->height()},{"scale",entry.z()},
                    {"actualFocus",window->activeFocusItem()?window->activeFocusItem()->objectName():QString()}});
                check(button&&button->hasActiveFocus(),"native compact Tab and reverse Tab visit each route and Add in logical order");
                const auto visible=button->mapRectToItem(list,button->boundingRect());
                report.insert("lastCompactTabBounds",QJsonObject{{"top",visible.top()},{"bottom",visible.bottom()},{"viewportHeight",list->height()}});
                if(visible.top()<-0.51||visible.bottom()>list->height()+0.51)capture(QStringLiteral("native-compact-tab-clipped.png"));
                check(visible.top()>=-0.51&&visible.bottom()<=list->height()+0.51,"native Tab-focused drawer control is completely visible in the short viewport");
                check(unchanged(),"native compact Tab traversal preserves paused playback volume and mute");
            }
        }
        auto *navigation=named(QStringLiteral("juxtopposedNavigation"));check(navigation,"actual drawer remains open for pointer and wheel checks");
        const auto press=home->mapToScene(QPointF(1,home->height()/2)).toPoint();
        const auto cancel=home->mapToScene(QPointF(-2,home->height()/2)).toPoint();
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,press);
        check(home->hasActiveFocus(),"native pointer press focuses the real drawer route");
        QTest::mouseMove(window,cancel);QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,cancel);
        check(context->isPlayerView()&&named(QStringLiteral("juxtopposedNavigation")),"cancelled route press does not activate a route or close the drawer");
        check(unchanged(),"native drawer pointer focus preserves primary playback and audio state");
        if(list->property("contentHeight").toDouble()>list->height()+1) {
            for(bool padding:{false,true}) {
                expression(list,QStringLiteral("positionViewAtBeginning(); forceLayout()"));
                const auto before=list->property("contentY").toDouble();
                const auto local=padding ? navigation->mapToScene(QPointF(2*entry.z(),navigation->height()/2))
                    : list->mapToScene(QPointF(list->width()/2,list->height()/2));
                QTest::mouseMove(window,local.toPoint());
                QWheelEvent wheel(local,window->mapToGlobal(local.toPoint()),QPoint(),QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
                QCoreApplication::sendEvent(window,&wheel);QTest::qWait(250);
                const auto after=list->property("contentY").toDouble();
                report.insert("lastCompactWheel",QJsonObject{{"padding",padding},{"before",before},{"after",after},{"accepted",wheel.isAccepted()}});
                check(unchanged(),"native drawer wheel remains isolated from player volume mute and paused clocks");
                check(after>before+0.5,padding?"native wheel over drawer padding scrolls the list":"native wheel over actual drawer rows scrolls the list");
            }
            for (int variant : {0,1,2}) {
                expression(list,QStringLiteral("positionViewAtBeginning(); forceLayout()"));
                const auto local=navigation->mapToScene(QPointF(2*entry.z(),navigation->height()/2));
                if (variant==2) {
                    // With no model rows, forceLayout() resets the header/footer
                    // position. Reach the boundary through the actual input path.
                    for (int step=0;step<8;++step) {
                        QWheelEvent advance(local,window->mapToGlobal(local.toPoint()),QPoint(),QPoint(0,-120),
                            Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
                        QCoreApplication::sendEvent(window,&advance);QTest::qWait(50);
                    }
                    const auto end=qMax(list->property("originY").toDouble(),
                        list->property("originY").toDouble()+list->property("contentHeight").toDouble()-list->height());
                    report.insert("compactWheelBoundary",QJsonObject{{"actual",list->property("contentY").toDouble()},{"expected",end}});
                    check(qAbs(list->property("contentY").toDouble()-end)<0.51,
                        "native padding wheel input actually reaches the list boundary before the end check");
                }
                const auto before=list->property("contentY").toDouble();
                QWheelEvent wheel(local,window->mapToGlobal(local.toPoint()),variant==0?QPoint(0,-23):QPoint(),
                    variant==0?QPoint():variant==1?QPoint(120,0):QPoint(0,-120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
                QCoreApplication::sendEvent(window,&wheel);QTest::qWait(150);
                const auto after=list->property("contentY").toDouble();
                report.insert("lastCompactWheelVariant",QJsonObject{{"variant",variant},{"before",before},{"after",after},{"accepted",wheel.isAccepted()}});
                check(unchanged(),"native pixel horizontal and end-of-list wheels preserve primary playback volume and mute");
                check(wheel.isAccepted(),"native drawer consumes pixel horizontal and end-of-list padding wheels");
                check(variant==0?qAbs(after-before-23)<0.51:qAbs(after-before)<0.51,
                    "native padding wheel honors pixel distance and clamps horizontal or end-of-list movement");
            }
        }
        capture(QStringLiteral("native-player-drawer-%1x%2-scale%3.png").arg(window->width()).arg(window->height()).arg(entry.z()));
        sizes.append(QJsonObject{{"requestedWidth",entry.x()},{"requestedHeight",entry.y()},{"minimumWidth",window->minimumWidth()},{"minimumHeight",window->minimumHeight()},{"width",window->width()},{"height",window->height()},{"scale",entry.z()}});
    }
    report.insert("compactDrawerCases",sizes);report.insert("nativeCompactDrawerAccepted",true);
    report.insert("nativeCompactTabPointerWheelAccepted",true);
    context->setIntfUserScaleFactor(1);window->resize(1280,900);QTest::qWait(250);
}
#include "movable_dialog_journey.hpp"
#include "navigation_recovery_journey.hpp"
#include "dialog_busy_journey.hpp"
#include "setup_journey.hpp"
#include "profiles_journey.hpp"
#include "transport_defaults_journey.hpp"
#include "customizer_journey.hpp"
#include "favorite_journey.hpp"
#include "playlist_origin_journey.hpp"
#include "fullscreen_controls_journey.hpp"
#include "settings_activation_journey.hpp"
#include "tooltip_edge_journey.hpp"
#include "audio_device_settings_journey.hpp"
#include "interface_title_journey.hpp"
#include "geometry_memory_journey.hpp"
#include "geometry_settings_journey.hpp"
#include "auto_hide_journey.hpp"
#include "fullscreen_screen_journey.hpp"

void exerciseTitlebar(const QString &mode)
{
    if(mode==QStringLiteral("titlebar-window")){verifySidebarIcons();exercisePixmapCache();}
    if (mode == QStringLiteral("titlebar-traditional")) {
        history()->push({QStringLiteral("home")},{},Qt::OtherFocusReason);
        wait([] { for(auto *object:objects()) if(auto *item=qobject_cast<QQuickItem *>(object);item&&item->isVisible()&&item->property("description").toString()==QStringLiteral("Show navigation menu")) return true; return false; },"actual traditional Home and its native navigation control finish loading");
        check(!named(QStringLiteral("juxtopposedTitlebar")), "traditional-controls opt-out keeps the original native titlebar path");
        check(!var_InheritBool(context->getIntf(),"qt-juxtopposed-controls"),"traditional startup option remains disabled");
        report.insert("nativeTraditionalHeaderAccepted",true);
        capture(QStringLiteral("native-titlebar-traditional.png")); return;
    }
    home();
    const bool media = mode == QStringLiteral("titlebar-media") || mode == QStringLiteral("titlebar-media-title") || mode == QStringLiteral("titlebar-video") || mode == QStringLiteral("titlebar-compact") || mode == QStringLiteral("titlebar-compact-audio") || mode == QStringLiteral("titlebar-dialogs") || mode == QStringLiteral("titlebar-recovery") || mode.startsWith(QStringLiteral("titlebar-busy"));
    const bool video = mode == QStringLiteral("titlebar-media-title") || mode == QStringLiteral("titlebar-video") || mode == QStringLiteral("titlebar-compact");
    auto *native = context->getIntf()->p_player;
    if (media) {
        const auto url = QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(video ? QStringLiteral("owned-clip.mp4") : QStringLiteral("first.flac")));
        if (video) history()->push({QStringLiteral("video"),QStringLiteral("all")},{},Qt::OtherFocusReason);
        context->getMediaLibrary()->addAndPlay(url);
        wait([&] { vlc_player_Lock(native); const auto *stats=vlc_player_GetStatistics(native); const bool ok=stats&&(video ? stats->i_decoded_video>0 : stats->i_decoded_audio>0); vlc_player_Unlock(native); return ok; }, "title-strip journey begins with actual decoded owned media",15000);
        vlc_player_Lock(native); vlc_player_Pause(native); vlc_player_Unlock(native);
        wait([] {return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"actual primary pauses before window and navigation actions");
        context->setPlayerView(true);
        wait([] {auto *strip=named(QStringLiteral("juxtopposedTitlebar"));return strip&&strip->property("playerContext").toBool();},"actual player surface uses the shared redesigned strip");
        QTest::qWait(250);
    }
    auto *input=player()->getInput();const auto time=nativeTime();const auto position=player()->getPosition();
    const auto volume=vlc_player_aout_GetVolume(native);const auto mute=vlc_player_aout_IsMuted(native);
    report.insert("initialNativeAudioState",QJsonObject{{"volume",volume},{"mute",mute},{"qtVolume",player()->getVolume()}});
    if(mode==QStringLiteral("titlebar-compact-audio"))check(volume>=0&&mute>=0,"decoded-audio compact case owns an actual audio output for volume and mute checks");
    auto unchanged=[&] {
        const auto currentVolume=vlc_player_aout_GetVolume(native);const auto currentMute=vlc_player_aout_IsMuted(native);
        report.insert("lastNativeAudioState",QJsonObject{{"volume",currentVolume},{"mute",currentMute},{"qtVolume",player()->getVolume()}});
        return player()->getInput()==input&&(!media||(player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED&&nativeTime()==time&&player()->getPosition()==position))&&qAbs(currentVolume-volume)<0.0001&&currentMute==mute;
    };
    for (double scale : {1.0,1.5,2.0}) {
        context->setIntfUserScaleFactor(scale); QTest::qWait(250);
        verifyTitleStrip(QStringLiteral("native-titlebar-%1-%2.png").arg(media?QStringLiteral("player"):QStringLiteral("home")).arg(scale));
        check(unchanged(),"native interface scaling preserves primary input and exact paused clocks");
    }
    context->setIntfUserScaleFactor(1);QTest::qWait(200);
    if (mode == QStringLiteral("titlebar-dialogs")) {
        exerciseMovableDialogs(unchanged);
        return;
    }
    if (mode == QStringLiteral("titlebar-recovery")) {
        exerciseNavigationRecovery(unchanged);
        return;
    }
    if (mode == QStringLiteral("titlebar-home-menu")) {
        exerciseHomePinMenu(unchanged);
        return;
    }
    if (mode == QStringLiteral("titlebar-busy")) {
        exerciseBusyDialog(unchanged);
        return;
    }
    if (mode == QStringLiteral("titlebar-busy-playing")) {
        vlc_player_Lock(native);vlc_player_Resume(native);vlc_player_Unlock(native);
        wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},
             "real primary resumes before the playing busy-dialog journey");
        const auto start=nativeTime();
        wait([&]{return nativeTime()>start+VLC_TICK_FROM_MS(200);},"real primary clock advances before the independent conversion");
        auto playingUnchanged=[&]{return player()->getInput()==input
            && player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING && nativeTime()>=start
            && qAbs(vlc_player_aout_GetVolume(native)-volume)<.0001 && vlc_player_aout_IsMuted(native)==mute;};
        exerciseBusyDialog(playingUnchanged);
        check(nativeTime()>start+VLC_TICK_FROM_MS(500),"real primary continues advancing through the busy-modal job and cancellation");
        report.insert("nativeBusyPlayingAccepted",true);
        vlc_player_Lock(native);vlc_player_Pause(native);vlc_player_Unlock(native);
        wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"owned primary pauses normally after playing independence verification");
        return;
    }
    auto *logo = named(QStringLiteral("juxtopposedStriplogo"));
    check(logo != nullptr, "real title-strip logo is a keyboard-focusable control");
    logo->forceActiveFocus(Qt::TabFocusReason); QTest::keyClick(window,Qt::Key_Tab);
    check(named(QStringLiteral("juxtopposedStripmenu"))->hasActiveFocus(), "native Tab moves from original logo to original menu button");
    QTest::keyClick(window,Qt::Key_Space);
    wait([] {return menu()!=nullptr;},"actual focused title-strip menu opens through the keyboard");
    QTest::keyClick(menu(),Qt::Key_Escape);
    wait([] {return menu()==nullptr;},"actual Escape dismisses the native global menu");
    check(unchanged(),"title-strip keyboard activation leaves paused primary input and clocks unchanged");
    if (media) {
        const auto displayed=named(QStringLiteral("juxtopposedStripTitle"))->property("text").toString();
        report.insert("titlePolicy",QJsonObject{{"startupEnabled",context->showMediaNameInTitle()},{"nativeName",player()->getName()},{"metadataTitle",player()->getTitle()},{"visibleStrip",displayed},{"nativeWindowTitle",window->title()}});
        check(mode==QStringLiteral("titlebar-media-title")?(!player()->getName().isEmpty() && displayed==player()->getName() && window->title()==player()->getName()):displayed==QStringLiteral("VLC Media Player"),"startup media-name preference is honored by the redesigned strip");
        click(named(QStringLiteral("juxtopposedStripnavigation")));
        wait([] {return named(QStringLiteral("juxtopposedNavigation"))!=nullptr;},"actual player navigation affordance opens the shared typed sidebar");
        check(unchanged(),"opening player navigation preserves paused decoded primary playback");
        capture(QStringLiteral("native-player-navigation-open.png"));
        if(mode==QStringLiteral("titlebar-compact")||mode==QStringLiteral("titlebar-compact-audio")) exerciseCompactNavigation(unchanged);
        sidebarRoute(QStringLiteral("recent"));
        wait([] {return !context->isPlayerView()&&history()->viewPath()==QStringList{QStringLiteral("recent"),QStringLiteral("media")};},"actual player sidebar route opens native Recent without restarting media");
        check(unchanged(),"player-to-library navigation preserves input and exact paused clocks");
        context->setPlayerView(true);
        wait([] {auto *strip=named(QStringLiteral("juxtopposedTitlebar"));return strip&&strip->property("playerContext").toBool();},"real player re-entry restores shared title strip");
        click(named(QStringLiteral("juxtopposedStripsourceFolder")));
        wait([] {return !context->isPlayerView()&&history()->viewPath()==QStringList{QStringLiteral("recent"),QStringLiteral("media")};},"actual original source affordance returns to the native source leaf");
        check(unchanged(),"original source return preserves paused decoded input and clocks");
        context->setPlayerView(true);
        wait([] {auto *strip=named(QStringLiteral("juxtopposedTitlebar"));return strip&&strip->property("playerContext").toBool();},"player returns after actual source-button navigation");
        click(named(QStringLiteral("juxtopposedStriplogo")));
        wait([] {return !context->isPlayerView()&&history()->viewPath().first()==QStringLiteral("home");},"actual original VLC logo opens native Home");
        check(unchanged(),"original Home action preserves paused decoded input and clocks");
        context->setPlayerView(true);
        wait([] {auto *strip=named(QStringLiteral("juxtopposedTitlebar"));return strip&&strip->property("playerContext").toBool();},"real player restores after original Home action");
    }
    if(mode==QStringLiteral("titlebar-window")||mode==QStringLiteral("titlebar-compact")||mode==QStringLiteral("titlebar-compact-audio")) {
        for(int cycle=0;cycle<2;++cycle) {
            auto *strip=named(QStringLiteral("juxtopposedTitlebar"));check(strip,"owned shared title strip exists for actual double-click");
            const auto point=strip->mapToScene(QPointF(strip->width()/2,strip->height()/2)).toPoint();
            QTest::mouseDClick(window,Qt::LeftButton,Qt::NoModifier,point);QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,QPoint(window->width()/2,point.y()));
            wait([&]{return window->visibility()==(cycle==0?QWindow::Maximized:QWindow::Windowed);},"actual original title-strip double-click toggles native maximize and restore");
        }
        check(unchanged(),"native title-strip double-click preserves primary input and clocks");report.insert("nativeTitleDoubleClickAccepted",true);
        exerciseNativeTitleDrag(unchanged);
    }
    click(named(QStringLiteral("juxtopposedStripmaximize")));
    wait([] {return window->visibility()==QWindow::Maximized;},"actual original maximize affordance maximizes the owned native window");
    click(named(QStringLiteral("juxtopposedStripmaximize")));
    wait([] {return window->visibility()==QWindow::Windowed;},"actual original maximize affordance restores the owned native window");
    check(unchanged(),"maximize/restore preserves primary input and paused clocks");
    click(named(QStringLiteral("juxtopposedStripfullscreen")));
    wait([] {return window->visibility()==QWindow::FullScreen;},"actual original fullscreen affordance enters native fullscreen");
    QTest::mouseMove(window,QPoint(window->width()-20,15));QTest::qWait(300);
    click(named(QStringLiteral("juxtopposedStripfullscreen")));
    wait([] {return window->visibility()!=QWindow::FullScreen;},"actual original fullscreen affordance exits native fullscreen");
    check(unchanged(),"fullscreen round trip preserves primary input and paused clocks");
    click(named(QStringLiteral("juxtopposedStripminimize")));
    wait([] {return window->visibility()==QWindow::Minimized;},"actual original minimize affordance minimizes the owned native window");
    context->requestInterfaceNormal();window->requestActivate();
    wait([] {return window->visibility()==QWindow::Windowed;},"owned native window restores normally after minimize");
    check(unchanged(),"minimize/restore preserves primary input and paused clocks");
    if (mode == QStringLiteral("titlebar-media-title")) {
        auto *retainedStrip = named(QStringLiteral("juxtopposedTitlebar"));
        QJsonArray transitions;
        for (const auto &file : {QStringLiteral("first.flac"),QStringLiteral("owned-clip.mp4")}) {
            const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(file));
            const bool videoFile=file.endsWith(QStringLiteral(".mp4"));
            context->getMediaLibrary()->addAndPlay(url);
            wait([&] { vlc_player_Lock(native); const auto *stats=vlc_player_GetStatistics(native); const bool decoded=stats&&(videoFile?stats->i_decoded_video>0:stats->i_decoded_audio>0); vlc_player_Unlock(native); return player()->getUrl()==url&&decoded; },"actual new source decodes while the same title strip remains mounted",15000);
            const bool titleMatched=QTest::qWaitFor([&] { auto *label=named(QStringLiteral("juxtopposedStripTitle"));return label&&!player()->getName().isEmpty()&&label->property("text").toString()==player()->getName(); },5000);
            report.insert("lastTitleTransition",QJsonObject{{"file",file},{"nativeName",player()->getName()},{"metadataTitle",player()->getTitle()},{"visibleStrip",named(QStringLiteral("juxtopposedStripTitle"))?named(QStringLiteral("juxtopposedStripTitle"))->property("text").toString():QString()},{"nativeWindowTitle",window->title()},{"playerView",context->isPlayerView()},{"sameStrip",named(QStringLiteral("juxtopposedTitlebar"))==retainedStrip}});
            check(titleMatched,"mounted native title follows the actual source name after media switch");
            check(named(QStringLiteral("juxtopposedTitlebar"))==retainedStrip,"source changes update the retained title strip without recreating it");
            transitions.append(QJsonObject{{"file",file},{"nativeName",player()->getName()},{"metadataTitle",player()->getTitle()},{"visibleStrip",named(QStringLiteral("juxtopposedStripTitle"))->property("text").toString()},{"nativeWindowTitle",window->title()}});
        }
        report.insert("nativeTitleSourceTransitions",transitions);
        report.insert("nativeTitleTransitionsAccepted",true);
    }
    report.insert("nativeTitlebarAccepted",true);
    report.insert("interfaceScales",QJsonArray{1.0,1.5,2.0});
    if (mode==QStringLiteral("titlebar-close")) {
        for(auto *button:context->csdButtonModel()->windowCSDButtons()) if(button->type()==CSDButton::Close)
            QObject::connect(button,&CSDButton::clicked,qApp,[] {
                report.insert("nativeCloseTriggered",true);
                QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT"));if(output.open(QIODevice::WriteOnly))output.write(QJsonDocument(report).toJson());
            });
    }
}
void run()
{
    try {
        check(context && window, "owned native window and MainCtx became ready");
        const auto mode = qEnvironmentVariable("VLC_OWNED_UI_MODE");
        if(mode.startsWith(QStringLiteral("geometry-editor-"))) {
            const auto r=window->geometry();
            report.insert("startupGeometry",QJsonObject{{"client",QJsonArray{r.x(),r.y(),r.width(),r.height()}},
                {"screen",window->screen()?window->screen()->name():QString{}},{"states",int(window->windowStates())}});
        }
        const auto screenName=qEnvironmentVariable("VLC_OWNED_UI_SCREEN");
        if(!screenName.isEmpty()) {
            QScreen *target=nullptr;
            for(auto *screen:QGuiApplication::screens())if(screen->name()==screenName)target=screen;
            check(target,"requested native test screen exists");
            window->setScreen(target);
            window->setPosition(target->availableGeometry().topLeft()+QPoint(40,40));
            wait([&]{return window->screen()==target;},"owned native window is on the requested test screen");
            report.insert("screenName",target->name());
        }
        window->resize(1280, 900); QTest::qWait(100);
        report.insert("mode",mode); check(pins() != nullptr, "real MainCtx owns pin service");
        if (mode.startsWith(QStringLiteral("geometry-editor-"))) {
            exerciseGeometrySettings(mode);
        } else if (mode.startsWith(QStringLiteral("geometry-snapshot-"))) {
            exerciseGeometrySnapshot(mode);
        } else if (mode.startsWith(QStringLiteral("fullscreen-screen-"))) {
            exerciseFullscreenScreen(mode);
        } else if (mode.startsWith(QStringLiteral("auto-hide-"))) {
            exerciseAutoHide(mode);
        } else if (mode.startsWith(QStringLiteral("interface-title-"))) {
            exerciseInterfaceTitle(mode);
        } else if (mode.startsWith(QStringLiteral("audio-device-"))) {
            exerciseAudioDeviceSettings(mode);
        } else if (mode == QStringLiteral("tooltip-edge")) {
            exerciseTooltipEdge();
        } else if (mode.startsWith(QStringLiteral("settings-activation-"))) {
            exerciseSettingsActivation(mode);
        } else if (mode.startsWith(QStringLiteral("fullscreen-controls-"))) {
            exerciseFullscreenControls(mode);
        } else if (mode.startsWith(QStringLiteral("playlist-origin"))) {
            exercisePlaylistOrigin(mode);
        } else if (mode.startsWith(QStringLiteral("favorite-"))) {
            exerciseFavorite(mode);
        } else if (mode.startsWith(QStringLiteral("customizer-"))) {
            exerciseCustomizer(mode);
        } else if (mode.startsWith(QStringLiteral("transport-"))) {
            exerciseTransportDefaults(mode);
        } else if (mode.startsWith(QStringLiteral("profiles-"))) {
            exerciseProfiles(mode);
        } else if (mode.startsWith(QStringLiteral("setup-"))) {
            exerciseSetup(mode);
        } else if (mode.startsWith(QStringLiteral("titlebar-"))) {
            exerciseTitlebar(mode);
        } else if (mode.startsWith(QStringLiteral("history-off"))) {
            exerciseHistoryOnly(mode);
        } else if (mode == QStringLiteral("populate")) {
            check(pins()->count() == 0 && pins()->error().isEmpty(), "fresh native profile starts with no manual pins");
            addMenu(QStringLiteral("Folder…")); filePicker(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER"));
            wait([] { return pins()->count() == 1 && !pins()->busy(); }, "actual Add folder chooser saves first pin");
            wait([] { auto *art=named(QStringLiteral("juxtopposedSidebarPinArt0"));return art && art->property("status").toInt()==1; }, "original folder SVG loads in actual native pin row");
            seedNativeCollections();
            const auto before = snapshot();
            addMenu(QStringLiteral("Files…")); filePicker(QString(), true);
            check(snapshot() == before, "actual file chooser cancellation preserves all pins");
            addMenu(QStringLiteral("Source address…"));
            QPointer<QInputDialog> sourceDialog;
            wait([&] { for (auto *widget : QApplication::topLevelWidgets()) if (auto *dialog=qobject_cast<QInputDialog *>(widget);dialog&&dialog->isVisible()) sourceDialog=dialog; return !sourceDialog.isNull(); }, "actual source address dialog opens");
            sourceDialog->setTextValue(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).toString());
            sourceDialog->accept(); wait([] { return !pins()->busy(); }, "actual source chooser completes");
            check(pins()->count() == 5, "duplicate source keeps stable identity and count");
            auto *last = pinRow(4); last->forceActiveFocus(Qt::TabFocusReason);
            const auto movedKey = key(4); const auto volume=player()->getVolume(); QTest::keyClick(window, Qt::Key_Up, Qt::ControlModifier);
            wait([&] { return key(3) == movedKey; }, "actual Control-Up reorders saved pin");
            check(pinRow(3)->hasActiveFocus(), "keyboard reorder retains focus on same pin identity");
            check(player()->getVolume()==volume, "pin reorder shortcut does not leak into player volume");
            const auto settingsPath = context->getIntf()->mainSettings->fileName();
            const auto handle = CreateFileW(settingsPath.toStdWString().c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            check(handle != INVALID_HANDLE_VALUE, "owned real settings file can be locked exclusively");
            pinMenu(3, QStringLiteral("Unpin"));
            const bool rolledBack = pins()->count() == 5 && pins()->canRetry() && !pins()->error().isEmpty();
            CloseHandle(handle); check(rolledBack, "actual failed unpin preserves model and exposes checked retry");
            revealFooter(); click(named(QStringLiteral("juxtopposedSidebarRetry")));
            wait([] { return pins()->count() == 4 && !pins()->canRetry(); }, "actual Retry completes the failed unpin in the same process");
            auto *card = named(QStringLiteral("homeaudio0")); check(card != nullptr, "real Home media card remains after unpin");
            card->forceActiveFocus(Qt::TabFocusReason); QTest::qWait(80); QTest::keyClick(window, Qt::Key_F10, Qt::ShiftModifier); chooseMenu(QStringLiteral("Pin to sidebar"));
            wait([] { return pins()->count() == 5 && !pins()->busy(); }, "actual Home action re-pins removed media");
            pinMenu(4, QStringLiteral("Move up")); check(kindRow(QStringLiteral("media")) == 3, "native context menu reorders by stable key");
            exerciseRoutes(true); exerciseRecent(true); home();
            const auto collectionKey = key(kindRow(QStringLiteral("playlist")));
            pins()->activate(collectionKey, Qt::OtherFocusReason);
            history()->push({QStringLiteral("network")}, {}, Qt::OtherFocusReason);
            QTest::qWait(300);
            check(history()->viewPath().first() == QStringLiteral("network"), "route change cancels stale asynchronous collection activation"); home();
            window->resize(320,360); QTest::qWait(100);
            auto *row = pinRow(4); row->forceActiveFocus(Qt::TabFocusReason); QTest::keyClick(window,Qt::Key_Down);
            check(named(QStringLiteral("juxtopposedSidebarAdd"))->hasActiveFocus(), "narrow sidebar scrolls keyboard focus to Add item");
            capture(QStringLiteral("native-pins-narrow.png")); window->resize(1280,900);
            const auto manualPins = snapshot();
            auto *playingInput = player()->getInput();
            auto *privacy = context->getPrivacyPreferences();
            check(privacy->clearHistory(), "real coordinated privacy clear request is accepted");
            wait([&] { return !privacy->busy(); }, "real native history clear finishes", 15000);
            check(privacy->lastError().isEmpty() && snapshot() == manualPins, "clearing native history preserves exact manual pin descriptors and order");
            check(player()->getInput() == playingInput, "manual-pin history check preserves exact primary input");
            const auto source=QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("second.flac"));
            const auto target=QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath(QStringLiteral("temporarily-moved-owned-second.flac"));
            check(QFileInfo(source).canonicalFilePath().startsWith(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).canonicalPath()+QLatin1Char('/')) && !QFileInfo::exists(target), "missing-file check is confined to generated E output");
            addMenu(QStringLiteral("Files…")); filePicker(source);
            wait([] { return pins()->count()==6 && !pins()->busy(); }, "actual file chooser pins inactive second owned track");
            struct OwnedMove { QString source,target; bool moved=false; ~OwnedMove(){if(moved)QFile::rename(target,source);} } move{source,target};
            wait([&] { if(!move.moved)move.moved=QFile::rename(source,target);return move.moved; }, "inactive owned target can be moved temporarily");
            click(pinRow(5));
            wait([] { return !pins()->error().isEmpty(); }, "actual absent media pin reports unavailable target");
            check(!pins()->get(5).value(QStringLiteral("available")).toBool() && player()->getInput()==playingInput, "unavailable pin remains represented and preserves primary media");
            pinMenu(5,QStringLiteral("Unpin"));check(pins()->count()==5,"actual unavailable pin can be unpinned from its native menu");
            check(QFile::rename(target,source),"generated second track is restored to its exact original path");move.moved=false;
            report.insert("expected",snapshot());
            QFile expected(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath("pins-expected.json"));
            check(expected.open(QIODevice::WriteOnly|QIODevice::NewOnly), "fresh expected restart descriptor record opens"); expected.write(QJsonDocument(snapshot()).toJson());
        } else {
            QFile expected(QDir(qEnvironmentVariable("VLC_OWNED_UI_BASE")).filePath("pins-expected.json")); check(expected.open(QIODevice::ReadOnly), "cold process reads retained expected descriptor record");
            const auto saved = QJsonDocument::fromJson(expected.readAll()).array();
            wait([] { return pins()->count() == 5; }, "fresh native process restores five mixed pins");
            auto actual = snapshot();
            for (int i=0;i<actual.size();++i) { auto a=actual[i].toObject(),b=saved[i].toObject();a.remove("available");b.remove("available");check(a==b,"cold process preserves exact order typed IDs and Unicode descriptors"); }
            if (mode == QStringLiteral("replacement")) {
                auto *library=context->getMediaLibrary();
                check(library != nullptr, "replacement process owns a fresh native library");
                library->addFolder(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")));
                wait([&] { return library->idle() && library->homeActivity()->mediaCount()==3; }, "replacement library indexes genuine owned content", 30000);
                struct Probe { bool ready=false,reused=false; };
                auto result=std::make_shared<Probe>();
                const auto oldPlaylist=pins()->get(kindRow(QStringLiteral("playlist"))).value(QStringLiteral("nativeId")).toString().toLongLong();
                const auto oldArtist=pins()->get(kindRow(QStringLiteral("artist"))).value(QStringLiteral("nativeId")).toString().toLongLong();
                library->runOnMLThread<Probe>(library, [oldPlaylist,oldArtist](vlc_medialibrary_t *ml, Probe &probe) {
                    ml_unique_ptr<vlc_ml_playlist_t> created(vlc_ml_playlist_create(ml,"Replacement collection"));
                    ml_unique_ptr<vlc_ml_playlist_t> playlist(vlc_ml_get_playlist(ml,oldPlaylist));
                    ml_unique_ptr<vlc_ml_artist_t> artist(vlc_ml_get_artist(ml,oldArtist));
                    probe.reused=created&&playlist&&artist;
                },[result](quint64,Probe &probe){*result=probe;result->ready=true;});
                wait([&]{return result->ready;}, "replacement native identity probe completes");
                check(result->reused,"replacement database really reuses both saved collection IDs");
                pins()->refresh();
            }
            exerciseRoutes(mode == QStringLiteral("reopen"));
            exerciseRecent(mode == QStringLiteral("reopen"));
        }
        report.insert("finalPins",snapshot());
        report.insert("settingsFile",context->getIntf()->mainSettings->fileName());
        capture(QStringLiteral("native-pins-final.png")); report.insert("passed",true);
    } catch (const std::exception &error) {
        report.insert("passed",false); report.insert("error",QString::fromUtf8(error.what()));
        if(context) {
            report.insert("failedPins",snapshot()); report.insert("pinServiceError",pins()->error());
            report.insert("failedRoute",QJsonArray::fromStringList(history()->viewPath()));
        }
        if(window)window->grabWindow().save(QDir(qEnvironmentVariable("VLC_OWNED_UI_OUTPUT")).filePath("failed-window.png"));
    }
    report.insert("checks",checks); report.insert("ownedWindowOnly",true); report.insert("fullBetaAccepted",false);
    QFile output(qEnvironmentVariable("VLC_OWNED_UI_REPORT")); if(output.open(QIODevice::WriteOnly)) output.write(QJsonDocument(report).toJson());
    if (qEnvironmentVariable("VLC_OWNED_UI_MODE") == QStringLiteral("titlebar-close") && report.value("passed").toBool()) {
        QTimer::singleShot(200,qApp,[] {
            auto *button=named(QStringLiteral("juxtopposedStripclose"));
            if (!button) return;
            const auto point=button->mapToScene(button->boundingRect().center()).toPoint();
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,point);
        });
    } else QTimer::singleShot(200,qApp,[] { if(context)libvlc_Quit(vlc_object_instance(context->getIntf()));else QCoreApplication::quit(); });
}
void waitForStartupApproval()
{
    const auto gate = qEnvironmentVariable("VLC_OWNED_UI_START_GATE");
    if (gate.isEmpty() || QFileInfo::exists(gate)) {
        report.insert("startupGateUsed", !gate.isEmpty());
        QTimer::singleShot(250, qApp, run);
        return;
    }
    // No QtTest input, window manipulation or automatic timeout while a human
    // may be handling a separate Windows permission dialog.
    const auto readyPath = gate + QStringLiteral(".ready.json");
    if (!QFileInfo::exists(readyPath)) {
        QFile ready(readyPath);
        if (ready.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
            const QJsonObject state{{"waitingForStartupApproval", true},
                                    {"testActionsStarted", false},
                                    {"screenName", window && window->screen() ? window->screen()->name() : QString()},
                                    {"mode", qEnvironmentVariable("VLC_OWNED_UI_MODE")}};
            ready.write(QJsonDocument(state).toJson());
        }
    }
    QTimer::singleShot(250, qApp, waitForStartupApproval);
}
void waitForWindow(int remaining)
{
    auto *host=named(QStringLiteral("juxtopposedDialogsHost"));
    if(host&&host->window()) {
        auto *engine=qmlEngine(host);auto *candidate=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;
        if(candidate) { context=candidate;window=host->window();waitForStartupApproval();return; }
    }
    if(remaining>0) QTimer::singleShot(250,qApp,[remaining]{waitForWindow(remaining-1);});
    else waitForStartupApproval();
}
void startOwnedPinJourney()
{
    if(qEnvironmentVariableIsEmpty("VLC_OWNED_UI_REPORT"))return;
    _set_error_mode(_OUT_TO_STDERR);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer::singleShot(1000,qApp,[]{waitForWindow(120);});
}
}
Q_COREAPP_STARTUP_FUNCTION(startOwnedPinJourney)
