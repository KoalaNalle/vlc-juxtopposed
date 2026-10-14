// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_SUBTITLE_APPEARANCE_HPP
#define VLC_QT_SUBTITLE_APPEARANCE_HPP
#include "util/variables.hpp"
#include <vlc_player.h>

class SubtitleAppearance : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool embeddedFormat READ embeddedFormat WRITE setEmbeddedFormat NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString textColor READ textColor WRITE setTextColor NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString outlineColor READ outlineColor WRITE setOutlineColor NOTIFY stateChanged FINAL)
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY stateChanged FINAL)
    Q_PROPERTY(int sizeMode READ sizeMode WRITE setSizeMode NOTIFY stateChanged FINAL)
    Q_PROPERTY(float fontSize READ fontSize WRITE setFontSize NOTIFY stateChanged FINAL)
    Q_PROPERTY(float fontSizeMinimum READ fontSizeMinimum NOTIFY stateChanged FINAL)
    Q_PROPERTY(float fontSizeMaximum READ fontSizeMaximum NOTIFY stateChanged FINAL)
    Q_PROPERTY(float outlineSize READ outlineSize WRITE setOutlineSize NOTIFY stateChanged FINAL)
    Q_PROPERTY(float backgroundOpacity READ backgroundOpacity WRITE setBackgroundOpacity NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool durationAvailable READ durationAvailable NOTIFY stateChanged FINAL)
    Q_PROPERTY(float durationFactor READ durationFactor WRITE setDurationFactor NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString durationUnit READ durationUnit NOTIFY stateChanged FINAL)
    Q_PROPERTY(QString durationDescription READ durationDescription NOTIFY stateChanged FINAL)
public:
    explicit SubtitleAppearance(vlc_player_t *, QObject *parent = nullptr);
    ~SubtitleAppearance();
    void resetOutput(vout_thread_t *);
    bool available() const;
    QString statusText() const;
    bool embeddedFormat() const;
    QString fontFamily() const;
    QString textColor() const;
    QString outlineColor() const;
    int offset() const;
    int sizeMode() const;
    float fontSize() const;
    float fontSizeMinimum() const { return (sizeMode() ? 1.f : 0.1f) * scale(); }
    float fontSizeMaximum() const { return (sizeMode() ? 512.f : 50.f) * scale(); }
    float outlineSize() const;
    float backgroundOpacity() const;
    bool durationAvailable() const;
    float durationFactor() const;
    QString durationUnit() const;
    QString durationDescription() const;
    Q_INVOKABLE void refresh();
public slots:
    void setEmbeddedFormat(bool);
    void setFontFamily(const QString &);
    void setTextColor(const QString &);
    void setOutlineColor(const QString &);
    void setOffset(int);
    void setSizeMode(int);
    void setFontSize(float);
    void setOutlineSize(float);
    void setBackgroundOpacity(float);
    void setDurationFactor(float);
signals:
    void stateChanged();
private:
    void releaseOutput();
    void prepareStyle();
    bool custom() const;
    float scale() const;
    int durationMode() const;
    QString color(const char *, const char *) const;
    void setColor(const QString &, QVLCInteger &);
    using Output = vlc::vlc_shared_data_ptr<vout_thread_t, &vout_Hold, &vout_Release>;
    Output m_output;
    vlc_player_t *m_player;
    QVLCBool m_enabled, m_embedded, m_supported;
    QVLCString m_font;
    QVLCInteger m_color, m_outlineColor, m_mode, m_background, m_offset;
    QVLCFloat m_relative, m_pixels, m_relativeOutline, m_pixelOutline;
    QVLCInteger m_scale;
    QVLCString m_subFilters;
    QVLCFloat m_duration;
    QVLCInteger m_durationMode;
};
#endif
