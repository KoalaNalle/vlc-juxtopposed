// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_preset_model.hpp"
#include <QCryptographicHash>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUuid>
#include <QTimer>
#include <vlc_iso_lang.h>
#include <cmath>
#include <algorithm>
#include <limits>

namespace {
constexpr int MaximumPresets = 64, MaximumAssociations = 1024, MaximumBytes = 2 * 1024 * 1024;
constexpr qint64 MaximumDelay = 60LL * 60 * 1000000;
const QString SettingsKey = QStringLiteral("Juxtopposed/SubtitlePresets/v1");
bool integer(const QJsonObject &o, const char *key, qint64 low, qint64 high, qint64 *result)
{
    const auto value = o.value(QString::fromLatin1(key));
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || number < low || number > high || std::floor(number) != number) return false;
    *result = static_cast<qint64>(number); return true;
}
bool textTrack(const vlc_player_track *track)
{
    if (!track) return false;
    switch (track->fmt.i_codec) {
        case VLC_CODEC_SUBT: case VLC_CODEC_TEXT: case VLC_CODEC_QTXT:
        case VLC_CODEC_TX3G: case VLC_CODEC_USF: case VLC_CODEC_TTML:
        case VLC_CODEC_TTML_TS: case VLC_CODEC_WEBVTT: return true;
        default: return false;
    }
}
QString languageCode(const QString &value)
{
    const auto bytes = value.toUtf8();
    const auto *language = vlc_find_iso639(bytes.constData(), true);
    return language ? QString::fromLatin1(language->psz_iso639_2T) : value.toLower();
}
}

bool SubtitlePresetModel::TrackPolicy::operator==(const TrackPolicy &s) const
{
    return id == s.id && language == s.language && index == s.index && order == s.order
        && ownDelay == s.ownDelay && (!ownDelay || delay == s.delay);
}
bool SubtitlePresetModel::same(const Preset &a, const Preset &b, bool compareTracks)
{
    return a.style == b.style && a.delay == b.delay && a.fps == b.fps
        && (!compareTracks || a.tracks == b.tracks);
}
QJsonObject SubtitlePresetModel::encode(const Preset &p)
{
    const auto &s = p.style;
    QJsonObject style{{"enabled", s.enabled}, {"embedded", s.embedded}, {"font", s.font},
        {"text", s.text}, {"outline", s.outline}, {"sizeMode", s.sizeMode}, {"background", s.background},
        {"offset", s.offset}, {"relativeSize", s.relativeSize}, {"pixelSize", s.pixelSize},
        {"relativeOutline", s.relativeOutline}, {"pixelOutline", s.pixelOutline}, {"scale", s.scale},
        {"durationEnabled", s.durationEnabled}, {"durationMode", s.durationMode},
        {"durationOverlap", s.durationOverlap}, {"durationFactor", s.durationFactor}};
    QJsonArray tracks;
    for (const auto &t : p.tracks)
        tracks.append(QJsonObject{{"id", t.id}, {"language", t.language}, {"index", t.index},
            {"order", t.order}, {"ownDelay", t.ownDelay}, {"delay", static_cast<double>(t.delay)}});
    return {{"id", p.id}, {"name", p.name}, {"style", style},
        {"delay", static_cast<double>(p.delay)}, {"fps", p.fps}, {"tracks", tracks}};
}
bool SubtitlePresetModel::decode(const QJsonObject &o, Preset *result)
{
    if (!result || !o["id"].isString() || !o["name"].isString() || !o["style"].isObject()
        || !o["tracks"].isArray() || !o["fps"].isDouble()) return false;
    Preset p; p.id = o["id"].toString(); p.name = o["name"].toString();
    static const QRegularExpression identifier("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
    if (!identifier.match(p.id).hasMatch() || p.name.trimmed() != p.name
        || p.name.isEmpty() || p.name.size() > 120 || p.name.contains(QChar::Null)) return false;
    if (!integer(o, "delay", -MaximumDelay, MaximumDelay, &p.delay)) return false;
    const double fps = o["fps"].toDouble();
    if (!std::isfinite(fps) || fps < 0 || fps > 1000) return false;
    p.fps = fps;
    const auto style = o["style"].toObject(); auto &s = p.style;
    for (const auto *key : {"enabled", "embedded", "durationEnabled"})
        if (!style[key].isBool()) return false;
    if (!style["font"].isString()) return false;
    s.enabled = style["enabled"].toBool(); s.embedded = style["embedded"].toBool();
    s.durationEnabled = style["durationEnabled"].toBool(); s.font = style["font"].toString();
    for (const auto &entry : {std::pair{"text", &s.text}, {"outline", &s.outline},
             {"sizeMode", &s.sizeMode}, {"background", &s.background}, {"offset", &s.offset},
             {"scale", &s.scale}, {"durationMode", &s.durationMode}, {"durationOverlap", &s.durationOverlap}}) {
        qint64 number;
        if (!integer(style, entry.first, std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), &number)) return false;
        *entry.second = number;
    }
    for (const auto &entry : {std::pair{"relativeSize", &s.relativeSize}, {"pixelSize", &s.pixelSize},
             {"relativeOutline", &s.relativeOutline}, {"pixelOutline", &s.pixelOutline}, {"durationFactor", &s.durationFactor}}) {
        if (!style[entry.first].isDouble() || !std::isfinite(style[entry.first].toDouble())) return false;
        *entry.second = style[entry.first].toDouble();
    }
    if (!SubtitleAppearance::validSnapshot(s)) return false;
    const auto tracks = o["tracks"].toArray();
    if (tracks.isEmpty() || tracks.size() > 2) return false;
    for (const auto &value : tracks) {
        if (!value.isObject()) return false;
        const auto object = value.toObject(); TrackPolicy t; qint64 index, order;
        if (!object["id"].isString() || !object["language"].isString() || !object["ownDelay"].isBool()
            || !integer(object, "index", 0, 1023, &index)
            || !integer(object, "order", VLC_VOUT_ORDER_PRIMARY, VLC_VOUT_ORDER_SECONDARY, &order)
            || !integer(object, "delay", -MaximumDelay, MaximumDelay, &t.delay)) return false;
        t.id = object["id"].toString(); t.language = object["language"].toString();
        t.index = index; t.order = order; t.ownDelay = object["ownDelay"].toBool();
        if (t.id.isEmpty() || t.id.size() > 1024 || t.language.size() > 64
            || t.id.contains(QChar::Null) || t.language.contains(QChar::Null)) return false;
        for (const auto &existing : p.tracks) if (existing.id == t.id) return false;
        p.tracks.append(std::move(t));
    }
    if (p.tracks[0].order != VLC_VOUT_ORDER_PRIMARY
        || (p.tracks.size() == 2 && p.tracks[1].order != VLC_VOUT_ORDER_SECONDARY)) return false;
    *result = std::move(p); return true;
}

SubtitlePresetModel::SubtitlePresetModel(vlc_player_t *player, SubtitleAppearance *style,
    QSettings *settings, vlc_object_t *configuration, QObject *parent)
    : QObject(parent), m_player(player), m_style(style), m_settings(settings), m_configuration(configuration)
{
    m_default.id = QStringLiteral("default"); m_default.name = qtr("Default Preset");
    m_default.style = SubtitleAppearance::configuredSnapshot(configuration);
    m_default.delay = configuration ? vlc_tick_from_samples(var_InheritInteger(configuration, "sub-delay"), 10) : 0;
    m_default.fps = configuration ? var_InheritFloat(configuration, "sub-fps") : 0.f;
    if (player) {
        static const vlc_player_cbs callbacks = [] {
            vlc_player_cbs c{};
            c.on_state_changed = playerStateChanged; c.on_current_media_changed = mediaChanged; c.on_track_list_changed = tracksChanged;
            c.on_track_selection_changed = selectionChanged; c.on_category_delay_changed = delayChanged;
            c.on_track_delay_changed = trackDelayChanged; c.on_associated_subs_fps_changed = fpsChanged;
            c.on_vout_changed = outputChanged; return c;
        }();
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(player);
        m_listener = vlc_player_AddListener(player, &callbacks, this);
    }
    if (style) connect(style, &SubtitleAppearance::stateChanged, this, &SubtitlePresetModel::refresh);
    reload();
}
SubtitlePresetModel::~SubtitlePresetModel()
{
    if (m_listener) {
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
        vlc_player_RemoveListener(m_player, m_listener);
    }
    // No destructor writes: reset preferences and failed saves must stay reset.
}
void SubtitlePresetModel::queueRefresh()
{
    const auto generation = m_generation.load();
    QMetaObject::invokeMethod(this, [this, generation] {
        if (generation == m_generation.load()) refresh();
    }, Qt::QueuedConnection);
}
void SubtitlePresetModel::playerStateChanged(vlc_player_t *, vlc_player_state state, void *opaque)
{
    auto *self = static_cast<SubtitlePresetModel *>(opaque);
    // A stopped input can restart with the same input_item pointer. Its delays
    // and tracks are new engine state, so it still needs a fresh restoration.
    if (state == VLC_PLAYER_STATE_STARTED) ++self->m_generation;
    self->queueRefresh();
}
void SubtitlePresetModel::mediaChanged(vlc_player_t *, input_item_t *, void *opaque)
{
    auto *self = static_cast<SubtitlePresetModel *>(opaque); ++self->m_generation; self->queueRefresh();
}
void SubtitlePresetModel::tracksChanged(vlc_player_t *, vlc_player_list_action, const vlc_player_track *t, void *opaque)
{ if (t->fmt.i_cat == SPU_ES) static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }
void SubtitlePresetModel::selectionChanged(vlc_player_t *, vlc_es_id_t *, vlc_es_id_t *, void *opaque)
{ static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }
void SubtitlePresetModel::delayChanged(vlc_player_t *, es_format_category_e cat, vlc_tick_t, void *opaque)
{ if (cat == SPU_ES) static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }
void SubtitlePresetModel::trackDelayChanged(vlc_player_t *, vlc_es_id_t *, vlc_tick_t, void *opaque)
{ static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }
void SubtitlePresetModel::fpsChanged(vlc_player_t *, float, void *opaque)
{ static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }
void SubtitlePresetModel::outputChanged(vlc_player_t *, vlc_player_vout_action, vout_thread_t *,
    vlc_vout_order, vlc_es_id_t *, void *opaque)
{ static_cast<SubtitlePresetModel *>(opaque)->queueRefresh(); }

void SubtitlePresetModel::fail(const QString &error) { m_error = error; emit stateChanged(); }
void SubtitlePresetModel::clearError() { if (!m_error.isEmpty()) { m_error.clear(); emit stateChanged(); } }
const SubtitlePresetModel::Preset *SubtitlePresetModel::find(const QString &id) const
{
    if (id == m_default.id) return &m_default;
    for (const auto &p : m_presets) if (p.id == id) return &p;
    return nullptr;
}
QVariantList SubtitlePresetModel::entries() const
{
    QVariantList entries;
    auto append = [&](const Preset &p) {
        entries.append(QVariantMap{{"id", p.id}, {"name", p.name},
            {"selected", p.id == m_selectedId && !m_edited && m_pendingId.isEmpty()}});
    };
    append(m_default); for (const auto &p : m_presets) append(p); return entries;
}
QString SubtitlePresetModel::selectedName() const
{
    if (m_edited) return qtr("Custom settings");
    const auto *preset = find(m_pendingId.isEmpty() ? m_selectedId : m_pendingId);
    return preset ? preset->name : qtr("Default Preset");
}
bool SubtitlePresetModel::canApply() const
{
    if (!m_player || !m_media || !m_style || !m_style->available() || !m_storageValid) return false;
    vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
    if (m_contextGeneration != m_generation.load() || vlc_player_GetCurrentMedia(m_player) != m_media.get()) return false;
    for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
        const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
        if (track->selected) {
            vlc_vout_order order = VLC_VOUT_ORDER_NONE;
            if (!textTrack(track) || !vlc_player_GetEsIdVout(m_player, track->es_id, &order)
                || order == VLC_VOUT_ORDER_NONE) return false;
        }
    }
    return true;
}
bool SubtitlePresetModel::canSave() const { return canApply() && m_presets.size() < MaximumPresets && m_pendingId.isEmpty(); }
QString SubtitlePresetModel::statusText() const
{
    if (!m_storageValid) return qtr("Saved subtitle presets could not be read. Reset Preferences can clear the damaged data.");
    if (!m_pendingId.isEmpty()) return qtr("Waiting for a supported subtitle track and video output.");
    if (!canApply()) {
        const auto status = m_style ? m_style->statusText() : qtr("Subtitle playback is unavailable.");
        return status.isEmpty() ? qtr("Select supported text subtitle tracks to use appearance presets.") : status;
    }
    if (m_presets.size() >= MaximumPresets) return qtr("The maximum of 64 subtitle presets has been reached.");
    return m_edited ? qtr("These settings have changed since the preset was applied.") : QString{};
}
void SubtitlePresetModel::reload()
{
    m_resetStyleNext = m_resetStyleNext || m_haveApplied;
    ++m_requestSerial;
    m_presets.clear(); m_associations.clear(); m_pendingId.clear(); m_selectedId = m_default.id;
    m_storageValid = true; m_haveApplied = false; m_edited = false; m_error.clear();
    m_selectionRequested = false; m_writesSubmitted = false;
    m_default.style = SubtitleAppearance::configuredSnapshot(m_configuration);
    m_default.delay = m_configuration ? vlc_tick_from_samples(var_InheritInteger(m_configuration, "sub-delay"), 10) : 0;
    m_default.fps = m_configuration ? var_InheritFloat(m_configuration, "sub-fps") : 0.f;
    if (!m_settings) { m_storageValid = false; fail(qtr("Subtitle preset storage is unavailable.")); return; }
    const auto bytes = m_settings->value(SettingsKey).toByteArray();
    if (bytes.size() > MaximumBytes) {
        m_storageValid = false; fail(qtr("The saved subtitle preset data is too large.")); return;
    }
    if (!bytes.isEmpty()) {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(bytes, &parseError);
        bool valid = bytes.size() <= MaximumBytes && parseError.error == QJsonParseError::NoError && document.isObject();
        const auto object = document.object(); qint64 schema = 0;
        valid = valid && integer(object, "schema", 1, 1, &schema)
            && object["presets"].isArray() && object["media"].isObject();
        const auto presets = object["presets"].toArray();
        const auto media = object["media"].toObject();
        valid = valid && presets.size() <= MaximumPresets && media.size() <= MaximumAssociations;
        QList<Preset> loaded;
        if (valid) for (const auto &value : presets) {
            Preset p;
            if (!value.isObject() || !decode(value.toObject(), &p)) { valid = false; break; }
            for (const auto &existing : loaded)
                if (existing.id == p.id || existing.name.compare(p.name, Qt::CaseInsensitive) == 0) valid = false;
            if (!valid) break;
            loaded.append(std::move(p));
        }
        QHash<QString, QString> associations;
        static const QRegularExpression keyPattern("^[0-9a-f]{64}$");
        if (valid) for (auto it = media.begin(); it != media.end(); ++it) {
            bool exists = it.value().isString() && it.value().toString() == m_default.id;
            for (const auto &p : loaded) if (it.value().isString() && it.value().toString() == p.id) exists = true;
            if (!keyPattern.match(it.key()).hasMatch() || !exists) { valid = false; break; }
            associations.insert(it.key(), it.value().toString());
        }
        if (!valid) { m_storageValid = false; fail(qtr("Saved subtitle preset data has an unsupported version or invalid values.")); }
        else { m_presets = std::move(loaded); m_associations = std::move(associations); }
    }
    m_media.reset(); m_mediaKey.clear(); refresh();
}
bool SubtitlePresetModel::persist(const QList<Preset> &presets, const QHash<QString, QString> &associations)
{
    if (!m_storageValid || !m_settings || associations.size() > MaximumAssociations) {
        fail(qtr("Subtitle preset storage is unavailable or full.")); return false;
    }
    QJsonArray records; for (const auto &p : presets) records.append(encode(p));
    QJsonObject media; for (auto it = associations.begin(); it != associations.end(); ++it) media[it.key()] = it.value();
    const auto bytes = QJsonDocument(QJsonObject{{"schema", 1}, {"presets", records}, {"media", media}}).toJson(QJsonDocument::Compact);
    if (bytes.size() > MaximumBytes) { fail(qtr("The saved subtitle preset data is too large.")); return false; }
    const auto previous = m_settings->value(SettingsKey);
    const bool existed = m_settings->contains(SettingsKey);
    m_settings->setValue(SettingsKey, bytes); m_settings->sync();
    if (m_settings->status() != QSettings::NoError) {
        // Restore the in-memory value too; a later QSettings flush must not
        // silently turn the reported failed operation into a successful save.
        if (existed) m_settings->setValue(SettingsKey, previous); else m_settings->remove(SettingsKey);
        fail(qtr("The subtitle preset could not be saved. Check access to your settings folder.")); return false;
    }
    return true;
}
void SubtitlePresetModel::refreshMedia()
{
    if (!m_player) return;
    SharedInputItem media;
    QString key;
    {
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
        media.reset(vlc_player_GetCurrentMedia(m_player));
        if (media) {
            auto uri = vlc::wrap_cptr(input_item_GetURI(media.get()));
            // Do not put media paths, stream credentials or passwords in the
            // preset document. Association identity is a deterministic digest.
            key = QString::fromLatin1(QCryptographicHash::hash(QByteArray(uri.get()), QCryptographicHash::Sha256).toHex());
        }
    }
    const auto generation = m_generation.load();
    if (media.get() == m_media.get() && generation == m_contextGeneration) return;
    m_contextGeneration = generation;
    ++m_requestSerial;
    m_resetStyleNext = m_resetStyleNext || m_haveApplied;
    m_media = std::move(media); m_mediaKey = std::move(key);
    m_haveApplied = false; m_edited = false; m_explicit = false; m_selectionRequested = false;
    m_writesSubmitted = false; m_selectedId = m_default.id;
    m_pendingId = m_media && m_storageValid ? m_associations.value(m_mediaKey, m_resetStyleNext ? m_default.id : QString{}) : QString{};
    if (m_storageValid) m_error.clear();
}
bool SubtitlePresetModel::capture(Preset *result) const
{
    if (!result || !canApply()) return false;
    Preset p;
    if (!m_style->snapshot(&p.style, m_media.get())) return false;
    vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
    if (m_contextGeneration != m_generation.load() || vlc_player_GetCurrentMedia(m_player) != m_media.get()) return false;
    auto *output = vlc_player_vout_Hold(m_player);
    if (!output) return false;
    vout_Release(output);
    p.delay = vlc_player_GetSubtitleDelay(m_player); p.fps = vlc_player_GetAssociatedSubsFPS(m_player);
    if (p.delay < -MaximumDelay || p.delay > MaximumDelay || !std::isfinite(p.fps) || p.fps < 0 || p.fps > 1000) return false;
    const size_t count = vlc_player_GetTrackCount(m_player, SPU_ES);
    for (size_t i = 0; i < count; ++i) {
        const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
        if (!track->selected) continue;
        TrackPolicy policy; policy.id = QString::fromUtf8(vlc_es_id_GetStrId(track->es_id));
        policy.language = languageCode(QString::fromUtf8(track->fmt.psz_language)); policy.index = i;
        vlc_vout_order order = VLC_VOUT_ORDER_NONE;
        if (!vlc_player_GetEsIdVout(m_player, track->es_id, &order) || order == VLC_VOUT_ORDER_NONE) return false;
        policy.order = order;
        const auto delay = vlc_player_GetEsIdDelay(m_player, track->es_id);
        policy.ownDelay = delay != INT64_MAX; policy.delay = policy.ownDelay ? delay : 0;
        if (policy.id.isEmpty() || policy.id.size() > 1024 || policy.index > 1023
            || policy.language.size() > 64 || policy.delay < -MaximumDelay || policy.delay > MaximumDelay) return false;
        p.tracks.append(std::move(policy));
    }
    if (p.tracks.isEmpty() || p.tracks.size() > 2) return false;
    std::sort(p.tracks.begin(), p.tracks.end(), [](const auto &a, const auto &b) { return a.order < b.order; });
    if (p.tracks[0].order != VLC_VOUT_ORDER_PRIMARY
        || (p.tracks.size() == 2 && p.tracks[1].order != VLC_VOUT_ORDER_SECONDARY)) return false;
    *result = std::move(p); return true;
}
void SubtitlePresetModel::updateMatch()
{
    Preset live;
    if (capture(&live)) {
        const auto &expected = m_haveApplied ? m_applied : m_default;
        m_edited = !same(live, expected, m_selectedId != m_default.id);
    }
}
void SubtitlePresetModel::refresh()
{
    if (m_applying) return;
    refreshMedia(); tryApply(); updateMatch(); emit stateChanged();
}
bool SubtitlePresetModel::save(const QString &input)
{
    const auto name = input.trimmed();
    if (!m_storageValid) return false;
    if (m_presets.size() >= MaximumPresets) { fail(qtr("The maximum of 64 subtitle presets has been reached.")); return false; }
    if (name.isEmpty() || name.size() > 120 || name.contains(QChar::Null)) {
        fail(qtr("Enter a preset name of 1 to 120 characters.")); return false;
    }
    if (name.compare(m_default.name, Qt::CaseInsensitive) == 0) { fail(qtr("Choose a name other than Default Preset.")); return false; }
    for (const auto &p : m_presets) if (p.name.compare(name, Qt::CaseInsensitive) == 0) {
        fail(qtr("A subtitle preset with this name already exists.")); return false;
    }
    Preset p;
    if (!canSave() || !capture(&p)) { fail(qtr("Select a text subtitle track before saving its current settings.")); return false; }
    const Preset live = p;
    if (!p.style.enabled) {
        // Freeze the effective native Freetype defaults when saving a named
        // preset. Merely storing disabled custom-style variables would make
        // this preset's appearance change whenever global defaults change.
        p.style.font = m_style->fontFamily();
        p.style.text = QColor(m_style->textColor()).rgb() & 0xFFFFFF;
        p.style.outline = QColor(m_style->outlineColor()).rgb() & 0xFFFFFF;
        p.style.relativeOutline = m_style->outlineSize();
        p.style.background = qRound(m_style->backgroundOpacity() * 255.f / 100.f);
        p.style.sizeMode = 0; p.style.relativeSize = 6.25f;
        p.style.embedded = true; p.style.enabled = true;
        if (!SubtitleAppearance::validSnapshot(p.style)) {
            fail(qtr("The current subtitle appearance could not be captured.")); return false;
        }
    }
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces); p.name = name;
    auto presets = m_presets; presets.append(p);
    auto associations = m_associations; associations.insert(m_mediaKey, p.id);
    if (!persist(presets, associations)) return false;
    m_presets = std::move(presets); m_associations = std::move(associations);
    m_selectedId = p.id; m_applied = live; m_haveApplied = true; m_edited = false; m_error.clear();
    emit stateChanged(); return true;
}
bool SubtitlePresetModel::apply(const QString &id)
{
    if (!m_storageValid) return false;
    if (!find(id)) { fail(qtr("This subtitle preset no longer exists.")); return false; }
    if (!canApply()) { fail(qtr("Select a text subtitle track before applying this preset.")); return false; }
    ++m_requestSerial;
    m_pendingId = id; m_explicit = true; m_selectionRequested = false; m_writesSubmitted = false;
    const bool applied = tryApply(); updateMatch(); emit stateChanged(); return applied || !m_pendingId.isEmpty();
}
bool SubtitlePresetModel::tryApply()
{
    if (m_pendingId.isEmpty() || m_applying || !m_media || !m_style) return false;
    if (m_writesSubmitted) return confirmSubmittedApply();
    const auto *record = find(m_pendingId);
    if (!record) { m_pendingId.clear(); return false; }
    const Preset preset = *record;
    Preset submitted = preset; submitted.tracks.clear();
    const bool explicitRequest = m_explicit;
    const auto generation = m_generation.load();
    QList<vlc_es_id_t *> targets;
    {
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
        if (m_contextGeneration != generation || generation != m_generation.load()
            || vlc_player_GetCurrentMedia(m_player) != m_media.get()) return false;
        const size_t count = vlc_player_GetTrackCount(m_player, SPU_ES);
        for (const auto &policy : preset.tracks) {
            const vlc_player_track *chosen = nullptr;
            for (size_t i = 0; i < count; ++i) {
                const auto *candidate = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (textTrack(candidate) && QString::fromUtf8(vlc_es_id_GetStrId(candidate->es_id)) == policy.id) { chosen = candidate; break; }
            }
            if (!chosen && !policy.language.isEmpty()) for (size_t i = 0; i < count; ++i) {
                const auto *candidate = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (textTrack(candidate) && languageCode(QString::fromUtf8(candidate->fmt.psz_language)) == policy.language
                    && !targets.contains(candidate->es_id)) { chosen = candidate; break; }
            }
            if (!chosen && policy.index >= 0 && static_cast<size_t>(policy.index) < count)
                chosen = vlc_player_GetTrackAt(m_player, SPU_ES, policy.index);
            if (!textTrack(chosen) || targets.contains(chosen->es_id)) {
                for (auto *target : targets) vlc_es_id_Release(target);
                if (explicitRequest) { m_pendingId.clear(); fail(qtr("A saved subtitle track is unavailable in this media.")); }
                return false;
            }
            targets.append(vlc_es_id_Hold(chosen->es_id));
            // Track matching may fall back to language/ordinal in another
            // input. Confirm against the actual resolved IDs, never old ESs.
            auto resolved = policy;
            resolved.id = QString::fromUtf8(vlc_es_id_GetStrId(chosen->es_id));
            resolved.language = languageCode(QString::fromUtf8(chosen->fmt.psz_language));
            for (size_t i = 0; i < count; ++i)
                if (vlc_player_GetTrackAt(m_player, SPU_ES, i) == chosen) resolved.index = i;
            submitted.tracks.append(std::move(resolved));
        }
        if (!targets.isEmpty()) {
            bool selected = true;
            size_t selectedCount = 0;
            for (size_t i = 0; i < count; ++i) {
                const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                if (track->selected) {
                    ++selectedCount;
                    if (!targets.contains(track->es_id)) selected = false;
                }
            }
            selected = selected && selectedCount == static_cast<size_t>(targets.size());
            bool rolesReady = true, wrongRole = false;
            for (qsizetype i = 0; i < targets.size(); ++i) {
                vlc_vout_order order = VLC_VOUT_ORDER_NONE;
                if (!vlc_player_GetEsIdVout(m_player, targets[i], &order)
                    || order == VLC_VOUT_ORDER_NONE) rolesReady = false;
                else if (order != preset.tracks[i].order) wrongRole = true;
            }
            if (!selected || wrongRole) {
                if (!m_selectionRequested) {
                    // EsOutSelectList keeps the role of an already selected
                    // ES. A genuine role change needs ordered deselection
                    // before selection; the input control FIFO preserves this
                    // order and retains each track's own native delay.
                    if (wrongRole) vlc_player_UnselectTrackCategory(m_player, SPU_ES);
                    auto list = targets; list.append(nullptr);
                    if (vlc_player_SelectEsIdList(m_player, SPU_ES, list.data()) != static_cast<unsigned>(targets.size())) {
                        for (auto *target : targets) vlc_es_id_Release(target);
                        m_pendingId.clear(); fail(qtr("The saved subtitle tracks could not be selected.")); return false;
                    }
                    m_selectionRequested = true;
                    const auto requested = m_pendingId;
                    const auto serial = m_requestSerial;
                    QTimer::singleShot(3000, this, [this, generation, requested, serial] {
                        if (generation == m_generation.load() && serial == m_requestSerial
                            && m_pendingId == requested && m_selectionRequested) {
                            m_pendingId.clear(); m_selectionRequested = false;
                            fail(qtr("The saved subtitle tracks did not become selected."));
                        }
                    });
                }
                for (auto *target : targets) vlc_es_id_Release(target);
                return false;
            }
            if (!rolesReady) {
                // Selected decoders can exist before their first vout/cue.
                // Wait for a real output event without reselection or writes.
                for (auto *target : targets) vlc_es_id_Release(target);
                return false;
            }
            m_selectionRequested = false;
        }
    }
    if (!canApply()) {
        for (auto *target : targets) vlc_es_id_Release(target);
        return false;
    }
    m_applying = true;
    bool success = m_style->applySnapshot(preset.style, m_media.get(), &m_generation, m_contextGeneration);
    if (success) {
        // The output is now ours even if media changes before delay mutation.
        m_resetStyleNext = true;
        m_writesSubmitted = true; m_submitted = std::move(submitted);
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
        success = generation == m_generation.load() && vlc_player_GetCurrentMedia(m_player) == m_media.get();
        if (success) {
            success = vlc_player_SetCategoryDelay(m_player, SPU_ES, preset.delay, VLC_PLAYER_WHENCE_ABSOLUTE) == VLC_SUCCESS;
            vlc_player_SetAssociatedSubsFPS(m_player, preset.fps);
            if (preset.id == m_default.id) {
                // Category delay does not override an explicit per-track
                // delay. Default must also clear overrides left by presets.
                for (size_t i = 0; i < vlc_player_GetTrackCount(m_player, SPU_ES); ++i) {
                    const auto *track = vlc_player_GetTrackAt(m_player, SPU_ES, i);
                    success = vlc_player_SetEsIdDelay(m_player, track->es_id, INT64_MAX,
                        VLC_PLAYER_WHENCE_ABSOLUTE) == VLC_SUCCESS && success;
                }
            }
            for (qsizetype i = 0; i < targets.size(); ++i)
                success = vlc_player_SetEsIdDelay(m_player, targets[i], preset.tracks[i].ownDelay ? preset.tracks[i].delay : INT64_MAX,
                    VLC_PLAYER_WHENCE_ABSOLUTE) == VLC_SUCCESS && success;
        }
    }
    for (auto *target : targets) vlc_es_id_Release(target);
    m_applying = false;
    if (!success) {
        if (explicitRequest || m_writesSubmitted) {
            m_pendingId.clear(); m_writesSubmitted = false;
            fail(qtr("This preset could not be applied to the current subtitle output."));
        }
        return false;
    }
    const auto requested = m_pendingId;
    const auto serial = m_requestSerial;
    QTimer::singleShot(3000, this, [this, generation, requested, serial] {
        if (generation == m_generation.load() && serial == m_requestSerial
            && m_pendingId == requested && m_writesSubmitted) {
            if (confirmSubmittedApply()) { emit stateChanged(); return; }
            m_pendingId.clear(); m_writesSubmitted = false;
            fail(qtr("The applied subtitle settings could not be confirmed on the current output."));
        }
    });
    return confirmSubmittedApply();
}
bool SubtitlePresetModel::confirmSubmittedApply()
{
    if (!m_writesSubmitted || m_pendingId.isEmpty()) return false;
    Preset applied;
    if (!capture(&applied) || !same(applied, m_submitted, m_submitted.id != m_default.id))
        return false;
    // Readiness can disappear after mutation. Native callbacks may retry this
    // read, but must never replay category/FPS/ES writes and flood the FIFO.
    const bool explicitRequest = m_explicit;
    const auto presetId = m_submitted.id;
    m_selectedId = presetId; m_pendingId.clear(); m_explicit = false; m_writesSubmitted = false;
    m_applied = std::move(applied); m_haveApplied = true;
    m_resetStyleNext = false; m_edited = false; m_error.clear();
    if (explicitRequest) {
        auto associations = m_associations; associations.insert(m_mediaKey, presetId);
        if (persist(m_presets, associations)) m_associations = std::move(associations);
        else m_error = qtr("The preset is applied, but its media association could not be saved.");
    }
    return true;
}

void SubtitlePresetModel::refreshDefaults()
{
    m_default.style = SubtitleAppearance::configuredSnapshot(m_configuration);
    m_default.delay = m_configuration ? vlc_tick_from_samples(var_InheritInteger(m_configuration, "sub-delay"), 10) : 0;
    m_default.fps = m_configuration ? var_InheritFloat(m_configuration, "sub-fps") : 0.f;
    updateMatch(); emit stateChanged();
}
