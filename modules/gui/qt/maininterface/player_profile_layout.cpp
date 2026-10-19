// SPDX-License-Identifier: GPL-2.0-or-later
#include "player_profile_layout.hpp"
#include "player/control_list_model.hpp"
#include "player/player_controlbar_model.hpp"
#include "style/vlcicons.hpp"
#include <cmath>

namespace {
using C = ControlListModel;
QStringList names() { return {QStringLiteral("video"), QStringLiteral("audio")}; }
bool validPlayer(int player) { return player == 0 || player == 1; }
bool validColumn(int column) { return column >= 0 && column < 3; }
bool tileValue(const QVariant &value, int &id)
{
    switch (value.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Double: break;
    default: return false;
    }
    const auto number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < 0 || number > 255) return false;
    id = int(number); return PlayerProfileLayout::supportedTile(id);
}
QVariantMap player(const QVariantList &left, const QVariantList &center, const QVariantList &right, bool above)
{ return {{QStringLiteral("columns"), QVariantList{left, center, right}}, {QStringLiteral("timelineAbove"), above}}; }
}

PlayerProfileLayout::PlayerProfileLayout(QObject *parent) : QObject(parent)
{
    load(m_active, defaults()); load(m_draft, defaults());
}
QVariantList PlayerProfileLayout::palette()
{
    struct Tile { int id; const char *name; };
    // Authored order: 9 + 9 + 9 + 4. Core transport/tracks/volume remain
    // valid persisted tiles but are not additional invented palette entries.
    static const Tile tiles[] = {
        {C::WIDGET_SPACER,"Spacer"}, {C::SPLITTER,"Divider"}, {C::PLAYBACK_SPEED_BUTTON,"Playback Speed"},
        {C::SKIP_FW_BUTTON,"Forward 10s"}, {C::SKIP_BACK_BUTTON,"Backward 10s"},
        {C::JUXTO_FORWARD_15,"Forward 15s"}, {C::JUXTO_BACKWARD_15,"Backward 15s"}, {C::STOP_BUTTON,"Stop"}, {C::LOOP_BUTTON,"Repeat"},
        {C::RANDOM_BUTTON,"Shuffle"}, {C::RECORD_BUTTON,"Record"}, {C::SNAPSHOT_BUTTON,"Snapshot"},
        {C::JUXTO_PICTURE_IN_PICTURE,"Picture in Picture"}, {C::JUXTO_HDR,"HDR"}, {C::JUXTO_ZOOM,"Zoom"},
        {C::JUXTO_RIGHT_SIDEBAR,"Right Sidebar"}, {C::QUIT_BUTTON,"Quit VLC"}, {C::JUXTO_VLC_SETTINGS,"VLC Settings"},
        {C::JUXTO_GO_HOME,"Go Home"}, {C::JUXTO_THUMBNAIL,"Thumbnail"}, {C::JUXTO_FILE_NAME,"File Name"},
        {C::JUXTO_ARTIST_NAME,"Artist Name"}, {C::JUXTO_FOLDER_NAME,"Folder Name"}, {C::JUXTO_PLAYLIST_NAME,"Playlist Name"},
        {C::INFO_BUTTON,"Properties"}, {C::JUXTO_REPLAY,"Replay"}, {C::JUXTO_CHAPTERS,"Chapters"},
        {C::JUXTO_FAVORITE,"Favorite"}, {C::BOOKMARK_BUTTON,"Bookmark"}, {C::JUXTO_BOOKMARKS,"Bookmarks"}, {C::JUXTO_PLAYLISTS,"Playlists"}};
    QVariantList result;
    for (const auto &tile : tiles) result.append(QVariantMap{{QStringLiteral("id"), tile.id},
        {QStringLiteral("name"), tr(tile.name)}});
    return result;
}
bool PlayerProfileLayout::supportedTile(int id)
{
    for (const auto &tile : palette()) if (tile.toMap().value(QStringLiteral("id")).toInt() == id) return true;
    return id == C::PLAY_BUTTON || id == C::PREVIOUS_BUTTON || id == C::NEXT_BUTTON || id == C::VOLUME
        || id == C::LANG_BUTTON || id == C::JUXTO_AUDIO_TRACKS || id == C::PLAYLIST_BUTTON || id == C::FULLSCREEN_BUTTON
        || id == C::EXTENDED_BUTTON;
}
QVariantMap PlayerProfileLayout::tileInfo(int id) const
{
    QVariantMap result;
    for (const auto &entry : palette()) {
        const auto tile = entry.toMap();
        if (tile.value(QStringLiteral("id")).toInt() == id) { result = tile; break; }
    }
    const char *name = nullptr;
    if (result.isEmpty()) {
        switch (id) {
        case C::PLAY_BUTTON: name = "Play"; break;
        case C::PREVIOUS_BUTTON: name = "Previous"; break;
        case C::NEXT_BUTTON: name = "Next"; break;
        case C::VOLUME: name = "Volume"; break;
        case C::LANG_BUTTON: name = "Subtitle tracks"; break;
        case C::JUXTO_AUDIO_TRACKS: name = "Audio tracks"; break;
        case C::PLAYLIST_BUTTON: name = "Play queue"; break;
        case C::FULLSCREEN_BUTTON: name = "Fullscreen"; break;
        case C::EXTENDED_BUTTON: name = "Playback settings"; break;
        default: return {};
        }
        result = {{QStringLiteral("id"),id},{QStringLiteral("name"),tr(name)}};
    }
    QString kind = QStringLiteral("button"), key;
    switch (id) {
    case C::PLAY_BUTTON: kind = QStringLiteral("play"); break;
    case C::PREVIOUS_BUTTON: kind = QStringLiteral("previous"); break;
    case C::NEXT_BUTTON: kind = QStringLiteral("next"); break;
    case C::LANG_BUTTON: kind = QStringLiteral("subtitles"); break;
    case C::JUXTO_AUDIO_TRACKS: kind = QStringLiteral("audio"); break;
    case C::EXTENDED_BUTTON: kind = QStringLiteral("settings"); break;
    case C::PLAYLIST_BUTTON: kind = QStringLiteral("queue"); break;
    case C::VOLUME: kind = QStringLiteral("volume"); break;
    case C::WIDGET_SPACER: kind = QStringLiteral("spacer"); break;
    case C::SPLITTER: kind = QStringLiteral("divider"); break;
    case C::JUXTO_THUMBNAIL: kind = QStringLiteral("artwork"); break;
    case C::JUXTO_FILE_NAME: kind = QStringLiteral("metadata"); key = QStringLiteral("file"); break;
    case C::JUXTO_ARTIST_NAME: kind = QStringLiteral("metadata"); key = QStringLiteral("artist"); break;
    case C::JUXTO_FOLDER_NAME: kind = QStringLiteral("metadata"); key = QStringLiteral("folder"); break;
    case C::JUXTO_PLAYLIST_NAME: kind = QStringLiteral("metadata"); key = QStringLiteral("playlist"); break;
    }
    result.insert(QStringLiteral("kind"),kind);
    if (!key.isEmpty()) result.insert(QStringLiteral("key"),key);
    // Cached exports do not contain the customizer's individual icon artwork.
    // Reuse VLC's existing font for a functional adaptation; keep that fidelity
    // limitation explicit rather than drawing replacement SVGs.
    QString glyph, family = VLCIcons::fontFamily;
    switch (id) {
    case C::PLAY_BUTTON: glyph = VLCIcons::play_filled; break;
    case C::PREVIOUS_BUTTON: glyph = VLCIcons::previous; break;
    case C::NEXT_BUTTON: glyph = VLCIcons::next; break;
    case C::VOLUME: glyph = VLCIcons::volume_high; break;
    case C::LANG_BUTTON: case C::JUXTO_AUDIO_TRACKS: glyph = VLCIcons::audiosub; break;
    case C::EXTENDED_BUTTON: case C::JUXTO_VLC_SETTINGS: glyph = VLCIcons::settings; break;
    case C::PLAYLIST_BUTTON: case C::JUXTO_RIGHT_SIDEBAR: glyph = VLCIcons::panel_left; break;
    case C::FULLSCREEN_BUTTON: glyph = VLCIcons::fullscreen; break;
    case C::WIDGET_SPACER: glyph = VLCIcons::space; break;
    case C::SPLITTER: glyph = QStringLiteral("│"); family = QStringLiteral("Inter"); break;
    case C::PLAYBACK_SPEED_BUTTON: glyph = VLCIcons::time; break;
    case C::SKIP_FW_BUTTON: glyph = VLCIcons::skip_for; break;
    case C::SKIP_BACK_BUTTON: glyph = VLCIcons::skip_back; break;
    case C::JUXTO_FORWARD_15: glyph = QStringLiteral("+15"); family = QStringLiteral("Inter"); break;
    case C::JUXTO_BACKWARD_15: glyph = QStringLiteral("−15"); family = QStringLiteral("Inter"); break;
    case C::STOP_BUTTON: glyph = VLCIcons::stop; break;
    case C::LOOP_BUTTON: glyph = VLCIcons::repeat_all; break;
    case C::RANDOM_BUTTON: glyph = VLCIcons::shuffle; break;
    case C::RECORD_BUTTON: glyph = VLCIcons::record; break;
    case C::SNAPSHOT_BUTTON: glyph = VLCIcons::snapshot; break;
    case C::JUXTO_PICTURE_IN_PICTURE: glyph = QStringLiteral("PiP"); family = QStringLiteral("Inter"); break;
    case C::JUXTO_HDR: glyph = QStringLiteral("HDR"); family = QStringLiteral("Inter"); break;
    case C::JUXTO_ZOOM: glyph = VLCIcons::search; break;
    case C::QUIT_BUTTON: glyph = VLCIcons::eject; break;
    case C::JUXTO_GO_HOME: glyph = VLCIcons::home; break;
    case C::JUXTO_THUMBNAIL: glyph = VLCIcons::album_cover; break;
    case C::JUXTO_FILE_NAME: case C::JUXTO_ARTIST_NAME: case C::JUXTO_FOLDER_NAME:
    case C::JUXTO_PLAYLIST_NAME: case C::INFO_BUTTON: glyph = VLCIcons::info; break;
    case C::JUXTO_REPLAY: glyph = VLCIcons::repeat_one; break;
    case C::JUXTO_CHAPTERS: case C::JUXTO_PLAYLISTS: glyph = VLCIcons::list; break;
    case C::JUXTO_FAVORITE: glyph = QStringLiteral("♡"); family = QStringLiteral("Inter"); break;
    case C::BOOKMARK_BUTTON: case C::JUXTO_BOOKMARKS: glyph = VLCIcons::bookmark; break;
    }
    result.insert(QStringLiteral("glyph"),glyph);
    result.insert(QStringLiteral("fontFamily"),family);
    return result;
}
bool PlayerProfileLayout::defaultVideo() const
{
    return snapshot().value(QStringLiteral("video")) == defaults().value(QStringLiteral("video"));
}
QVariantMap PlayerProfileLayout::defaults()
{
    return {{QStringLiteral("version"), 1},
        {QStringLiteral("video"), player({C::PREVIOUS_BUTTON,C::PLAY_BUTTON,C::NEXT_BUTTON}, {},
            {C::VOLUME,C::JUXTO_AUDIO_TRACKS,C::LANG_BUTTON,C::EXTENDED_BUTTON,C::PLAYLIST_BUTTON}, false)},
        {QStringLiteral("audio"), player({C::JUXTO_THUMBNAIL,C::JUXTO_FILE_NAME,C::JUXTO_ARTIST_NAME},
            {C::RANDOM_BUTTON,C::PREVIOUS_BUTTON,C::PLAY_BUTTON,C::NEXT_BUTTON,C::LOOP_BUTTON},
            {C::VOLUME,C::JUXTO_AUDIO_TRACKS,C::EXTENDED_BUTTON,C::PLAYLIST_BUTTON}, true)}};
}
bool PlayerProfileLayout::validate(const QVariantMap &snapshot, QString &error)
{
    error.clear();
    auto reject = [&error] {error = tr("This player layout has invalid columns or unsupported controls. Its saved data has been preserved."); return false;};
    if (snapshot.keys() != QStringList{QStringLiteral("audio"),QStringLiteral("version"),QStringLiteral("video")}) return reject();
    const auto version = snapshot.value(QStringLiteral("version"));
    if (version.typeId() == QMetaType::Bool || version.typeId() == QMetaType::QString
        || !version.canConvert<double>() || version.toDouble() != 1.) return reject();
    for (const auto &name : names()) {
        if (snapshot.value(name).typeId() != QMetaType::QVariantMap) return reject();
        const auto data = snapshot.value(name).toMap();
        if (data.keys() != QStringList{QStringLiteral("columns"),QStringLiteral("timelineAbove")}
            || data.value(QStringLiteral("timelineAbove")).typeId() != QMetaType::Bool
            || data.value(QStringLiteral("columns")).typeId() != QMetaType::QVariantList) return reject();
        const auto columns = data.value(QStringLiteral("columns")).toList(); if (columns.size() != 3) return reject();
        int count = 0;
        for (const auto &column : columns) {
            if (column.typeId() != QMetaType::QVariantList) return reject();
            for (const auto &entry : column.toList()) {int id; if (!tileValue(entry,id) || ++count > 64) return reject();}
        }
    }
    return true;
}
QVariantMap PlayerProfileLayout::capture(const ControlbarProfile &profile, bool videoAbove, bool audioAbove)
{
    auto result = defaults();
    for (int identifier = 0; identifier < 2; ++identifier) {
        QVariantList columns;
        for (const auto &column : profile.getModelData(identifier)) {QVariantList values; for (int id : column) values.append(id); columns.append(QVariant(values));}
        result[names()[identifier]] = QVariantMap{{QStringLiteral("columns"),columns},
            {QStringLiteral("timelineAbove"),identifier == 0 ? videoAbove : audioAbove}};
    }
    return result;
}
void PlayerProfileLayout::load(ControlbarProfile &profile, const QVariantMap &snapshot)
{
    for (int identifier = 0; identifier < 2; ++identifier) {
        const auto columns = snapshot.value(names()[identifier]).toMap().value(QStringLiteral("columns")).toList();
        std::array<QVector<int>,3> data;
        for (int c = 0; c < 3; ++c) for (const auto &entry : columns[c].toList()) data[c].append(entry.toInt());
        profile.setModelData(identifier,data);
    }
    profile.resetDirty();
}
QVariantMap PlayerProfileLayout::snapshot() const { return capture(m_active,m_videoAbove,m_audioAbove); }
bool PlayerProfileLayout::apply(const QVariantMap &value, QString &error)
{
    if (!validate(value,error)) return false;
    load(m_active,value); m_videoAbove = value.value(QStringLiteral("video")).toMap().value(QStringLiteral("timelineAbove")).toBool();
    m_audioAbove = value.value(QStringLiteral("audio")).toMap().value(QStringLiteral("timelineAbove")).toBool();
    emit changed(); return true;
}
PlayerControlbarModel *PlayerProfileLayout::video() const {return m_active.getModel(0);}
PlayerControlbarModel *PlayerProfileLayout::audio() const {return m_active.getModel(1);}
PlayerControlbarModel *PlayerProfileLayout::draftVideo() const {return m_draft.getModel(0);}
PlayerControlbarModel *PlayerProfileLayout::draftAudio() const {return m_draft.getModel(1);}
void PlayerProfileLayout::setError(const QString &error) {if (m_error != error) {m_error = error; emit lastErrorChanged();}}
bool PlayerProfileLayout::fail(const QString &message) {setError(message); return false;}
void PlayerProfileLayout::beginEdit()
{
    if (m_editing) return;
    load(m_draft,snapshot()); m_draftVideoAbove = m_videoAbove; m_draftAudioAbove = m_audioAbove;
    m_editing = true; setError({}); emit editingChanged(); emit draftChanged();
}
void PlayerProfileLayout::cancelEdit()
{
    if (!m_editing) return;
    m_editing = false; setError({}); emit editingChanged();
}
QVariantMap PlayerProfileLayout::draftSnapshot() const {return capture(m_draft,m_draftVideoAbove,m_draftAudioAbove);}
bool PlayerProfileLayout::insertTile(int player, int column, int index, int id)
{
    if (!m_editing || !validPlayer(player) || !validColumn(column) || !supportedTile(id)) return fail(tr("Choose a valid player control and destination."));
    auto data = m_draft.getModelData(player); int count = 0; for (const auto &c : data) count += c.size();
    if (count >= 64 || index < 0 || index > data[column].size()) return fail(tr("This control cannot be inserted at that position."));
    data[column].insert(index,id); m_draft.setModelData(player,data); setError({}); emit draftChanged(); return true;
}
bool PlayerProfileLayout::moveTile(int player, int fromColumn, int fromIndex, int toColumn, int toIndex)
{
    if (!m_editing || !validPlayer(player) || !validColumn(fromColumn) || !validColumn(toColumn)) return fail(tr("Choose a valid control and destination."));
    auto data = m_draft.getModelData(player);
    if (fromIndex < 0 || fromIndex >= data[fromColumn].size() || toIndex < 0 || toIndex > data[toColumn].size()) return fail(tr("This control has moved. Choose it again."));
    const int id = data[fromColumn].takeAt(fromIndex);
    if (fromColumn == toColumn && toIndex > fromIndex) --toIndex;
    data[toColumn].insert(toIndex,id); m_draft.setModelData(player,data); setError({}); emit draftChanged(); return true;
}
bool PlayerProfileLayout::removeTile(int player, int column, int index)
{
    if (!m_editing || !validPlayer(player) || !validColumn(column)) return fail(tr("Choose a valid player control."));
    auto data = m_draft.getModelData(player);
    if (index < 0 || index >= data[column].size()) return fail(tr("This control has moved. Choose it again."));
    data[column].removeAt(index); m_draft.setModelData(player,data); setError({}); emit draftChanged(); return true;
}
bool PlayerProfileLayout::setTimelineAbove(int player, bool above)
{
    if (!m_editing || !validPlayer(player)) return fail(tr("Open Customize Player before changing this layout."));
    (player == 0 ? m_draftVideoAbove : m_draftAudioAbove) = above; setError({}); emit draftChanged(); return true;
}
void PlayerProfileLayout::resetDraft()
{
    if (!m_editing) return;
    load(m_draft,defaults()); m_draftVideoAbove = false; m_draftAudioAbove = true;
    setError({}); emit draftChanged();
}
