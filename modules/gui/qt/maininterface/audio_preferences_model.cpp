// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "audio_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include "util/audio_defaults_policy.hpp"
#include "util/audio_windows_devices.hpp"
#include "../../../audio_filter/equalizer_presets.h"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_iso_lang.h>
#include <QCoreApplication>
#include <QLocale>
#include <climits>
#include <limits>

namespace {
using namespace AudioDefaultsPolicy;
QString tr(const char *text) { return QCoreApplication::translate("AudioPreferencesModel", text); }
QString nativeText(const char *key)
{
    if (config_GetType(key) != VLC_VAR_STRING) return {};
    char *raw = config_GetPsz(key); const auto text = QString::fromUtf8(raw ? raw : ""); free(raw); return text;
}
QVariant nativeValue(const char *key)
{
    switch (config_GetType(key)) {
    case VLC_VAR_BOOL: return bool(config_GetInt(key));
    case VLC_VAR_INTEGER: return qlonglong(config_GetInt(key));
    case VLC_VAR_FLOAT: {
        const float value=config_GetFloat(key);
        // Preserve malformed native profile bytes without sending NaN/Inf to
        // QML controls or making the startup fingerprint unequal to itself.
        return std::isfinite(value)?QVariant(double(value)):QVariant{};
    }
    case VLC_VAR_STRING: return nativeText(key);
    default: return {};
    }
}
const char *const nativeKeys[] = {"audio","aout","mmdevice-audio-device","mix-mode","qt-max-volume","volume-save",
    "mmdevice-volume","volume-step","audio-filter","norm-max-level","audio-replay-gain-mode","audio-replay-gain-preamp",
    "equalizer-preset","equalizer-bands","equalizer-preamp","equalizer-2pass","equalizer-vlcfreqs","pitch-shift",
    "audio-language","audio-desync","audio-visual","effect-list","mmdevice-passthrough","force-dolby-surround",
    "audio-resampler","soxr-resampler-quality","role","effect-width","effect-height"};
QVariantList effectiveBands();
QVariantMap nativeSnapshot()
{
    QVariantMap result;
    for (const char *key : nativeKeys) if (config_GetType(key)) result.insert(QString::fromLatin1(key),nativeValue(key));
    // Empty native bands plus a known preset and its explicit ten-band form
    // describe the same effective defaults. Materializing that form must not
    // leave a false restart warning after the original sound is restored.
    const auto bands=effectiveBands();
    if (bands.size()==EQZ_BANDS_MAX) {
        result.insert(QStringLiteral("equalizer-bands"),bands);
        result.remove(QStringLiteral("equalizer-preset"));
    }
    return result;
}
QVariantMap choice(const QString &name, const QVariant &value)
{ return {{QStringLiteral("name"), name},{QStringLiteral("value"),value}}; }
QVariantList nativeStringChoices(const char *key)
{
    QVariantList result; char **values = nullptr, **labels = nullptr;
    const ssize_t count = config_GetPszChoices(key,&values,&labels);
    for (ssize_t i = 0; i < count; ++i) {
        if (values[i]) result.push_back(choice(QString::fromUtf8(labels[i] ? labels[i] : values[i]),QString::fromUtf8(values[i])));
        free(values[i]); free(labels[i]);
    }
    free(values); free(labels); return result;
}
QVariantList integerChoices(std::initializer_list<const char *> labels)
{
    QVariantList rows; int value = 0;
    for (const char *label : labels) rows.push_back(choice(tr(label),value++));
    return rows;
}
bool listed(const QVariantList &rows, const QVariant &value)
{ for (const auto &row : rows) if (row.toMap().value(QStringLiteral("value")) == value) return true; return false; }
bool managedOutput()
{ const auto aout = nativeText("aout"); return aout.isEmpty() || aout == QLatin1String("any") || aout == QLatin1String("mmdevice"); }
bool roleConsumerAvailable()
{
    // MMDevice always requests eConsole; changing the core role flag does not
    // select another Windows endpoint role. Only expose a write for an actual
    // explicitly selected output whose native implementation consumes it.
    const auto output = nativeText("aout");
    return (output == QLatin1String("pulse") && module_exists("pulse"))
        || (output == QLatin1String("pipewire") && module_exists("pipewire"));
}
int presetIndex(const QString &name)
{ for (int i = 0; i < NB_PRESETS; ++i) if (name == QLatin1String(preset_list[i])) return i; return -1; }
QVariantList effectiveBands()
{
    const auto raw = nativeText("equalizer-bands");
    const auto pieces = raw.split(QLatin1Char(' '),Qt::SkipEmptyParts);
    const int preset = presetIndex(nativeText("equalizer-preset"));
    QVariantList result;
    if (pieces.isEmpty()) {
        if (preset < 0) return {};
        for (const float gain : eqz_preset_10b[preset].f_amp) result.push_back(double(gain));
        return result;
    }
    if (pieces.size() != EQZ_BANDS_MAX) return {};
    for (const auto &piece : pieces) {
        bool valid = false; const float gain = QLocale::c().toFloat(piece,&valid);
        if (!valid || !std::isfinite(gain) || gain < -20 || gain > 20) return {};
        result.push_back(double(gain));
    }
    return result;
}
bool encodeBands(const QVariant &value, QString &text)
{
    if (value.typeId() != QMetaType::QVariantList) return false;
    const auto values = value.toList(); if (values.size() != EQZ_BANDS_MAX) return false;
    QStringList pieces;
    for (const auto &entry : values) {
        double gain; if (!numeric(entry,-20,20,gain)) return false;
        // String option accepts decimal text; no hidden float-settings alias.
        pieces.push_back(QString::number(double(float(gain)),'g',9));
    }
    text = pieces.join(QLatin1Char(' ')); return true;
}
struct Filter { const char *key; const char *module; };
constexpr Filter filters[] = {{"normalizeLoudness","normvol"},{"equalizerEnabled","equalizer"},
    {"stereoWidening","stereo_widen"},{"compressor","compressor"},{"chorusFlanger","chorus_flanger"},{"karaoke","karaoke"}};
bool validLanguage(const QString &input)
{
    if (!validText(input,256)) return false;
    if (input.isEmpty()) return true;
    const auto entries = input.split(QLatin1Char(','));
    if (entries.size() > 16) return false;
    for (const auto &entry : entries) {
        const auto code = entry.toUtf8();
        if (entry != QLatin1String("any") && entry != QLatin1String("none")
            && ((entry.size() != 2 && entry.size() != 3) || !vlc_find_iso639(code.constData(),false))) return false;
    }
    return true;
}
}

AudioPreferencesModel::AudioPreferencesModel(qt_intf_t *intf,QObject *parent)
    : AudioPreferencesModel(intf,Consumers{},parent) {}
AudioPreferencesModel::AudioPreferencesModel(qt_intf_t *intf,Consumers consumers,QObject *parent)
    : QObject(parent),m_intf(intf),m_consumers(consumers)
{ refresh(); m_startupNative=nativeSnapshot(); updateRestart(); }
void AudioPreferencesModel::setError(const QString &text)
{ if (m_error == text) return; m_error=text; emit lastErrorChanged(); }
bool AudioPreferencesModel::fail(const QString &text) { setError(text); return false; }
void AudioPreferencesModel::updateRestart()
{
    const bool needed = nativeSnapshot() != m_startupNative;
    if (needed == m_restart) return; m_restart=needed; emit restartRequiredChanged();
}
void AudioPreferencesModel::refreshDevices()
{
    QVariantList devices;
    m_deviceEnumerationSucceeded = m_intf && m_consumers.windowsDevices && managedOutput()
        && module_exists("mmdevice") && config_GetType("aout") == VLC_VAR_STRING
        && config_GetType("mmdevice-audio-device") == VLC_VAR_STRING
        && enumerateAudioWindowsDevices(devices);
    if (devices != m_devices) { m_devices=devices; emit devicesChanged(); }
}
QVariantList AudioPreferencesModel::equalizerFrequencies() const
{
    QVariantList rows;
    const bool vlc = config_GetType("equalizer-vlcfreqs") == VLC_VAR_BOOL && config_GetInt("equalizer-vlcfreqs");
    const float *table = vlc ? f_vlc_frequency_table_10b : f_iso_frequency_table_10b;
    for (int i = 0; i < EQZ_BANDS_MAX; ++i) rows.push_back(double(table[i]));
    return rows;
}
QVariantMap AudioPreferencesModel::ranges() const
{
    auto range=[](double minimum,double maximum) { return QVariantMap{{QStringLiteral("minimum"),minimum},{QStringLiteral("maximum"),maximum}}; };
    return {{QStringLiteral("maximumVolume"),range(60,300)},
        {QStringLiteral("startupVolume"),range(0,125)},
        {QStringLiteral("volumeStep"),range(100./256.,100)},
        {QStringLiteral("normalizationTarget"),range(0.000001,double(std::numeric_limits<float>::max()))},
        {QStringLiteral("replayGainPreamp"),range(-20,20)},
        {QStringLiteral("equalizerPreamp"),range(-20,20)},
        {QStringLiteral("equalizerBands"),range(-20,20)},
        {QStringLiteral("pitchShift"),range(-12,12)},
        {QStringLiteral("audioDelayMs"),range(INT_MIN,INT_MAX)}};
}
void AudioPreferencesModel::refresh()
{
    using namespace AudioDefaultsPolicy;
    refreshDevices();
    const auto frequencies=equalizerFrequencies();
    if (frequencies!=m_frequencies) { m_frequencies=frequencies; emit equalizerFrequenciesChanged(); }
    QList<ChainLink> chain; const bool chainValid = parseChain(nativeText("audio-filter"),chain);
    const bool service = m_intf != nullptr;
    const auto native = nativeSnapshot();
    auto read = [&native](const char *key) { return native.value(QString::fromLatin1(key)); };
    auto has = [service](const char *name,int type) { return service && config_GetType(name) == type; };
    const bool eq = module_exists("equalizer");
    const bool eqParameters = chainValid && !hasOptions(chain,QStringLiteral("equalizer"));
    const bool windows = service && m_consumers.windowsDevices && managedOutput() && module_exists("mmdevice")
        && has("aout",VLC_VAR_STRING);
    const auto resampler = nativeText("audio-resampler");
    const bool managedResampler = resampler.isEmpty() || resampler == QLatin1String("any") || resampler == QLatin1String("soxr");
    QString visiblePreset=nativeText("equalizer-preset");
    const int selectedPreset=presetIndex(visiblePreset);
    if (selectedPreset>=0) {
        const auto bands=effectiveBands(); bool matches=bands.size()==EQZ_BANDS_MAX
            && config_GetFloat("equalizer-preamp")==eqz_preset_10b[selectedPreset].f_preamp;
        for (int i=0;matches&&i<EQZ_BANDS_MAX;++i) matches=bands[i].toFloat()==eqz_preset_10b[selectedPreset].f_amp[i];
        if (!matches) visiblePreset.clear(); // Custom gains must not show an untrue named preset.
    }
    QVariantMap values {
        {QStringLiteral("audioEnabled"),read("audio")}, {QStringLiteral("outputDevice"),read("mmdevice-audio-device")},
        {QStringLiteral("speakerSetup"),read("mix-mode")}, {QStringLiteral("maximumVolume"),read("qt-max-volume")},
        {QStringLiteral("rememberVolume"),read("volume-save")},
        {QStringLiteral("startupVolume"),read("mmdevice-volume").isValid() ? QVariant(read("mmdevice-volume").toDouble()*100.) : QVariant{}},
        {QStringLiteral("volumeStep"),read("volume-step").isValid() ? QVariant(read("volume-step").toDouble()*100./256.) : QVariant{}},
        {QStringLiteral("normalizationTarget"),read("norm-max-level")},
        {QStringLiteral("nightMode"),QVariant{}},
        {QStringLiteral("replayGainMode"),read("audio-replay-gain-mode")}, {QStringLiteral("replayGainPreamp"),read("audio-replay-gain-preamp")},
        {QStringLiteral("equalizerPreset"),visiblePreset}, {QStringLiteral("equalizerTwoPass"),read("equalizer-2pass")},
        {QStringLiteral("equalizerPreamp"),read("equalizer-preamp")}, {QStringLiteral("equalizerBands"),effectiveBands()},
        {QStringLiteral("spatialAudio"),QVariant{}}, {QStringLiteral("spatialAudioMode"),QVariant{}},
        {QStringLiteral("pitchShift"),read("pitch-shift")}, {QStringLiteral("preferredLanguage"),read("audio-language")},
        {QStringLiteral("audioDelayMs"),read("audio-desync")},
        {QStringLiteral("visualizer"),nativeText("audio-visual") == QLatin1String("visual") ? nativeText("effect-list")
            : nativeText("audio-visual").isEmpty() ? QStringLiteral("none") : nativeText("audio-visual")},
        {QStringLiteral("passthrough"),has("mmdevice-passthrough",VLC_VAR_INTEGER)
            && config_GetInt("mmdevice-passthrough")>=0 && config_GetInt("mmdevice-passthrough")<=2
            ? QVariant(config_GetInt("mmdevice-passthrough")>0) : QVariant{}},
        {QStringLiteral("detectDolby"),read("force-dolby-surround")},
        {QStringLiteral("resamplingQuality"),resampler==QLatin1String("soxr")?read("soxr-resampler-quality"):QVariant{}},
        {QStringLiteral("mediaRole"),read("role")},
        {QStringLiteral("visualizationSize"),QVariantMap{{QStringLiteral("width"),read("effect-width")},{QStringLiteral("height"),read("effect-height")}}}
    };
    QVariantMap caps {
        {QStringLiteral("audioEnabled"),has("audio",VLC_VAR_BOOL)}, {QStringLiteral("outputDevice"),m_deviceEnumerationSucceeded},
        {QStringLiteral("speakerSetup"),has("mix-mode",VLC_VAR_INTEGER)}, {QStringLiteral("maximumVolume"),has("qt-max-volume",VLC_VAR_INTEGER)},
        {QStringLiteral("rememberVolume"),has("volume-save",VLC_VAR_BOOL)},
        {QStringLiteral("startupVolume"),windows && has("aout",VLC_VAR_STRING) && has("mmdevice-volume",VLC_VAR_FLOAT)},
        {QStringLiteral("volumeStep"),m_consumers.authoredVolumeStep && has("volume-step",VLC_VAR_FLOAT)},
        {QStringLiteral("normalizationTarget"),module_exists("normvol") && has("norm-max-level",VLC_VAR_FLOAT)
            && chainValid && !hasOptions(chain,QStringLiteral("normvol"))},
        {QStringLiteral("nightMode"),false}, {QStringLiteral("replayGainMode"),has("audio-replay-gain-mode",VLC_VAR_STRING)},
        {QStringLiteral("replayGainPreamp"),has("audio-replay-gain-preamp",VLC_VAR_FLOAT)},
        {QStringLiteral("equalizerPreset"),eq && eqParameters && has("equalizer-preset",VLC_VAR_STRING)
            && has("equalizer-bands",VLC_VAR_STRING) && has("equalizer-preamp",VLC_VAR_FLOAT)},
        {QStringLiteral("equalizerTwoPass"),eq && eqParameters && has("equalizer-2pass",VLC_VAR_BOOL)},
        {QStringLiteral("equalizerPreamp"),eq && eqParameters && has("equalizer-preamp",VLC_VAR_FLOAT)
            && effectiveBands().size()==EQZ_BANDS_MAX && has("equalizer-bands",VLC_VAR_STRING)
            && has("equalizer-preset",VLC_VAR_STRING)},
        {QStringLiteral("equalizerBands"),eq && eqParameters && has("equalizer-bands",VLC_VAR_STRING) && has("equalizer-preset",VLC_VAR_STRING)},
        {QStringLiteral("spatialAudio"),false}, {QStringLiteral("spatialAudioMode"),false},
        {QStringLiteral("pitchShift"),has("pitch-shift",VLC_VAR_FLOAT) && module_exists("scaletempo_pitch")
            && has("audio-filter",VLC_VAR_STRING) && chainValid && !hasOptions(chain,QStringLiteral("scaletempo_pitch"))},
        {QStringLiteral("preferredLanguage"),has("audio-language",VLC_VAR_STRING)}, {QStringLiteral("audioDelayMs"),has("audio-desync",VLC_VAR_INTEGER)},
        {QStringLiteral("visualizer"),has("audio-visual",VLC_VAR_STRING)},
        {QStringLiteral("passthrough"),windows && has("mmdevice-passthrough",VLC_VAR_INTEGER)},
        {QStringLiteral("detectDolby"),has("force-dolby-surround",VLC_VAR_INTEGER)},
        {QStringLiteral("resamplingQuality"),managedResampler && has("soxr-resampler-quality",VLC_VAR_INTEGER)
            && has("audio-resampler",VLC_VAR_STRING) && module_exists("soxr")},
        {QStringLiteral("mediaRole"),has("role",VLC_VAR_STRING) && roleConsumerAvailable()}, {QStringLiteral("visualizationSize"),false}
    };
    for (const auto &filter : filters) {
        const auto name = QString::fromLatin1(filter.module), key=QString::fromLatin1(filter.key);
        values.insert(key,chainValid ? QVariant(contains(chain,name)) : QVariant{});
        bool capable = service && chainValid && module_exists(filter.module) && has("audio-filter",VLC_VAR_STRING);
        if (key == QLatin1String("equalizerEnabled") && (effectiveBands().size()!=EQZ_BANDS_MAX
            || !has("equalizer-bands",VLC_VAR_STRING))) capable=false;
        caps.insert(key,capable);
    }
    QVariantList presets;
    for (int i=0;i<NB_PRESETS;++i) presets.push_back(choice(tr(preset_list_text[i]),QString::fromLatin1(preset_list[i])));
    QVariantList languages{choice(tr("Automatic"),QString{}),choice(tr("Any language"),QStringLiteral("any")),choice(tr("No audio track"),QStringLiteral("none"))};
    for (char a='a';a<='z';++a) for (char b='a';b<='z';++b) {
        const char code[]={a,b,'\0'}; const auto *language=vlc_find_iso639(code,false);
        if (language) languages.push_back(choice(QString::fromUtf8(language->psz_eng_name),QString::fromLatin1(code)));
    }
    QVariantList visualizers{choice(tr("Off"),QStringLiteral("none"))};
    if (module_exists("visual") && has("effect-list",VLC_VAR_STRING)) for (const auto *name : {"scope","spectrum","spectrometer","vuMeter"})
        visualizers.push_back(choice(QString::fromLatin1(name),QString::fromLatin1(name)));
    for (const auto &row : nativeStringChoices("audio-visual")) {
        const auto id=row.toMap().value(QStringLiteral("value")).toString();
        if (!id.isEmpty() && id!=QLatin1String("none") && id!=QLatin1String("visual") && module_exists(qtu(id))) visualizers.push_back(row);
    }
    auto speakers=integerChoices({"Automatic","Stereo","Binaural","4.0","5.1","7.1"});
    // Keep genuine native enum numbers after removing an unavailable choice.
    if (!module_exists("spatialaudio")) speakers.removeAt(2);
    QVariantMap options {
        {QStringLiteral("speakerSetup"),speakers},
        {QStringLiteral("equalizerPreset"),presets}, {QStringLiteral("replayGainMode"),nativeStringChoices("audio-replay-gain-mode")},
        {QStringLiteral("preferredLanguage"),languages},{QStringLiteral("visualizer"),visualizers},
        {QStringLiteral("detectDolby"),integerChoices({"Automatic","On","Off"})},
        {QStringLiteral("resamplingQuality"),integerChoices({"Quick cubic interpolation","Low 16-bit","Balanced (medium 16-bit)","High quality","Very high quality"})},
        {QStringLiteral("mediaRole"),nativeStringChoices("role")}
    };
    if (values!=m_values) { m_values=values; emit valuesChanged(); }
    if (caps!=m_capabilities) { m_capabilities=caps; emit capabilitiesChanged(); }
    if (options!=m_options) { m_options=options; emit optionsChanged(); }
}
void AudioPreferencesModel::reload() { setError({}); refresh(); updateRestart(); }

bool AudioPreferencesModel::setValue(const QString &key,const QVariant &value)
{
    using namespace AudioDefaultsPolicy;
    refresh();
    if (!m_capabilities.contains(key) || !m_capabilities.value(key).toBool())
        return fail(tr("This audio setting is unavailable in this build."));
    QVariantMap changes;
    QList<ChainLink> chain; const bool chainValid=parseChain(nativeText("audio-filter"),chain);
    for (const auto &filter : filters) if (key==QLatin1String(filter.key)) {
        if (value.typeId()!=QMetaType::Bool || !chainValid) return fail(tr("Choose On or Off for this setting."));
        if (contains(chain,QString::fromLatin1(filter.module))==value.toBool()) { setError({}); return true; }
        changes.insert(QStringLiteral("audio-filter"),changeFilter(chain,QString::fromLatin1(filter.module),value.toBool()));
        if (key==QLatin1String("equalizerEnabled") && value.toBool()) {
            QString bands; if (!encodeBands(effectiveBands(),bands)) return fail(tr("Choose a valid equalizer preset first."));
            changes.insert(QStringLiteral("equalizer-bands"),bands);
        }
    }
    int integerValue=0; double number=0;
    if (key==QLatin1String("audioEnabled") || key==QLatin1String("rememberVolume") || key==QLatin1String("equalizerTwoPass")) {
        if (value.typeId()!=QMetaType::Bool) return fail(tr("Choose On or Off for this setting."));
        changes.insert(key==QLatin1String("audioEnabled")?QStringLiteral("audio"):
            key==QLatin1String("rememberVolume")?QStringLiteral("volume-save"):QStringLiteral("equalizer-2pass"),value);
    } else if (key==QLatin1String("speakerSetup") || key==QLatin1String("detectDolby") || key==QLatin1String("maximumVolume")
               || key==QLatin1String("audioDelayMs") || key==QLatin1String("resamplingQuality")) {
        const int lo=key==QLatin1String("maximumVolume")?60:key==QLatin1String("audioDelayMs")?INT_MIN:0;
        const int hi=key==QLatin1String("speakerSetup")?5:key==QLatin1String("detectDolby")?2:
            key==QLatin1String("maximumVolume")?300:key==QLatin1String("audioDelayMs")?INT_MAX:4;
        if (!integer(value,lo,hi,integerValue)) return fail(tr("Choose a supported whole-number value."));
        if (key==QLatin1String("speakerSetup") && !listed(m_options.value(key).toList(),integerValue))
            return fail(tr("This speaker setup is unavailable in this build."));
        const QString option=key==QLatin1String("speakerSetup")?QStringLiteral("mix-mode"):
            key==QLatin1String("detectDolby")?QStringLiteral("force-dolby-surround"):
            key==QLatin1String("maximumVolume")?QStringLiteral("qt-max-volume"):
            key==QLatin1String("audioDelayMs")?QStringLiteral("audio-desync"):QStringLiteral("soxr-resampler-quality");
        changes.insert(option,integerValue);
        if (key==QLatin1String("resamplingQuality")) changes.insert(QStringLiteral("audio-resampler"),QStringLiteral("soxr"));
    } else if (key==QLatin1String("volumeStep") || key==QLatin1String("startupVolume") || key==QLatin1String("normalizationTarget")
               || key==QLatin1String("replayGainPreamp") || key==QLatin1String("equalizerPreamp") || key==QLatin1String("pitchShift")) {
        const double lo=key==QLatin1String("volumeStep")?100./256.:key==QLatin1String("startupVolume")?0:
            key==QLatin1String("normalizationTarget")?0.000001:key==QLatin1String("pitchShift")?-12:-20;
        const double hi=key==QLatin1String("volumeStep")?100:key==QLatin1String("startupVolume")?125:
            key==QLatin1String("normalizationTarget")?double(std::numeric_limits<float>::max()):key==QLatin1String("pitchShift")?12:20;
        // Bounds for these fields are native floats too. Accept the exact
        // float minimum and its double readback without widening integer ranges.
        if (!numeric(value,double(float(lo)),double(float(hi)),number)) return fail(tr("Enter a number within this setting's supported range."));
        const QString option=key==QLatin1String("volumeStep")?QStringLiteral("volume-step"):
            key==QLatin1String("startupVolume")?QStringLiteral("mmdevice-volume"):
            key==QLatin1String("normalizationTarget")?QStringLiteral("norm-max-level"):
            key==QLatin1String("replayGainPreamp")?QStringLiteral("audio-replay-gain-preamp"):
            key==QLatin1String("equalizerPreamp")?QStringLiteral("equalizer-preamp"):QStringLiteral("pitch-shift");
        changes.insert(option,key==QLatin1String("volumeStep")?number*256./100.:key==QLatin1String("startupVolume")?number/100.:number);
        const auto previousNumber=nativeValue(qtu(option));
        if ((key==QLatin1String("startupVolume") || key==QLatin1String("equalizerPreamp"))
            && previousNumber.isValid() && previousNumber.toFloat()==changes.value(option).toFloat()) { setError({}); return true; }
        if (key==QLatin1String("startupVolume")) changes.insert(QStringLiteral("aout"),QStringLiteral("mmdevice"));
        if (key==QLatin1String("equalizerPreamp")) {
            QString bands; if (!encodeBands(effectiveBands(),bands)) return fail(tr("Choose a valid equalizer preset first."));
            changes.insert(QStringLiteral("equalizer-bands"),bands); changes.insert(QStringLiteral("equalizer-preset"),QString{});
        }
        if (key==QLatin1String("pitchShift")) changes.insert(QStringLiteral("audio-filter"),changeFilter(chain,QStringLiteral("scaletempo_pitch"),number!=0));
    } else if (key==QLatin1String("equalizerBands")) {
        QString bands; if (!encodeBands(value,bands)) return fail(tr("Provide exactly ten equalizer gains between -20 and 20 dB."));
        changes.insert(QStringLiteral("equalizer-bands"),bands); changes.insert(QStringLiteral("equalizer-preset"),QString{});
    } else if (key==QLatin1String("equalizerPreset")) {
        if (value.typeId()!=QMetaType::QString) return fail(tr("Choose a supported equalizer preset."));
        const int preset=presetIndex(value.toString()); if (preset<0) return fail(tr("Choose a supported equalizer preset."));
        QVariantList gains; for (const float gain : eqz_preset_10b[preset].f_amp) gains.push_back(double(gain));
        QString bands; if (!encodeBands(gains,bands)) return fail(tr("The equalizer preset is unavailable."));
        changes={{QStringLiteral("equalizer-preset"),value},{QStringLiteral("equalizer-bands"),bands},
                 {QStringLiteral("equalizer-preamp"),double(eqz_preset_10b[preset].f_preamp)}};
    } else if (key==QLatin1String("passthrough")) {
        if (value.typeId()!=QMetaType::Bool) return fail(tr("Choose On or Off for this setting."));
        if (m_values.value(key).isValid() && m_values.value(key).toBool()==value.toBool()) { setError({}); return true; }
        const auto previous=config_GetInt("mmdevice-passthrough");
        changes={{QStringLiteral("aout"),QStringLiteral("mmdevice")},
                 {QStringLiteral("mmdevice-passthrough"),value.toBool()?(previous==1?1:2):0}};
    } else if (key==QLatin1String("outputDevice") || key==QLatin1String("replayGainMode") || key==QLatin1String("preferredLanguage")
               || key==QLatin1String("visualizer") || key==QLatin1String("mediaRole")) {
        if (value.typeId()!=QMetaType::QString || !validText(value.toString())) return fail(tr("Choose a supported text value."));
        const auto text=value.toString();
        if (key==QLatin1String("outputDevice")) {
            if (!listed(m_devices,text)) return fail(tr("This output device is no longer available. Refresh the choices."));
            if (nativeText("mmdevice-audio-device")==text) { setError({}); return true; }
            changes={{QStringLiteral("aout"),QStringLiteral("mmdevice")},{QStringLiteral("mmdevice-audio-device"),text}};
        } else if (key==QLatin1String("preferredLanguage")) {
            if (!validLanguage(text)) return fail(tr("Use supported language codes separated by commas."));
            changes.insert(QStringLiteral("audio-language"),text);
        } else {
            if (!listed(m_options.value(key).toList(),text)) return fail(tr("Choose a supported option."));
            if (key==QLatin1String("visualizer")) {
                const bool builtin=text==QLatin1String("scope") || text==QLatin1String("spectrum") || text==QLatin1String("spectrometer") || text==QLatin1String("vuMeter");
                changes.insert(QStringLiteral("audio-visual"),builtin?QStringLiteral("visual"):text);
                if (builtin) changes.insert(QStringLiteral("effect-list"),text);
            } else changes.insert(key==QLatin1String("replayGainMode")?QStringLiteral("audio-replay-gain-mode"):QStringLiteral("role"),text);
        }
    }
    if (changes.isEmpty()) return fail(tr("This audio setting has no supported native operation."));
    bool unchanged=true;
    for (auto it=changes.cbegin();it!=changes.cend();++it) {
        const auto name=it.key().toUtf8();
        const auto before=nativeValue(name.constData());
        unchanged &= before.isValid() && (config_GetType(name.constData())==VLC_VAR_FLOAT
            ? before.toFloat()==it.value().toFloat() : before==it.value());
    }
    if (unchanged) { setError({}); return true; }
    QString error; if (!CheckedPreferencesStore(m_intf).saveNative(changes,error)) { refresh(); updateRestart(); return fail(error); }
    setError({}); refresh(); updateRestart(); return true;
}
