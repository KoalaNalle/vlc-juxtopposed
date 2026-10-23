// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player_profile_preferences.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <QCoreApplication>
#include <QLocale>
#include <cmath>

namespace {
QString tr(const char *message) { return QCoreApplication::translate("PlayerProfilePreferences", message); }
bool reject(QString &error, const QString &message) { error = message; return false; }
bool clean(const QString &text)
{
    const auto bytes = text.toUtf8();
    if (bytes.size() > 32760 || QString::fromUtf8(bytes) != text) return false;
    for (const auto c : text) if (c.unicode() < 32 || c.unicode() == 127) return false;
    return true;
}
bool numeric(const QVariant &value)
{
    switch (value.typeId()) {
    case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
    case QMetaType::ULongLong: case QMetaType::Float: case QMetaType::Double: return true;
    default: return false;
    }
}
QVariant read(const QByteArray &name)
{
    switch (config_GetType(name.constData())) {
    case VLC_VAR_BOOL: return config_GetInt(name.constData()) != 0;
    case VLC_VAR_INTEGER: return qlonglong(config_GetInt(name.constData()));
    case VLC_VAR_FLOAT: return double(config_GetFloat(name.constData()));
    case VLC_VAR_STRING: {
        char *raw = config_GetPsz(name.constData());
        const auto text = QString::fromUtf8(raw ? raw : ""); free(raw); return text;
    }
    default: return {};
    }
}
bool validate(const QByteArray &name, const QVariant &value, bool writing, QVariant &normalized)
{
    const int type = config_GetType(name.constData());
    const auto *config = config_FindConfig(name.constData());
    if (!config) return false;
    if (type == VLC_VAR_BOOL) {
        if (value.typeId() != QMetaType::Bool) return false;
        normalized = value; return true;
    }
    if (type == VLC_VAR_STRING) {
        if (value.typeId() != QMetaType::QString || !clean(value.toString())) return false;
        normalized = value; return true;
    }
    if (!numeric(value) || !std::isfinite(value.toDouble())) return false;
    if (type == VLC_VAR_INTEGER) {
        const double number = value.toDouble();
        // JSON must retain the integer exactly through a cold process restart.
        if (std::floor(number) != number || std::abs(number) > 9007199254740991.) return false;
        const qlonglong integer = qlonglong(number);
        if (writing && (integer < config->min.i || integer > config->max.i)) return false;
        normalized = integer; return true;
    }
    if (type == VLC_VAR_FLOAT) {
        const float number = value.toFloat();
        // Reject hidden double-to-float rounding in a manually edited profile.
        if (!std::isfinite(number) || double(number) != value.toDouble()) return false;
        if (writing) {
            if ((config->min.f != 0.f || config->max.f != 0.f)
                && (number < config->min.f || number > config->max.f)) return false;
            bool valid = false;
            const float saved = QLocale::c().toFloat(QString::number(double(number), 'f', 6), &valid);
            if (!valid || saved != number) return false;
        }
        normalized = QVariant::fromValue(number); return true;
    }
    return false;
}
}

QStringList PlayerProfilePreferences::groups()
{ return {QStringLiteral("playback"), QStringLiteral("audio"), QStringLiteral("video"), QStringLiteral("subtitles")}; }

QStringList PlayerProfilePreferences::nativeKeys(const QString &group)
{
    // Keep native helper fields alongside their owning presentation options:
    // e.g. repeat+loop, preset+bands+preamp and both subtitle unit systems.
    // The duplicated Playback/Video time-shift option has one owner here.
    if (group == QLatin1String("playback")) return {
        QStringLiteral("restore-playback-pos"), QStringLiteral("start-paused"), QStringLiteral("rate"),
        QStringLiteral("playlist-cork"), QStringLiteral("preferred-resolution"), QStringLiteral("menu-language"),
        QStringLiteral("audio-track"), QStringLiteral("audio-track-id"), QStringLiteral("sub-track"), QStringLiteral("sub-track-id"),
        QStringLiteral("repeat"), QStringLiteral("loop"), QStringLiteral("random"), QStringLiteral("play-and-stop"),
        QStringLiteral("play-and-exit"), QStringLiteral("input-fast-seek"), QStringLiteral("qt-timeline-thumbnails"), QStringLiteral("dvd"), QStringLiteral("vcd"),
        QStringLiteral("cd-audio"), QStringLiteral("avi-index"), QStringLiteral("mkv-preload-local-dir"),
        QStringLiteral("input-timeshift-path"), QStringLiteral("input-timeshift-granularity")};
    if (group == QLatin1String("audio")) return {
        QStringLiteral("audio"), QStringLiteral("aout"), QStringLiteral("mmdevice-audio-device"), QStringLiteral("mix-mode"),
        QStringLiteral("qt-max-volume"), QStringLiteral("volume-save"), QStringLiteral("mmdevice-volume"), QStringLiteral("volume-step"),
        QStringLiteral("audio-filter"), QStringLiteral("norm-max-level"), QStringLiteral("audio-replay-gain-mode"), QStringLiteral("audio-replay-gain-preamp"),
        QStringLiteral("equalizer-preset"), QStringLiteral("equalizer-bands"), QStringLiteral("equalizer-preamp"), QStringLiteral("equalizer-2pass"),
        QStringLiteral("equalizer-vlcfreqs"), QStringLiteral("pitch-shift"), QStringLiteral("audio-language"), QStringLiteral("audio-desync"),
        QStringLiteral("audio-visual"), QStringLiteral("effect-list"), QStringLiteral("mmdevice-passthrough"), QStringLiteral("force-dolby-surround"),
        QStringLiteral("audio-resampler"), QStringLiteral("soxr-resampler-quality"), QStringLiteral("role"), QStringLiteral("effect-width"), QStringLiteral("effect-height")};
    if (group == QLatin1String("video")) return {
        QStringLiteral("video"), QStringLiteral("fullscreen"), QStringLiteral("disable-screensaver"), QStringLiteral("video-wallpaper"),
        QStringLiteral("hue"), QStringLiteral("brightness"), QStringLiteral("contrast"), QStringLiteral("saturation"), QStringLiteral("gamma"),
        QStringLiteral("video-filter"), QStringLiteral("deinterlace"), QStringLiteral("deinterlace-mode"), QStringLiteral("aspect-ratio"),
        QStringLiteral("crop"), QStringLiteral("autoscale"), QStringLiteral("zoom"), QStringLiteral("fit"), QStringLiteral("align"),
        QStringLiteral("snapshot-path"), QStringLiteral("snapshot-prefix"), QStringLiteral("snapshot-format"), QStringLiteral("snapshot-width"),
        QStringLiteral("snapshot-height"), QStringLiteral("snapshot-sequential"), QStringLiteral("snapshot-preview"),
        QStringLiteral("custom-aspect-ratios"), QStringLiteral("custom-crop-ratios"), QStringLiteral("monitor-par")};
    if (group == QLatin1String("subtitles")) return {
        QStringLiteral("spu"), QStringLiteral("sub-language"), QStringLiteral("subsdec-encoding"), QStringLiteral("sub-autodetect-file"),
        QStringLiteral("sub-autodetect-fuzzy"), QStringLiteral("sub-autodetect-path"), QStringLiteral("freetype-bold"), QStringLiteral("freetype-opacity"),
        QStringLiteral("freetype-font"), QStringLiteral("freetype-color"), QStringLiteral("freetype-outline-color"),
        QStringLiteral("freetype-outline-thickness"), QStringLiteral("freetype-background-opacity"),
        QStringLiteral("freetype-shadow-opacity"), QStringLiteral("freetype-shadow-color"), QStringLiteral("freetype-shadow-distance"),
        QStringLiteral("freetype-shadow-angle"), QStringLiteral("freetype-background-color"), QStringLiteral("freetype-text-direction"),
        QStringLiteral("freetype-monofont"), QStringLiteral("sub-primary-alignment"), QStringLiteral("sub-delay"),
        QStringLiteral("osd-shortcut-feedback"), QStringLiteral("osd-feedback-duration"), QStringLiteral("video-title-show"), QStringLiteral("video-title-timeout"),
        QStringLiteral("sub-text-style-enabled"), QStringLiteral("sub-text-style-embedded"), QStringLiteral("sub-text-style-font"),
        QStringLiteral("sub-text-style-color"), QStringLiteral("sub-text-style-outline-color"), QStringLiteral("sub-text-style-size-mode"),
        QStringLiteral("sub-text-style-relative-size"), QStringLiteral("sub-text-style-pixel-size"),
        QStringLiteral("sub-text-style-relative-outline"), QStringLiteral("sub-text-style-pixel-outline"), QStringLiteral("sub-text-style-background-opacity")};
    return {};
}

bool PlayerProfilePreferences::snapshot(QVariantMap &result, QString &error) const
{
    error.clear(); result.clear();
    if (!m_intf) return reject(error, tr("The native player preferences service is unavailable."));
    QVariantMap captured;
    for (const auto &group : groups()) {
        QVariantMap fields;
        for (const auto &key : nativeKeys(group)) {
            const auto name = key.toUtf8();
            if (!config_GetType(name.constData())) continue;
            const auto value = read(name); QVariant normalized;
            if (!validate(name, value, false, normalized))
                return reject(error, tr("The existing %1 default cannot be preserved in a player profile. Correct it in Advanced first.").arg(key));
            fields.insert(key, value);
        }
        captured.insert(group, fields);
    }
    result = captured; return true;
}

QVariantMap PlayerProfilePreferences::compatibleDefaults(QVariantMap target)
{
    const auto group = QStringLiteral("playback");
    const auto key = QStringLiteral("qt-timeline-thumbnails");
    if (config_GetType("qt-timeline-thumbnails") == VLC_VAR_BOOL
        && target.value(group).typeId() == QMetaType::QVariantMap) {
        auto fields = target.value(group).toMap();
        if (!fields.contains(key)) {
            fields.insert(key, false);
            target.insert(group, fields);
        }
    }
    return target;
}

bool PlayerProfilePreferences::prepare(const QVariantMap &saved, QVariantMap &changes, QString &error) const
{
    const auto target = compatibleDefaults(saved);
    error.clear(); changes.clear(); QVariantMap current;
    if (!snapshot(current, error)) return false;
    if (target.keys() != current.keys()) return reject(error, tr("This profile has an unsupported player settings group."));
    QVariantMap prepared;
    for (const auto &group : groups()) {
        if (target.value(group).typeId() != QMetaType::QVariantMap)
            return reject(error, tr("This profile has invalid player settings."));
        const auto fields = target.value(group).toMap(); const auto before = current.value(group).toMap();
        if (fields.keys() != before.keys()) return reject(error, tr("This profile requires different native options. Its saved data has been preserved."));
        for (auto it = fields.cbegin(); it != fields.cend(); ++it) {
            const auto name = it.key().toUtf8(); QVariant normalized, original;
            if (!validate(name, it.value(), false, normalized))
                return reject(error, tr("The profile contains an invalid %1 default.").arg(it.key()));
            if (it.value() == before.value(it.key())) continue;
            if (!validate(name, it.value(), true, normalized) || !validate(name, before.value(it.key()), true, original))
                return reject(error, tr("The %1 default cannot be saved and restored exactly. Its range or precision needs correction in Advanced.").arg(it.key()));
            prepared.insert(it.key(), normalized);
        }
    }
    changes = prepared; return true;
}

bool PlayerProfilePreferences::apply(const QVariantMap &target, QString &error)
{
    QVariantMap changes;
    if (!prepare(target, changes, error)) return false;
    if (changes.isEmpty()) return true;
    // One native write avoids intermediate EQ/filter and multi-field UI states.
    // Runtime policy refresh belongs to the coordinator after this save succeeds.
    return CheckedPreferencesStore(m_intf).saveNative(changes, error);
}
