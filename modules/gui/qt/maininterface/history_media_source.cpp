// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "history_media_source.hpp"
#include "medialibrary/medialib.hpp"
#include <vlc_media_library.h>

HistoryMediaSource::HistoryMediaSource(qt_intf_t *intf, vlc::playlist::PlaylistController *playlist,
                                     MediaLib *browsing, QObject *parent)
    : QObject(parent), m_intf(intf), m_playlistController(playlist),
      m_available(browsing || (intf && vlc_ml_history_available(intf))), m_source(browsing)
{
    m_openingPool.setMaxThreadCount(1);
    if (m_source)
        connect(m_source, &MediaLib::historyClearRequested, this, &HistoryMediaSource::historyClearRequested);
}
HistoryMediaSource::~HistoryMediaSource()
{
    // The interface and borrowed backend remain alive until the opening job has
    // returned. Posted completion callbacks are removed with this QObject.
    m_openingPool.waitForDone();
    m_ownedSource.reset();
}
void HistoryMediaSource::open()
{
    if (m_source || m_busy) return;
    if (!m_available) {
        m_error = tr("Playback history is unavailable.");
        emit changed();
        return;
    }
    m_busy = true;
    m_error.clear();
    emit changed();
    m_openingPool.start([this, intf = m_intf] {
        auto *backend = vlc_ml_history_get(intf);
        // Module creation precedes the SDK's lazy storage initialization. Read
        // through the status-returning API so an empty history is distinguishable
        // from an inaccessible database before publishing a usable frontend.
        size_t count = 0;
        if (backend && vlc_ml_list(backend, VLC_ML_COUNT_HISTORY, nullptr,
                                  static_cast<int>(VLC_ML_HISTORY_TYPE_GLOBAL), &count) != VLC_SUCCESS)
            backend = nullptr;
        QMetaObject::invokeMethod(this, [this, backend] {
            m_busy = false;
            if (!backend) {
                m_error = tr("Could not open playback history.");
            } else {
                m_ownedSource = MediaLib::createHistorySource(m_intf, m_playlistController, backend, this);
                m_source = m_ownedSource.get();
                connect(m_source, &MediaLib::historyClearRequested, this, &HistoryMediaSource::historyClearRequested);
            }
            emit changed();
        }, Qt::QueuedConnection);
    });
}
