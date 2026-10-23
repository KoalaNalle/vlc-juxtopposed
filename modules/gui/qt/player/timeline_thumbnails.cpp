// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "timeline_thumbnails.hpp"
#include <vlc_image.h>
#include <vlc_picture.h>
#include <vlc_preparser.h>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QQuickImageProvider>
#include <QSemaphore>
#include <QStringList>
#include <cmath>
#include <limits>
#include <algorithm>

struct TimelineThumbnailImages
{
    QMutex mutex;
    QHash<QString, QImage> images;
    QStringList order;
};

namespace {
class ThumbnailImageProvider final : public QQuickImageProvider
{
public:
    explicit ThumbnailImageProvider(std::shared_ptr<TimelineThumbnailImages> images)
        : QQuickImageProvider(QQuickImageProvider::Image), m_images(std::move(images)) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &) override
    {
        QMutexLocker lock(&m_images->mutex);
        const auto image = m_images->images.value(id);
        if (size) *size = image.size();
        return image;
    }
private:
    std::shared_ptr<TimelineThumbnailImages> m_images;
};
struct DecodeState
{
    QSemaphore ended;
    int status = VLC_EGENERIC;
    picture_t *picture = nullptr;
    ~DecodeState() { if (picture) picture_Release(picture); }
};
const vlc_thumbnailer_cbs Callbacks = {
    [](vlc_preparser_req *, int status, picture_t *picture, void *opaque) {
        auto *state = static_cast<DecodeState *>(opaque);
        state->status = status;
        if (picture) state->picture = picture_Hold(picture);
        state->ended.release();
    }
};
}

TimelineThumbnails::TimelineThumbnails(vlc_object_t *parent, QObject *objectParent)
    : QObject(objectParent), m_parent(parent), m_images(std::make_shared<TimelineThumbnailImages>())
{
    m_pool.setMaxThreadCount(1);
    m_timer.setSingleShot(true);
    m_timer.setInterval(120);
    connect(&m_timer, &QTimer::timeout, this, &TimelineThumbnails::startDecode);
}

TimelineThumbnails::~TimelineThumbnails()
{
    invalidate();
    // No worker may outlive its borrowed native parent or this QObject.
    m_pool.waitForDone();
}

QQuickImageProvider *TimelineThumbnails::createImageProvider() const
{
    return new ThumbnailImageProvider(m_images);
}

void TimelineThumbnails::invalidate()
{
    ++m_generation;
    m_timer.stop();
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    m_loading = false;
    m_requested = -1;
    m_imageSource.clear();
    m_error.clear();
    m_client.clear();
}

void TimelineThumbnails::cancel()
{
    invalidate();
    emit stateChanged();
}

void TimelineThumbnails::setInput(input_item_t *input)
{
    invalidate();
    // Clone even when the URI is unchanged: reopening an item is a new media
    // generation. Parsing/thumbnail metadata must not mutate the live item.
    m_input = SharedInputItem(input ? input_item_Copy(input) : nullptr, false);
    m_cache.clear();
    {
        QMutexLocker lock(&m_images->mutex);
        m_images->images.clear();
        m_images->order.clear();
    }
    emit stateChanged();
}

void TimelineThumbnails::setEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!enabled) invalidate();
    emit enabledChanged();
    emit stateChanged();
}

bool TimelineThumbnails::request(double seconds)
{
    m_client.clear();
    return requestTime(seconds);
}

bool TimelineThumbnails::requestFor(const QString &client, double seconds)
{
    if (client.isEmpty()) return false;
    m_client = client;
    return requestTime(seconds);
}

void TimelineThumbnails::cancelFor(const QString &client)
{
    if (!client.isEmpty() && client == m_client) cancel();
}

bool TimelineThumbnails::requestTime(double seconds)
{
    // Quantize to the displayed whole second and coalesce moving-pointer
    // requests. Do not allocate one decoder/job per mouse event.
    if (!m_enabled || !m_input || !std::isfinite(seconds) || seconds < 0
        || seconds >= double(std::numeric_limits<vlc_tick_t>::max() / CLOCK_FREQ - 1))
    {
        cancel();
        return false;
    }
    const auto second = qint64(std::floor(seconds));
    if (m_requested == second && (m_loading || !m_imageSource.isEmpty())) return true;
    ++m_generation;
    if (m_cancel) *m_cancel = true;
    m_pool.clear();
    m_requested = second;
    m_error.clear();
    m_imageSource.clear();
    if (const auto *image = m_cache.object(second))
    {
        m_timer.stop();
        publish(*image);
        return true;
    }
    m_loading = true;
    emit stateChanged();
    // Throttle rather than debounce: continuous movement still gets requests.
    if (!m_timer.isActive()) m_timer.start();
    return true;
}

void TimelineThumbnails::publish(const QImage &image)
{
    const auto id = QString::number(++m_imageSerial);
    {
        QMutexLocker lock(&m_images->mutex);
        m_images->images.insert(id, image);
        m_images->order.append(id);
        while (m_images->order.size() > 8)
            m_images->images.remove(m_images->order.takeFirst());
    }
    m_imageSource = QStringLiteral("image://timelinepreview/") + id;
    m_loading = false;
    emit stateChanged();
}

void TimelineThumbnails::startDecode()
{
    if (!m_enabled || !m_input || m_requested < 0) return;
    const auto generation = m_generation;
    const auto second = m_requested;
    const auto input = m_input;
    m_cancel = std::make_shared<std::atomic_bool>(false);
    const auto cancel = m_cancel;
    m_pool.start([this, generation, second, input, cancel] {
        auto result = decode(m_parent, input, second, cancel);
        if (*cancel) return;
        QMetaObject::invokeMethod(this, [this, generation, second, result = std::move(result)] {
            if (generation != m_generation) return;
            m_loading = false;
            m_error = result.error;
            if (!result.image.isNull())
            {
                m_cache.insert(second, new QImage(result.image));
                publish(result.image);
            }
            else emit stateChanged();
        }, Qt::QueuedConnection);
    });
}

TimelineThumbnails::Result TimelineThumbnails::decode(vlc_object_t *parent,
    const SharedInputItem &input, qint64 second, const Cancellation &cancel)
{
    if (*cancel) return {};
    vlc_preparser_cfg cfg{};
    cfg.types = VLC_PREPARSER_TYPE_THUMBNAIL;
    cfg.max_thumbnailer_threads = 1;
    cfg.timeout = VLC_TICK_FROM_SEC(3);
    cfg.external_process = false;
    auto *parser = vlc_preparser_New(parent, &cfg);
    if (!parser) return {{}, tr("Could not start the video preview decoder.")};
    DecodeState state;
    vlc_thumbnailer_arg args{};
    args.seek.type = vlc_thumbnailer_arg::seek::VLC_THUMBNAILER_SEEK_TIME;
    args.seek.time = VLC_TICK_FROM_SEC(second);
    args.seek.speed = vlc_thumbnailer_arg::seek::VLC_THUMBNAILER_SEEK_PRECISE;
    auto *request = vlc_preparser_req_NewThumbnail(parser, input.get(), &args, &Callbacks, &state);
    bool ended = false;
    if (request && vlc_preparser_Submit(parser, request) == VLC_SUCCESS)
    {
        QElapsedTimer timer;
        timer.start();
        while (!(ended = state.ended.tryAcquire(1, 20)))
            if (*cancel || timer.elapsed() >= 4000) break;
        if (!ended) vlc_preparser_Cancel(parser, request);
    }
    if (request) vlc_preparser_req_Release(request);
    // Join callbacks before their context/picture can be destroyed. Native
    // cancellation and deletion stay on this worker, never on pointer input.
    vlc_preparser_Delete(parser);
    if (*cancel) return {};
    if (!ended || state.status != VLC_SUCCESS || !state.picture)
        return {{}, tr("No video preview is available at this position.")};
    // Convert pixels directly. Encoding raw RGBA as an "image" unnecessarily
    // probes image encoders, incurs codec work and can emit encoder errors.
    auto in = state.picture->format; // Borrowed palette/ICC data, not cleaned here.
    const auto width = in.i_visible_width ? in.i_visible_width : in.i_width;
    const auto height = in.i_visible_height ? in.i_visible_height : in.i_height;
    if (!width || !height) return {{}, tr("The video preview has no image dimensions.")};
    const double sar = in.i_sar_num && in.i_sar_den ? double(in.i_sar_num)/in.i_sar_den : 1.;
    // Crop in source coordinates, accounting for non-square pixels and the
    // quarter-turn/transposed orientations before converting to square RGBA.
    const bool swapped = in.orientation >= ORIENT_LEFT_TOP;
    const double ratio = (swapped ? 142./252. : 252./142.) / sar;
    unsigned cropWidth = width, cropHeight = height;
    if (double(width)/height > ratio)
        cropWidth = std::clamp(unsigned(height*ratio),1u,width);
    else cropHeight = std::clamp(unsigned(width/ratio),1u,height);
    in.i_x_offset += (width-cropWidth)/2;
    in.i_y_offset += (height-cropHeight)/2;
    in.i_visible_width = cropWidth;
    in.i_visible_height = cropHeight;
    if (!in.i_sar_num || !in.i_sar_den) in.i_sar_num = in.i_sar_den = 1;
    video_format_t format;
    video_format_Init(&format,VLC_CODEC_RGBA);
    format.i_width = format.i_visible_width = 252;
    format.i_height = format.i_visible_height = 142;
    format.i_sar_num = format.i_sar_den = 1;
    auto *handler = image_HandlerCreate(parent);
    if (!handler) return {{}, tr("Could not start the video preview converter.")};
    auto *pixels = image_Convert(handler,state.picture,&in,&format);
    QImage image;
    if (pixels)
    {
        const auto &f = pixels->format;
        const auto &p = pixels->p[0];
        if (f.i_chroma == VLC_CODEC_RGBA && f.i_visible_width == 252 && f.i_visible_height == 142
            && (quint64(f.i_x_offset)+252)*4 <= quint64(p.i_pitch)
            && quint64(f.i_y_offset)+142 <= quint64(p.i_lines) && p.i_pitch > 0 && p.i_lines > 0)
            image = QImage(p.p_pixels + f.i_y_offset*p.i_pitch + f.i_x_offset*4,
                252,142,p.i_pitch,QImage::Format_RGBA8888).copy();
        picture_Release(pixels);
    }
    image_HandlerDelete(handler);
    video_format_Clean(&format);
    if (image.isNull()) return {{}, tr("The video preview has an unsupported image format.")};
    return {image, {}};
}
