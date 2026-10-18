// SPDX-License-Identifier: GPL-2.0-or-later
// Private in-process QtTest driver. Never linked into the ordinary GUI.
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "qt.hpp"
#include "maininterface/mainctx.hpp"
#include "maininterface/sidebar_pins_model.hpp"
#include "maininterface/privacy_preferences_model.hpp"
#include "medialibrary/medialib.hpp"
#include "medialibrary/mlhomeactivity.hpp"
#include "medialibrary/mlhelper.hpp"
#include "player/player_controller.hpp"
#include "util/navigation_history.hpp"
#include <vlc_interface.h>
#include <vlc_player.h>
#include <QApplication>
#include <QAction>
#include <QFileInfo>
#include <QQmlPropertyMap>
#include <vlc_input_item.h>
#include <QDeadlineTimer>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QSet>
#include <QTest>
#include <QTimer>
#include <QWindow>
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
    auto *item = pinRow(row); item->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(window, Qt::Key_F10, Qt::ShiftModifier); chooseMenu(text);
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
void run()
{
    try {
        check(context && window, "owned native window and MainCtx became ready");
        window->resize(1280, 900); QTest::qWait(100);
        const auto mode = qEnvironmentVariable("VLC_OWNED_UI_MODE");
        report.insert("mode",mode); check(pins() != nullptr, "real MainCtx owns pin service");
        if (mode == QStringLiteral("populate")) {
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
            exerciseRoutes(true); home();
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
    QTimer::singleShot(200,qApp,[] { if(context)libvlc_Quit(vlc_object_instance(context->getIntf()));else QCoreApplication::quit(); });
}
void waitForWindow(int remaining)
{
    auto *host=named(QStringLiteral("juxtopposedDialogsHost"));
    if(host&&host->window()) {
        auto *engine=qmlEngine(host);auto *candidate=engine?engine->singletonInstance<MainCtx *>("VLC.MainInterface","MainCtx"):nullptr;
        if(candidate) { context=candidate;window=host->window();QTimer::singleShot(250,qApp,run);return; }
    }
    if(remaining>0) QTimer::singleShot(250,qApp,[remaining]{waitForWindow(remaining-1);});
    else run();
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
