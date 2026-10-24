// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "audio_equalizer.hpp"
#include "../../../audio_filter/equalizer_presets.h"
#include <vlc_modules.h>
#include <cmath>

static QString outputString(audio_output_t *output, const char *name)
{
    if (!output) return {};
    const auto text = vlc::wrap_cptr(var_GetString(output, name));
    return QString::fromUtf8(text.get());
}

AudioEqualizer::AudioEqualizer(vlc_player_t *player, QObject *parent)
    : QObject(parent), m_player(player)
    , m_filters((audio_output_t *)nullptr, "audio-filter")
    , m_bands((audio_output_t *)nullptr, "equalizer-bands")
    , m_preset((audio_output_t *)nullptr, "equalizer-preset")
    , m_preamp((audio_output_t *)nullptr, "equalizer-preamp")
    , m_twoPass((audio_output_t *)nullptr, "equalizer-2pass")
    , m_vlcFrequencies((audio_output_t *)nullptr, "equalizer-vlcfreqs")
{
    connect(&m_filters, &QVLCString::valueChanged, this, &AudioEqualizer::stateChanged);
    connect(&m_bands, &QVLCString::valueChanged, this, &AudioEqualizer::stateChanged);
    connect(&m_preset, &QVLCString::valueChanged, this, &AudioEqualizer::stateChanged);
    connect(&m_preamp, &QVLCFloat::valueChanged, this, &AudioEqualizer::stateChanged);
    connect(&m_twoPass, &QVLCBool::valueChanged, this, &AudioEqualizer::stateChanged);
    connect(&m_vlcFrequencies, &QVLCBool::valueChanged, this, &AudioEqualizer::stateChanged);
}

AudioEqualizer::~AudioEqualizer() { releaseOutput(); }

void AudioEqualizer::releaseOutput()
{
    m_filters.clearObject();
    m_bands.clearObject();
    m_preset.clearObject();
    m_preamp.clearObject();
    m_twoPass.clearObject();
    m_vlcFrequencies.clearObject();
    if (m_output)
        for (const char *name : {"equalizer-bands", "equalizer-preset", "equalizer-preamp",
                                 "equalizer-2pass", "equalizer-vlcfreqs"})
            var_Destroy(m_output.get(), name);
    m_output.reset();
}

void AudioEqualizer::refresh()
{
    Output output;
    if (m_player)
    {
        vlc_player_Lock(m_player);
        output.reset(vlc_player_aout_Hold(m_player), false);
        vlc_player_Unlock(m_player);
    }
    if (output.get() == m_output.get())
        return;
    releaseOutput();
    m_output = std::move(output);
    if (m_output)
    {
        // Keep one owned reference while controls observe the variables. The
        // filter may add/remove its own references as it is enabled or rebuilt.
        var_Create(m_output.get(), "equalizer-bands", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
        var_Create(m_output.get(), "equalizer-preset", VLC_VAR_STRING | VLC_VAR_DOINHERIT);
        var_Create(m_output.get(), "equalizer-preamp", VLC_VAR_FLOAT | VLC_VAR_DOINHERIT);
        var_Create(m_output.get(), "equalizer-2pass", VLC_VAR_BOOL | VLC_VAR_DOINHERIT);
        var_Create(m_output.get(), "equalizer-vlcfreqs", VLC_VAR_BOOL | VLC_VAR_DOINHERIT);
        m_filters.resetObject(m_output.get());
        m_bands.resetObject(m_output.get());
        m_preset.resetObject(m_output.get());
        m_preamp.resetObject(m_output.get());
        m_twoPass.resetObject(m_output.get());
        m_vlcFrequencies.resetObject(m_output.get());
    }
    emit stateChanged();
}

bool AudioEqualizer::available() const { return m_output && module_exists("equalizer"); }
bool AudioEqualizer::enabled() const
{
    return outputString(m_output.get(), "audio-filter").split(':', Qt::SkipEmptyParts).contains("equalizer");
}
bool AudioEqualizer::twoPass() const { return m_output && var_GetBool(m_output.get(), "equalizer-2pass"); }
float AudioEqualizer::preamp() const { return m_output ? var_GetFloat(m_output.get(), "equalizer-preamp") : 0.f; }
int AudioEqualizer::presetIndex() const
{
    const QString name = outputString(m_output.get(), "equalizer-preset");
    for (int i = 0; i < NB_PRESETS; ++i)
        if (name == QLatin1String(preset_list[i])) return i;
    return -1;
}

QVariantList AudioEqualizer::bands() const
{
    QVariantList values;
    const auto parts = outputString(m_output.get(), "equalizer-bands").split(' ', Qt::SkipEmptyParts);
    const int preset = presetIndex();
    for (int i = 0; i < EQZ_BANDS_MAX; ++i)
    {
        bool valid = false;
        const float gain = i < parts.size() ? parts[i].toFloat(&valid) : 0.f;
        values.push_back(valid && std::isfinite(gain) ? qBound(-20.f, gain, 20.f)
                         : preset >= 0 ? eqz_preset_10b[preset].f_amp[i] : 0.f);
    }
    return values;
}

QVariantList AudioEqualizer::frequencies() const
{
    QVariantList values;
    const auto *table = m_output && var_GetBool(m_output.get(), "equalizer-vlcfreqs") ? f_vlc_frequency_table_10b
                                                  : f_iso_frequency_table_10b;
    for (int i = 0; i < EQZ_BANDS_MAX; ++i) values.push_back(table[i]);
    return values;
}
QStringList AudioEqualizer::presets() const
{
    QStringList values;
    for (const char *name : preset_list_text) values.push_back(qtr(name));
    return values;
}

void AudioEqualizer::setEnabled(bool enable)
{
    refresh();
    if (!available() || enabled() == enable) return;
    vlc_player_Lock(m_player);
    vlc_player_aout_EnableFilter(m_player, "equalizer", enable);
    vlc_player_Unlock(m_player);
}
void AudioEqualizer::setTwoPass(bool value)
{
    if (available()) m_twoPass.setValue(value);
}
void AudioEqualizer::setPreamp(float gain)
{
    if (available() && std::isfinite(gain)) m_preamp.setValue(qBound(-20.f, gain, 20.f));
}
void AudioEqualizer::setBand(int index, float gain)
{
    if (!available() || index < 0 || index >= EQZ_BANDS_MAX || !std::isfinite(gain)) return;
    auto values = bands();
    values[index] = qBound(-20.f, gain, 20.f);
    QStringList text;
    for (const auto &value : values) text.push_back(QString::number(value.toFloat(), 'f', 1));
    m_bands.setValue(text.join(' '));
}
void AudioEqualizer::setPreset(int index)
{
    if (!available() || index < 0 || index >= NB_PRESETS) return;
    QStringList text;
    for (const float gain : eqz_preset_10b[index].f_amp)
        text.push_back(QString::number(gain, 'f', 1));
    // Preset callbacks can also update bands/preamp. Explicit values are needed
    // when the filter is disabled and its callbacks are not installed.
    m_preset.setValue(QLatin1String(preset_list[index]));
    m_bands.setValue(text.join(' '));
    m_preamp.setValue(eqz_preset_10b[index].f_preamp);
}
