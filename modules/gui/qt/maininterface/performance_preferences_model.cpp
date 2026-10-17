// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "performance_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include "util/plugin_cache_rescan.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_process_priority.h>
#include <vlc_cxx_helpers.hpp>
#include <QByteArrayList>
#include <QMap>
#include <QRegularExpression>
#include <cmath>
#include <cstring>

namespace {
struct Option { const char *key, *native; int type; };
constexpr Option options[] = {
    {"hardwareDecoder","dec-dev",VLC_VAR_STRING},
    {"postprocessing","postproc-q",VLC_VAR_INTEGER},
    {"loopFilter","avcodec-skiploopfilter",VLC_VAR_INTEGER},
    {"decodingThreads","avcodec-threads",VLC_VAR_INTEGER},
    {"dropLateFrames","drop-late-frames",VLC_VAR_BOOL},
    {"realTimePriority","win32-realtime-priority",VLC_VAR_BOOL},
    {"processPriority","win32-process-priority",VLC_VAR_INTEGER},
    {"encoder","sout-transcode-preferred-encoder",VLC_VAR_STRING},
    {"decoder","codec",VLC_VAR_STRING},
    {"clockSync","clock-synchro",VLC_VAR_INTEGER},
    {"clockJitter","clock-jitter",VLC_VAR_INTEGER},
    {"clockAverage","cr-average",VLC_VAR_INTEGER},
};
QString text(const char *key)
{
    if (config_GetType(key) != VLC_VAR_STRING) return {};
    auto value = vlc::wrap_cptr(config_GetPsz(key));
    return QString::fromUtf8(value ? value.get() : "");
}
bool integer(const QVariant &input, qlonglong low, qlonglong high, qlonglong &value)
{
    switch (input.typeId()) {
        case QMetaType::Int: case QMetaType::UInt: case QMetaType::LongLong:
        case QMetaType::ULongLong: case QMetaType::Float: case QMetaType::Double: break;
        default: return false;
    }
    const double number = input.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < low || number > high) return false;
    value = qlonglong(number); return true;
}
bool listed(const QVariantList &choices, const QVariant &value)
{
    for (const auto &entry : choices)
        if (entry.toMap().value(QStringLiteral("value")) == value) return true;
    return false;
}
QVariantMap choice(const QVariant &value, const QString &name)
{ return {{QStringLiteral("value"),value},{QStringLiteral("name"),name}}; }
QVariantList modules(std::initializer_list<const char *> capabilities)
{
    QMap<QString,QString> names;
    size_t count = 0;
    module_t **list = module_list_get(&count);
    for (size_t i = 0; i < count; ++i)
        for (const auto *capability : capabilities)
            if (module_provides(list[i], capability))
                names.insert(QString::fromUtf8(module_get_object(list[i])),
                             QString::fromUtf8(module_get_name(list[i], true)));
    module_list_free(list);
    QVariantList result{choice(QStringLiteral("any"), QObject::tr("Automatic"))};
    for (auto it = names.cbegin(); it != names.cend(); ++it)
        if (it.key() != QStringLiteral("any")) result.append(choice(it.key(), it.value()));
    return result;
}
QVariantList nativeIntegers(const char *key)
{
    QVariantList result;
    if (config_GetType(key) != VLC_VAR_INTEGER) return result;
    int64_t *values = nullptr; char **names = nullptr;
    const auto count = config_GetIntChoices(key, &values, &names);
    for (ssize_t i = 0; i < count; ++i) {
        result.append(choice(qlonglong(values[i]), QString::fromUtf8(names[i] ? names[i] : "")));
        free(names[i]);
    }
    free(values); free(names); return result;
}
struct Postprocessing {
    bool editable = true, multiple = false;
    unsigned count = 0;
    int quality = -1;
    QByteArray removed, enabled;
};
Postprocessing postprocessing(int replacement = -2)
{
    Postprocessing result;
    QByteArrayList removed, enabled;
    auto remaining = vlc::wrap_cptr(config_GetPsz("video-filter"));
    if (remaining && strlen(remaining.get()) > 65536) { result.editable = false; return result; }
    const auto nativeQuality = config_GetType("postproc-q") == VLC_VAR_INTEGER ? config_GetInt("postproc-q") : 0;
    result.quality = nativeQuality >= 0 && nativeQuality <= 6 ? int(nativeQuality) : -2;
    while (remaining && *remaining) {
        char *name = nullptr; config_chain_t *options = nullptr;
        auto next = vlc::wrap_cptr(config_ChainCreate(&name, &options, remaining.get()));
        const size_t length = strlen(remaining.get()) - (next ? strlen(next.get()) + 1 : 0);
        const QByteArray original(remaining.get(), length);
        const bool owned = name && (!strcmp(name,"postproc") || !strcmp(name,"postprocess") || !strcmp(name,"pp"));
        if (!owned) { removed.append(original); enabled.append(original); }
        else {
            ++result.count;
            QByteArrayList fields;
            for (auto *entry = options; entry; entry = entry->p_next) {
                if (!entry->psz_name || !QRegularExpression(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9-]*$"))
                         .match(QString::fromUtf8(entry->psz_name)).hasMatch()) { result.editable = false; continue; }
                if (!strcmp(entry->psz_name, "q")) {
                    bool ok = false;
                    const auto quality = QString::fromUtf8(entry->psz_value ? entry->psz_value : "").toInt(&ok);
                    if (!ok || quality < 0 || quality > 6) result.editable = false;
                    else result.quality = quality;
                    if (replacement >= 1) continue;
                }
                QByteArray field(entry->psz_name);
                if (entry->psz_value) {
                    auto escaped = vlc::wrap_cptr(config_StringEscape(entry->psz_value));
                    if (!escaped) result.editable = false;
                    else field += "='" + QByteArray(escaped.get()) + "'";
                }
                fields.append(field);
            }
            if (replacement >= 1) fields.append("q=" + QByteArray::number(replacement));
            enabled.append("postproc{" + fields.join(',') + "}");
        }
        free(name); config_ChainDestroy(options); remaining = std::move(next);
        if (removed.size() + result.count > 128) { result.editable = false; break; }
    }
    result.multiple = result.count > 1;
    if (!result.count) {
        result.quality = -1;
        if (replacement >= 1) enabled.append("postproc{q=" + QByteArray::number(replacement) + "}");
    } else if (result.quality == 0) result.quality = -1;
    result.removed = removed.join(':'); result.enabled = enabled.join(':');
    return result;
}
QString className(const vlc_process_priority_state &state)
{
    if (state.realtime) return QObject::tr("Real time");
    switch (state.level) {
        case -2: return QObject::tr("Idle"); case -1: return QObject::tr("Below normal");
        case 0: return QObject::tr("Normal"); case 1: return QObject::tr("Above normal");
        case 2: return QObject::tr("High"); default: return QObject::tr("Unavailable");
    }
}
}

PerformancePreferencesModel::PerformancePreferencesModel(qt_intf_t *intf, Consumers consumers, QObject *parent)
    : QObject(parent), m_intf(intf), m_consumers(consumers),
      m_scan(std::make_unique<PluginCacheRescan>(intf ? VLC_OBJECT(intf) : nullptr))
{
    connect(m_scan.get(), &PluginCacheRescan::busyChanged, this, &PerformancePreferencesModel::busyChanged);
    connect(m_scan.get(), &PluginCacheRescan::finished, this, [this](bool success) {
        if (success) { setError({}); setStatus(tr("Plugins rescanned. Restart VLC to use the updated cache.")); }
        else { setStatus({}); setError(m_scan->lastError()); }
        refresh();
    });
    refresh();
    if (m_intf && m_consumers.priority) {
        auto *root = VLC_OBJECT(vlc_object_instance(m_intf));
        if (var_Type(root, "win32-priority-request-error")) {
            const auto error = var_GetInteger(root, "win32-priority-request-error");
            if (error) setError(tr("Windows did not apply the startup scheduling request (error %1). Current class: %2.")
                                .arg(error).arg(m_values.value(QStringLiteral("actualPriority")).toString()));
        }
    }
}
PerformancePreferencesModel::~PerformancePreferencesModel() = default;
bool PerformancePreferencesModel::busy() const { return m_scan->busy(); }
void PerformancePreferencesModel::setError(const QString &value)
{ if (m_error != value) { m_error = value; emit lastErrorChanged(); } }
void PerformancePreferencesModel::setStatus(const QString &value)
{ if (m_status != value) { m_status = value; emit statusChanged(); } }
bool PerformancePreferencesModel::fail(const QString &value) { setError(value); return false; }
bool PerformancePreferencesModel::commandLineOverride(const QString &key) const
{
    if (!m_intf) return false;
    auto *root = VLC_OBJECT(vlc_object_instance(m_intf));
    for (const auto &option : options) if (key == QLatin1String(option.key))
        return var_Type(root, option.native) != 0
            || (key == QStringLiteral("hardwareDecoder") && var_Type(root, "hw-dec"))
            || (key == QStringLiteral("postprocessing") && var_Type(root, "video-filter"));
    return false;
}
void PerformancePreferencesModel::refresh()
{
    QVariantMap values, caps, choices;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        const bool type = config_GetType(option.native) == option.type;
        values[key] = option.type == VLC_VAR_STRING ? QVariant(text(option.native))
            : option.type == VLC_VAR_BOOL ? QVariant(type && config_GetInt(option.native) != 0)
            : QVariant(type ? qlonglong(config_GetInt(option.native)) : qlonglong(0));
        const bool consumer = key == QStringLiteral("postprocessing") ? m_consumers.postprocessing
            : key == QStringLiteral("encoder") ? m_consumers.encoder
            : key.startsWith(QStringLiteral("clock")) ? m_consumers.clocks
            : key == QStringLiteral("realTimePriority") || key == QStringLiteral("processPriority") ? m_consumers.priority
            : m_consumers.decoding;
        caps[key] = m_intf && consumer && type;
    }
    choices[QStringLiteral("hardwareDecoder")] = modules({"decoder device"});
    auto hardware = choices.value(QStringLiteral("hardwareDecoder")).toList();
    hardware.append(choice(QStringLiteral("off"), tr("Off"))); choices[QStringLiteral("hardwareDecoder")] = hardware;
    caps[QStringLiteral("hardwareDecoder")] = caps.value(QStringLiteral("hardwareDecoder")).toBool()
        && config_GetType("hw-dec") == VLC_VAR_BOOL;
    if (config_GetType("hw-dec") == VLC_VAR_BOOL && !config_GetInt("hw-dec"))
        values[QStringLiteral("hardwareDecoder")] = QStringLiteral("off");
    choices[QStringLiteral("loopFilter")] = nativeIntegers("avcodec-skiploopfilter");
    choices[QStringLiteral("clockSync")] = nativeIntegers("clock-synchro");
    choices[QStringLiteral("encoder")] = modules({"audio encoder","video encoder","spu encoder"});
    choices[QStringLiteral("decoder")] = modules({"audio decoder","video decoder","spu decoder"});
    if (values.value(QStringLiteral("encoder")).toString().isEmpty()
        || values.value(QStringLiteral("encoder")) == QStringLiteral("none"))
        values[QStringLiteral("encoder")] = QStringLiteral("any");
    if (values.value(QStringLiteral("decoder")).toString().isEmpty())
        values[QStringLiteral("decoder")] = QStringLiteral("any");
    QVariantList qualities{choice(-1, tr("Off"))};
    for (int i = 1; i <= 6; ++i) qualities.append(choice(i, tr("Level %1").arg(i)));
    choices[QStringLiteral("postprocessing")] = qualities;
    const auto filters = postprocessing();
    values[QStringLiteral("postprocessing")] = filters.editable && !filters.multiple && filters.quality >= -1
        ? QVariant(filters.quality) : QVariant(QStringLiteral("custom"));
    caps[QStringLiteral("postprocessing")] = caps.value(QStringLiteral("postprocessing")).toBool()
        && config_GetType("video-filter") == VLC_VAR_STRING && module_exists("postproc") && filters.editable;
    vlc_process_priority_state state;
    const bool priorityAvailable = vlc_process_priority_get(&state) == VLC_SUCCESS;
    caps[QStringLiteral("processPriority")] = caps.value(QStringLiteral("processPriority")).toBool() && priorityAvailable;
    caps[QStringLiteral("realTimePriority")] = caps.value(QStringLiteral("realTimePriority")).toBool() && priorityAvailable;
    if (priorityAvailable) {
        values[QStringLiteral("realTimePriority")] = state.realtime;
        if (values.value(QStringLiteral("processPriority")).toLongLong() == VLC_PROCESS_PRIORITY_INHERIT)
            values[QStringLiteral("processPriority")] = state.level;
    }
    values[QStringLiteral("actualPriority")] = priorityAvailable ? className(state) : tr("Unavailable");
    values[QStringLiteral("prioritySavedRealTime")] = config_GetType("win32-realtime-priority") == VLC_VAR_BOOL
        && config_GetInt("win32-realtime-priority") != 0;
    caps[QStringLiteral("rescan")] = m_intf && m_consumers.rescan && m_scan->available();
    caps[QStringLiteral("advanced")] = m_intf != nullptr;
    m_values = values; m_capabilities = caps; m_choices = choices; emit valuesChanged();
}
QVariantMap PerformancePreferencesModel::applyModes() const
{
    QVariantMap modes;
    for (const auto &option : options) {
        const auto key = QString::fromLatin1(option.key);
        modes[key] = key == QStringLiteral("realTimePriority") || key == QStringLiteral("processPriority")
            ? QStringLiteral("live") : commandLineOverride(key) ? QStringLiteral("commandLineOverride")
            : key == QStringLiteral("hardwareDecoder") || key == QStringLiteral("postprocessing") ? QStringLiteral("restart")
            : key == QStringLiteral("encoder") ? QStringLiteral("nextJob") : QStringLiteral("nextInput");
    }
    modes[QStringLiteral("rescan")] = QStringLiteral("restart");
    return modes;
}
QVariantMap PerformancePreferencesModel::supportNotes() const
{
    return {
        {QStringLiteral("hardwareDecoder"),tr("Uses the native hardware-decoding flag and device preference after restarting VLC. A device requires compatible hardware; normal software fallback remains available. Off preserves the saved device choice.")},
        {QStringLiteral("postprocessing"),tr("Uses the actual postproc video filter after restarting VLC. Other filters and inline options are preserved. For multiple postproc stages, select Off before choosing one quality.")},
        {QStringLiteral("decodingThreads"),tr("0 selects automatic threading. This build caps explicit HEVC threads at 32 and other video decoders at 16; hardware-only decoding uses one thread. Next input.")},
        {QStringLiteral("loopFilter"),tr("Native avcodec H.264 loop-filter policy for the next input. Other decoders may not use it.")},
        {QStringLiteral("dropLateFrames"),tr("Native late-video-frame policy for the next input.")},
        {QStringLiteral("priority"),tr("Live Windows process scheduling. Current class: %1. Base levels: -2 Idle, -1 Below normal, 0 Normal, 1 Above normal, 2 High. The base is used when real-time is off. Changes require exact Windows readback and a successful preference save.")
            .arg(m_values.value(QStringLiteral("actualPriority")).toString())},
        {QStringLiteral("encoder"),tr("Preferred encoder for the next automatic conversion job. Explicit profile encoders take precedence; an incompatible or missing preference falls back normally. Running jobs retain their captured preference.")},
        {QStringLiteral("decoder"),tr("Native decoder preference for the next input; available modules and media format determine selection.")},
        {QStringLiteral("clock"),tr("Native input-clock defaults for the next input. Jitter is in milliseconds; reference averaging scales with the input's caching delay. A zero average uses the native fallback.")},
        {QStringLiteral("rescan"),tr("Refresh the installed plugin cache in an owned process, preserving the current cache on failure. Restart VLC to use the new inventory.")},
        {QStringLiteral("commandLineOverride"),tr("This launch has a command-line override. Saved defaults require restarting without that override.")}
    };
}
bool PerformancePreferencesModel::save(const QVariantMap &changes)
{
    QString error;
    if (!CheckedPreferencesStore(m_intf).saveNative(changes, error)) return fail(error);
    refresh(); return true;
}
bool PerformancePreferencesModel::priority(const QString &key, const QVariant &input)
{
    vlc_process_priority_state before;
    if (vlc_process_priority_get(&before) != VLC_SUCCESS) return fail(tr("Windows process scheduling is unavailable."));
    qlonglong level = m_values.value(QStringLiteral("processPriority")).toLongLong();
    bool realtime = before.realtime;
    if (key == QStringLiteral("realTimePriority")) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("Choose On or Off."));
        realtime = input.toBool();
        if (level < -2 || level > 2) level = 0;
    } else if (!integer(input, -2, 2, level))
        return fail(tr("Enter a whole process-priority level from -2 to 2."));
    vlc_process_priority_state applied;
    if (vlc_process_priority_apply(int(level), realtime, &applied) != VLC_SUCCESS) {
        refresh();
        return fail(tr("Windows did not grant the requested scheduling class (error %1). Current class: %2.")
                    .arg(applied.system_error).arg(className(applied)));
    }
    QString error;
    if (!CheckedPreferencesStore(m_intf).saveNative({
            {QStringLiteral("win32-process-priority"), level},
            {QStringLiteral("win32-realtime-priority"), realtime}}, error)) {
        vlc_process_priority_state restored;
        const bool rollback = vlc_process_priority_apply(before.level, before.realtime, &restored) == VLC_SUCCESS;
        refresh();
        return fail(rollback ? error : error + tr(" Windows could not restore the prior scheduling class. Current class: %1.").arg(className(restored)));
    }
    refresh(); return true;
}
bool PerformancePreferencesModel::setValue(const QString &key, const QVariant &input)
{
    setError({}); setStatus({});
    if (busy()) return fail(tr("Wait for the plugin rescan to finish."));
    refresh();
    if (!m_capabilities.value(key).toBool()) return fail(tr("This Performance setting is unavailable in this build."));
    if (key == QStringLiteral("processPriority") || key == QStringLiteral("realTimePriority")) return priority(key,input);
    const Option *option = nullptr;
    for (const auto &candidate : options) if (key == QLatin1String(candidate.key)) { option = &candidate; break; }
    if (!option) return fail(tr("Unknown Performance setting."));
    const auto native = QString::fromLatin1(option->native);
    if (key == QStringLiteral("postprocessing")) {
        qlonglong quality;
        if (!integer(input,-1,6,quality) || quality == 0) return fail(tr("Choose Off or a supported post-processing quality."));
        const auto chain = postprocessing(int(quality));
        if (!chain.editable || (chain.multiple && quality != -1))
            return fail(tr("Select Off before replacing a custom post-processing chain."));
        return save({{QStringLiteral("postproc-q"), quality == -1 ? qlonglong(0) : quality},
                     {QStringLiteral("video-filter"),QString::fromUtf8(quality == -1 ? chain.removed : chain.enabled)}});
    }
    if (option->type == VLC_VAR_STRING) {
        if (input.typeId() != QMetaType::QString || !listed(m_choices.value(key).toList(),input))
            return fail(tr("Choose an available module or Automatic."));
        if (key == QStringLiteral("hardwareDecoder")) {
            QVariantMap changes{{QStringLiteral("hw-dec"), input.toString() != QStringLiteral("off")}};
            if (input.toString() != QStringLiteral("off")) changes[native] = input;
            return save(changes);
        }
        return save({{native,input}});
    }
    if (option->type == VLC_VAR_BOOL) {
        if (input.typeId() != QMetaType::Bool) return fail(tr("Choose On or Off."));
        return save({{native,input}});
    }
    qlonglong value;
    const qlonglong maximum = key == QStringLiteral("decodingThreads") ? 32
        : key == QStringLiteral("clockAverage") ? 1000 : key == QStringLiteral("clockJitter") ? 60000 : 4;
    const qlonglong minimum = key == QStringLiteral("clockSync") ? -1 : 0;
    if (!integer(input,minimum,maximum,value)) return fail(tr("Enter a whole number in the supported range."));
    if (m_choices.contains(key) && !listed(m_choices.value(key).toList(), QVariant(value)))
        return fail(tr("Choose one of the native supported values."));
    return save({{native,value}});
}
bool PerformancePreferencesModel::rescanPlugins()
{
    setError({}); setStatus({}); refresh();
    if (!m_capabilities.value(QStringLiteral("rescan")).toBool()) return fail(tr("Plugin rescanning is unavailable."));
    if (!m_scan->start()) return fail(m_scan->lastError());
    setStatus(tr("Rescanning plugins…")); return true;
}
void PerformancePreferencesModel::reload()
{
    if (busy()) { fail(tr("Wait for the plugin rescan to finish.")); return; }
    setError({}); setStatus({}); refresh();
}
