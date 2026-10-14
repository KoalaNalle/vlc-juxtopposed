// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "video_transform.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <QByteArrayList>
#include <QMap>
#include <cmath>

namespace {
struct FilterEntry
{
    QString name;
    QByteArray original;
    QMap<QString, QString> options;
};

// Use VLC's parser: a ':' inside a quoted filter option is not a separator.
QList<FilterEntry> filterEntries(vout_thread_t *output)
{
    QList<FilterEntry> entries;
    if (!output) return entries;
    auto chain = vlc::wrap_cptr(var_GetString(output, "video-filter"));
    while (chain && *chain)
    {
        char *name = nullptr;
        config_chain_t *options = nullptr;
        auto next = vlc::wrap_cptr(config_ChainCreate(&name, &options, chain.get()));
        FilterEntry entry;
        entry.name = QString::fromUtf8(name);
        const size_t length = strlen(chain.get()) - (next ? strlen(next.get()) + 1 : 0);
        entry.original = QByteArray(chain.get(), length);
        for (auto *option = options; option; option = option->p_next)
            entry.options.insert(QString::fromUtf8(option->psz_name), QString::fromUtf8(option->psz_value));
        if (!entry.name.isEmpty()) entries.push_back(std::move(entry));
        free(name);
        config_ChainDestroy(options);
        chain = std::move(next);
    }
    return entries;
}

struct State { int flip = 0; float angle = 0.f; };
State currentState(vout_thread_t *output)
{
    State state;
    for (const auto &entry : filterEntries(output))
    {
        if (entry.name == "transform")
        {
            auto value = vlc::wrap_cptr(var_GetString(output, "transform-type"));
            const auto type = entry.options.value("type", QString::fromUtf8(value.get()));
            if (type == "hflip") state.flip = 1;
            else if (type == "vflip") state.flip = 2;
            else if (type == "90") state.angle += 270.f;
            else if (type == "180") state.angle += 180.f;
            else if (type == "270") state.angle += 90.f;
            else if (type == "transpose") { state.flip = 1; state.angle += 270.f; }
            else if (type == "antitranspose") { state.flip = 1; state.angle += 90.f; }
        }
        else if (entry.name == "rotate")
            state.angle += entry.options.contains("angle") ? entry.options["angle"].toFloat()
                                                           : var_GetFloat(output, "rotate-angle");
    }
    state.angle = std::isfinite(state.angle) ? std::fmod(state.angle, 360.f) : 0.f;
    if (state.angle < 0.f) state.angle += 360.f;
    return state;
}
}

VideoTransform::VideoTransform(QObject *parent)
    : QObject(parent)
    , m_filters((vout_thread_t *)nullptr, "video-filter")
    , m_type((vout_thread_t *)nullptr, "transform-type")
    , m_angle((vout_thread_t *)nullptr, "rotate-angle")
    , m_motion((vout_thread_t *)nullptr, "rotate-use-motion")
{
    connect(&m_filters, &QVLCString::valueChanged, this, &VideoTransform::stateChanged);
    connect(&m_type, &QVLCString::valueChanged, this, &VideoTransform::stateChanged);
    connect(&m_angle, &QVLCFloat::valueChanged, this, &VideoTransform::stateChanged);
    connect(&m_motion, &QVLCBool::valueChanged, this, &VideoTransform::stateChanged);
}
VideoTransform::~VideoTransform() { releaseOutput(); }
void VideoTransform::releaseOutput()
{
    m_filters.clearObject();
    m_type.clearObject();
    m_angle.clearObject();
    m_motion.clearObject();
    // Filter mapping variables belong to the vout and survive proxy removal,
    // as in filter_AddProxyCallbacks(). Only our observers release references.
    m_output.reset();
}
void VideoTransform::resetOutput(vout_thread_t *output)
{
    if (output == m_output.get()) return;
    releaseOutput();
    m_output.reset(output);
    if (output)
    {
        // ISCOMMAND also lets the filter proxy remove its callbacks on rebuild.
        for (const auto &option : {std::pair{"transform-type", VLC_VAR_STRING},
                                   std::pair{"rotate-angle", VLC_VAR_FLOAT},
                                   std::pair{"rotate-use-motion", VLC_VAR_BOOL}})
        {
            const bool exists = var_Type(output, option.first) != 0;
            var_Create(output, option.first, option.second | VLC_VAR_DOINHERIT | VLC_VAR_ISCOMMAND);
            if (exists) var_Destroy(output, option.first);
        }
        m_filters.resetObject(output);
        m_type.resetObject(output);
        m_angle.resetObject(output);
        m_motion.resetObject(output);
    }
    emit stateChanged();
}
bool VideoTransform::available() const { return m_output && module_exists("transform") && module_exists("rotate"); }
int VideoTransform::flip() const { return currentState(m_output.get()).flip; }
float VideoTransform::rotation() const { return currentState(m_output.get()).angle; }
void VideoTransform::apply(int flip, float angle)
{
    if (!available()) return;
    QByteArrayList filters;
    for (const auto &entry : filterEntries(m_output.get()))
    {
        if (entry.name != "transform" && entry.name != "rotate")
            filters.push_back(entry.original);
    }
    // These are inherited by newly added filters and forwarded by live proxies.
    m_type.setValue(flip == 2 ? "vflip" : "hflip");
    m_angle.setValue(angle);
    m_motion.setValue(false);
    // Transform changes orientation metadata, while rotate samples raw pixels.
    // Keep transform last so the vout's final converter applies its orientation.
    if (angle != 0.f) filters.push_back("rotate");
    if (flip) filters.push_back("transform");
    m_filters.setValue(QString::fromUtf8(filters.join(':')));
}
void VideoTransform::setFlip(int value)
{
    if (value >= 0 && value <= 2) apply(value, rotation());
}
void VideoTransform::setRotation(float value)
{
    if (std::isfinite(value) && value >= 0.f && value <= 360.f)
        apply(flip(), value == 360.f ? 0.f : value);
}
