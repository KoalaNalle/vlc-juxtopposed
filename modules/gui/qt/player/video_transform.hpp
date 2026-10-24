// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifndef VLC_QT_VIDEO_TRANSFORM_HPP
#define VLC_QT_VIDEO_TRANSFORM_HPP

#include "util/variables.hpp"

class VideoTransform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY stateChanged FINAL)
    Q_PROPERTY(int flip READ flip WRITE setFlip NOTIFY stateChanged FINAL)
    Q_PROPERTY(float rotation READ rotation WRITE setRotation NOTIFY stateChanged FINAL)
public:
    explicit VideoTransform(QObject *parent = nullptr);
    ~VideoTransform();
    void resetOutput(vout_thread_t *);
    bool available() const;
    int flip() const;
    float rotation() const;
public slots:
    void setFlip(int);
    void setRotation(float);
signals:
    void stateChanged();
private:
    void releaseOutput();
    void apply(int flip, float angle);
    using Output = vlc::vlc_shared_data_ptr<vout_thread_t, &vout_Hold, &vout_Release>;
    Output m_output;
    QVLCString m_filters;
    QVLCString m_type;
    QVLCFloat m_angle;
    QVLCBool m_motion;
};
#endif
