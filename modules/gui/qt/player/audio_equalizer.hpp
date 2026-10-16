// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_AUDIO_EQUALIZER_HPP
#define VLC_QT_AUDIO_EQUALIZER_HPP

#include "util/variables.hpp"
#include <vlc_player.h>
#include <QVariantList>
#include <QStringList>

class AudioEqualizer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool twoPass READ twoPass WRITE setTwoPass NOTIFY stateChanged FINAL)
    Q_PROPERTY(float preamp READ preamp WRITE setPreamp NOTIFY stateChanged FINAL)
    Q_PROPERTY(QVariantList bands READ bands NOTIFY stateChanged FINAL)
    Q_PROPERTY(QVariantList frequencies READ frequencies NOTIFY stateChanged FINAL)
    Q_PROPERTY(QStringList presets READ presets CONSTANT FINAL)
    Q_PROPERTY(int presetIndex READ presetIndex NOTIFY stateChanged FINAL)
public:
    explicit AudioEqualizer(vlc_player_t *, QObject *parent = nullptr);
    ~AudioEqualizer();
    bool available() const;
    bool enabled() const;
    bool twoPass() const;
    float preamp() const;
    QVariantList bands() const;
    QVariantList frequencies() const;
    QStringList presets() const;
    int presetIndex() const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setBand(int index, float gain);
    Q_INVOKABLE void setPreset(int index);
public slots:
    void setEnabled(bool);
    void setTwoPass(bool);
    void setPreamp(float);
signals:
    void stateChanged();
private:
    void releaseOutput();
    vlc_player_t *m_player;
    using Output = vlc::vlc_shared_data_ptr<audio_output_t, &aout_Hold, &aout_Release>;
    Output m_output;
    QVLCString m_filters;
    QVLCString m_bands;
    QVLCString m_preset;
    QVLCFloat m_preamp;
    QVLCBool m_twoPass;
    QVLCBool m_vlcFrequencies;
};
#endif
