// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "recording_preferences_model.hpp"
#include "qt.hpp"
#include "util/cast_discovery_policy.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_plugin.h>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>
#include <cmath>

namespace {
struct Option { const char *key, *native; int type; };
constexpr Option options[] = {
    {"recordingDirectory", "input-record-path", VLC_VAR_STRING},
    {"nativeRecording", "input-record-native", VLC_VAR_BOOL},
    {"castQuality", "sout-chromecast-conversion-quality", VLC_VAR_INTEGER},
    {"playLocally", "sout-display", VLC_VAR_BOOL},
    {"includeAudio", "sout-audio", VLC_VAR_BOOL},
    {"includeVideo", "sout-video", VLC_VAR_BOOL},
    {"includeSubtitles", "sout-spu", VLC_VAR_BOOL},
    {"encodingPreset", "sout-x264-preset", VLC_VAR_STRING},
    {"tuning", "sout-x264-tune", VLC_VAR_STRING},
};
QString nativeText(const char *key)
{
    char *raw = config_GetPsz(key);
    const auto value = QString::fromUtf8(raw ? raw : "");
    free(raw); return value;
}
QVariantMap choice(const QVariant &value, const QString &name)
{ return {{QStringLiteral("value"), value}, {QStringLiteral("name"), name}}; }
QVariantList stringChoices(const char *key)
{
    QVariantList result;
    if (config_GetType(key) != VLC_VAR_STRING) return result;
    char **values = nullptr, **names = nullptr;
    const auto count = config_GetPszChoices(key, &values, &names);
    for (ssize_t i = 0; i < count; ++i) {
        const auto value = QString::fromUtf8(values[i] ? values[i] : "");
        auto label = QString::fromUtf8(names[i] ? names[i] : "");
        if (label.isEmpty()) label = value.isEmpty() ? QObject::tr("None") : value;
        if (!label.isEmpty()) label[0] = label.at(0).toUpper();
        result.append(choice(value, label)); free(values[i]); free(names[i]);
    }
    free(values); free(names); return result;
}
QVariantList qualityChoices()
{
    const char *key = "sout-chromecast-conversion-quality";
    const auto *metadata = config_FindConfig(key);
    if (!metadata || config_GetType(key) != VLC_VAR_INTEGER) return {};
    // Automatic delegates to VLC's platform default. It is not an adaptive
    // bitrate feature and must not silently replace a different saved value.
    const qlonglong platformDefault = metadata->orig.i;
    QVariantList result{choice(platformDefault, QObject::tr("Automatic"))};
    int64_t *values = nullptr; char **names = nullptr;
    const auto count = config_GetIntChoices(key, &values, &names);
    for (ssize_t i = 0; i < count; ++i) {
        if (values[i] != platformDefault)
            result.append(choice(qlonglong(values[i]), QString::fromUtf8(names[i] ? names[i] : "")));
        free(names[i]);
    }
    free(values); free(names); return result;
}
bool listed(const QVariantList &choices, const QVariant &value)
{
    for (const auto &entry : choices)
        if (entry.toMap().value(QStringLiteral("value")) == value) return true;
    return false;
}
bool validFolder(const QString &value)
{
    if (value.isEmpty()) return true;
    const auto utf8 = value.toUtf8();
    if (utf8.size() > 32767 || QString::fromUtf8(utf8) != value) return false;
    for (const auto ch : value) if (ch.unicode() < 32 || ch.unicode() == 127) return false;
    const QFileInfo info(value);
    return info.isAbsolute() && info.exists() && info.isDir() && info.isWritable();
}
}

RecordingPreferencesModel::RecordingPreferencesModel(qt_intf_t *intf, QSettings *settings,
                                                     Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_settings(settings), m_consumers(consumers)
{
    m_choices[QStringLiteral("castQuality")] = qualityChoices();
    m_choices[QStringLiteral("encodingPreset")] = stringChoices("sout-x264-preset");
    m_choices[QStringLiteral("tuning")] = stringChoices("sout-x264-tune");
    refresh();
}
void RecordingPreferencesModel::setError(const QString &value)
{ if (value != m_error) { m_error = value; emit lastErrorChanged(); } }
bool RecordingPreferencesModel::fail(const QString &value) { setError(value); return false; }
void RecordingPreferencesModel::refresh()
{
    QVariantMap values, capabilities;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        const bool native = config_GetType(option.native) == option.type;
        const bool enabled = key == QStringLiteral("recordingDirectory") || key == QStringLiteral("nativeRecording")
            ? m_consumers.recording : key == QStringLiteral("castQuality") ? m_consumers.castQuality : m_consumers.streaming;
        capabilities[key] = m_intf && native && enabled
            && (!m_choices.contains(key) || !m_choices.value(key).toList().isEmpty());
        values[key] = !native ? QVariant{} : option.type == VLC_VAR_STRING ? QVariant(nativeText(option.native))
            : option.type == VLC_VAR_BOOL ? QVariant(config_GetInt(option.native) != 0) : QVariant(qlonglong(config_GetInt(option.native)));
    }
    values[QStringLiteral("discoverDevices")] = vlc::qt::castDiscoveryEnabled(m_settings);
    values[QStringLiteral("defaultRecordingDirectory")] = QVLCUserDir(VLC_VIDEOS_DIR);
    capabilities[QStringLiteral("discoverDevices")] = m_intf && m_settings && m_consumers.discovery;
    capabilities[QStringLiteral("browse")] = capabilities.value(QStringLiteral("recordingDirectory"));
    capabilities[QStringLiteral("advanced")] = m_intf && m_consumers.advanced;
    m_values = values; m_capabilities = capabilities; emit valuesChanged();
}
bool RecordingPreferencesModel::commandLineOverride(const char *key) const
{
    if (!m_intf) return false;
    auto *root = VLC_OBJECT(vlc_object_instance(m_intf));
    return var_Type(root, key) != 0;
}
QVariantMap RecordingPreferencesModel::applyModes() const
{
    QVariantMap result;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        result[key] = commandLineOverride(option.native) ? QStringLiteral("commandLineOverride")
            : key == QStringLiteral("recordingDirectory") || key == QStringLiteral("nativeRecording")
            ? QStringLiteral("nextInput") : QStringLiteral("nextJob");
    }
    result[QStringLiteral("discoverDevices")] = QStringLiteral("live");
    return result;
}
QVariantMap RecordingPreferencesModel::supportNotes() const
{
    return {
        {QStringLiteral("recordingDirectory"), tr("Existing writable local folder for recordings after opening the next media. Leave empty for VLC's default video or music folder. Running recordings keep their destination.")},
        {QStringLiteral("nativeRecording"), tr("Allow the source's native recording path when supported; otherwise VLC uses its normal recording fallback. Applies after opening the next media.")},
        {QStringLiteral("discoverDevices"), tr("Allow device discovery when opening the Cast menu. Turning this off stops discovery; it does not disconnect an active cast.")},
        {QStringLiteral("castQuality"), tr("Quality for the next cast requiring conversion. Automatic uses VLC's platform default; compatible media can play without conversion. Available devices and codecs still determine the result.")},
        {QStringLiteral("playLocally"), tr("Default local playback for the next Stream wizard. Its explicit local-output choice takes precedence. Running streams are unchanged.")},
        {QStringLiteral("streamTracks"), tr("Native output-track defaults for future stream jobs. Explicit media/profile options take precedence. Running jobs keep their existing tracks.")},
        {QStringLiteral("encodingPreset"), tr("Default x264 encoding preset for future streams that use x264. Explicit encoder options take precedence.")},
        {QStringLiteral("tuning"), tr("Default x264 tuning for future streams that use x264. None keeps the encoder's normal behavior. Explicit encoder options take precedence.")},
        {QStringLiteral("commandLineOverride"), tr("A command-line option currently overrides this saved default. Restart without that option to apply it.")},
    };
}
bool RecordingPreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setError({});
    if (busy()) return fail(tr("Finish choosing the recording folder first."));
    refresh();
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Recording setting is unavailable in this build."));
    QString error;
    if (key == QStringLiteral("discoverDevices")) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("Choose On or Off."));
        if (!CheckedPreferencesStore(m_intf).saveSettings(m_settings,
                QLatin1String(vlc::qt::castDiscoverySettingsKey), input, false, error)) return fail(error);
        refresh();
        if (!input.toBool()) emit discoveryDisabled();
        return true;
    }
    const Option *option = nullptr;
    for (const auto &candidate : options) if (key == QLatin1String(candidate.key)) { option = &candidate; break; }
    if (!option) return fail(tr("Unknown Recording setting."));
    QVariant value;
    if (option->type == VLC_VAR_BOOL) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("Choose On or Off."));
        value = input;
    } else if (option->type == VLC_VAR_STRING) {
        if (input.typeId() != QMetaType::QString) return fail(tr("Enter a supported text value."));
        value = input;
        if (key == QStringLiteral("recordingDirectory")) {
            const auto folder = input.toString();
            if (!validFolder(folder)) return fail(tr("Choose an existing writable local recording folder, or leave it empty."));
            value = folder;
        } else if (!listed(m_choices.value(key).toList(), value)) return fail(tr("Choose a listed encoder option."));
    } else {
        const auto type = input.typeId();
        if (type != QMetaType::Int && type != QMetaType::UInt && type != QMetaType::LongLong
            && type != QMetaType::ULongLong && type != QMetaType::Double && type != QMetaType::Float)
            return fail(tr("Choose a listed cast quality."));
        const auto number = input.toDouble();
        if (!std::isfinite(number) || number < 0 || number > 3 || std::floor(number) != number)
            return fail(tr("Choose a listed cast quality."));
        value = qlonglong(number);
        if (!listed(m_choices.value(key).toList(), value)) return fail(tr("Choose a listed cast quality."));
    }
    if (!CheckedPreferencesStore(m_intf).saveNative({{QString::fromLatin1(option->native), value}}, error)) return fail(error);
    refresh(); return true;
}
bool RecordingPreferencesModel::chooseRecordingDirectory()
{
    setError({});
    if (busy()) return fail(tr("A recording folder chooser is already open."));
    refresh();
    if (!m_capabilities.value(QStringLiteral("browse")).toBool()
        || !qobject_cast<QApplication *>(QCoreApplication::instance()))
        return fail(tr("The recording folder chooser is unavailable in this session."));
    m_pickerActive = true; emit busyChanged();
    QPointer<RecordingPreferencesModel> lifetime(this);
    auto initial = m_values.value(QStringLiteral("recordingDirectory")).toString();
    if (initial.isEmpty()) initial = m_values.value(QStringLiteral("defaultRecordingDirectory")).toString();
    const auto selected = QFileDialog::getExistingDirectory(nullptr, tr("Choose recording folder"), initial);
    if (!lifetime) return false;
    m_pickerActive = false; emit busyChanged();
    if (selected.isEmpty()) return true;
    return setValue(QStringLiteral("recordingDirectory"), selected);
}
bool RecordingPreferencesModel::openAdvanced()
{
    setError({});
    if (busy() || !m_capabilities.value(QStringLiteral("advanced")).toBool())
        return fail(tr("Advanced Recording preferences are unavailable."));
    emit advancedOptionsRequested(); return true;
}
void RecordingPreferencesModel::reload()
{
    if (busy()) { fail(tr("Finish choosing the recording folder first.")); return; }
    setError({}); refresh();
}
