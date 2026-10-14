// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_appearance.hpp"
#include <vlc_modules.h>
#include <QColor>
#include <QRegularExpression>
#include <cmath>

SubtitleAppearance::SubtitleAppearance(vlc_player_t *player, QObject *parent)
    : QObject(parent)
    , m_player(player)
    , m_enabled((vout_thread_t *)nullptr, "sub-text-style-enabled")
    , m_embedded((vout_thread_t *)nullptr, "sub-text-style-embedded")
    , m_supported((vout_thread_t *)nullptr, "sub-text-style-supported")
    , m_font((vout_thread_t *)nullptr, "sub-text-style-font")
    , m_color((vout_thread_t *)nullptr, "sub-text-style-color")
    , m_outlineColor((vout_thread_t *)nullptr, "sub-text-style-outline-color")
    , m_mode((vout_thread_t *)nullptr, "sub-text-style-size-mode")
    , m_background((vout_thread_t *)nullptr, "sub-text-style-background-opacity")
    , m_offset((vout_thread_t *)nullptr, "sub-margin")
    , m_relative((vout_thread_t *)nullptr, "sub-text-style-relative-size")
    , m_pixels((vout_thread_t *)nullptr, "sub-text-style-pixel-size")
    , m_relativeOutline((vout_thread_t *)nullptr, "sub-text-style-relative-outline")
    , m_pixelOutline((vout_thread_t *)nullptr, "sub-text-style-pixel-outline")
    , m_scale((vlc_object_t *)nullptr, "sub-text-scale")
{
    for (auto *value : {&m_enabled, &m_embedded, &m_supported})
        connect(value, &QVLCBool::valueChanged, this, &SubtitleAppearance::stateChanged);
    connect(&m_font, &QVLCString::valueChanged, this, &SubtitleAppearance::stateChanged);
    for (auto *value : {&m_color, &m_outlineColor, &m_mode, &m_background, &m_offset, &m_scale})
        connect(value, &QVLCInteger::valueChanged, this, &SubtitleAppearance::stateChanged);
    for (auto *value : {&m_relative, &m_pixels, &m_relativeOutline, &m_pixelOutline})
        connect(value, &QVLCFloat::valueChanged, this, &SubtitleAppearance::stateChanged);
    if (player) m_scale.resetObject(reinterpret_cast<vlc_object_t *>(player));
}
SubtitleAppearance::~SubtitleAppearance() { releaseOutput(); m_scale.clearObject(); }
void SubtitleAppearance::releaseOutput()
{
    m_enabled.clearObject(); m_embedded.clearObject(); m_supported.clearObject(); m_font.clearObject();
    m_color.clearObject(); m_outlineColor.clearObject(); m_mode.clearObject();
    m_background.clearObject(); m_offset.clearObject();
    m_relative.clearObject(); m_pixels.clearObject();
    m_relativeOutline.clearObject(); m_pixelOutline.clearObject();
    m_output.reset();
}
void SubtitleAppearance::resetOutput(vout_thread_t *output)
{
    if (output == m_output.get()) return;
    releaseOutput();
    m_output.reset(output);
    if (output)
    {
        m_enabled.resetObject(output); m_embedded.resetObject(output); m_supported.resetObject(output); m_font.resetObject(output);
        m_color.resetObject(output); m_outlineColor.resetObject(output); m_mode.resetObject(output);
        m_background.resetObject(output); m_offset.resetObject(output);
        m_relative.resetObject(output); m_pixels.resetObject(output);
        m_relativeOutline.resetObject(output); m_pixelOutline.resetObject(output);
    }
    emit stateChanged();
}
void SubtitleAppearance::refresh() { emit stateChanged(); }
bool SubtitleAppearance::available() const
{
    if (!m_output || !m_player || !var_GetBool(m_output.get(), "sub-text-style-supported")) return false;
    vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
    const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
    if (!track) return false;
    switch (track->fmt.i_codec)
    {
        case VLC_CODEC_SUBT: case VLC_CODEC_TEXT: case VLC_CODEC_QTXT:
        case VLC_CODEC_TX3G: case VLC_CODEC_USF: case VLC_CODEC_TTML:
        case VLC_CODEC_TTML_TS: case VLC_CODEC_WEBVTT: return true;
        default: return false;
    }
}
QString SubtitleAppearance::statusText() const
{
    if (m_output && !var_GetBool(m_output.get(), "sub-text-style-supported"))
        return qtr("Subtitle appearance is unavailable in this playback mode.");
    return available() ? QString{} : qtr("Select a text subtitle track to change its appearance.");
}
bool SubtitleAppearance::custom() const { return m_output && var_GetBool(m_output.get(), "sub-text-style-enabled"); }
float SubtitleAppearance::scale() const { return m_player ? qMax(0.1f, var_GetInteger(reinterpret_cast<vlc_object_t *>(m_player), "sub-text-scale") / 100.f) : 1.f; }
bool SubtitleAppearance::embeddedFormat() const { return !custom() || var_GetBool(m_output.get(), "sub-text-style-embedded"); }
int SubtitleAppearance::sizeMode() const { return custom() ? var_GetInteger(m_output.get(), "sub-text-style-size-mode") : 0; }
int SubtitleAppearance::offset() const { return m_output ? var_GetInteger(m_output.get(), "sub-margin") : 0; }
QString SubtitleAppearance::fontFamily() const
{
    if (!m_output) return {};
    auto name = vlc::wrap_cptr(var_InheritString(m_output.get(), custom() ? "sub-text-style-font" : "freetype-font"));
    if (!name || !*name) name.reset(var_InheritString(m_output.get(), "freetype-font"));
    return QString::fromUtf8(name.get());
}
QString SubtitleAppearance::color(const char *name, const char *fallback) const
{
    const int rgb = m_output ? var_InheritInteger(m_output.get(), custom() ? name : fallback) : 0xFFFFFF;
    return QColor::fromRgb(rgb & 0xFFFFFF).name(QColor::HexRgb).toUpper();
}
QString SubtitleAppearance::textColor() const { return color("sub-text-style-color", "freetype-color"); }
QString SubtitleAppearance::outlineColor() const { return color("sub-text-style-outline-color", "freetype-outline-color"); }
float SubtitleAppearance::fontSize() const
{
    return (custom() ? var_GetFloat(m_output.get(), sizeMode() ? "sub-text-style-pixel-size" : "sub-text-style-relative-size") : 6.25f) * scale();
}
float SubtitleAppearance::outlineSize() const
{
    if (!m_output) return 0.f;
    return custom() ? var_GetFloat(m_output.get(), sizeMode() ? "sub-text-style-pixel-outline" : "sub-text-style-relative-outline")
                    : var_InheritInteger(m_output.get(), "freetype-outline-thickness");
}
float SubtitleAppearance::backgroundOpacity() const
{
    return m_output ? var_InheritInteger(m_output.get(), custom() ? "sub-text-style-background-opacity" : "freetype-background-opacity") * 100.f / 255.f : 0.f;
}
void SubtitleAppearance::prepareStyle()
{
    if (custom()) return;
    // Preserve the current native defaults when the user first customizes one field.
    const auto font = fontFamily(), text = textColor(), outline = outlineColor();
    const auto thickness = outlineSize(), background = backgroundOpacity();
    m_font.setValue(font);
    m_color.setValue(QColor(text).rgb() & 0xFFFFFF);
    m_outlineColor.setValue(QColor(outline).rgb() & 0xFFFFFF);
    m_relativeOutline.setValue(thickness);
    m_mode.setValue(0);
    m_relative.setValue(6.25f);
    m_embedded.setValue(true);
    m_background.setValue(qRound(background * 255.f / 100.f));
    m_enabled.setValue(true);
}
void SubtitleAppearance::setEmbeddedFormat(bool value) { if (available()) { prepareStyle(); m_embedded.setValue(value); } }
void SubtitleAppearance::setFontFamily(const QString &value)
{
    if (available() && !value.trimmed().isEmpty() && value.size() <= 256)
    { prepareStyle(); m_font.setValue(value.trimmed()); }
}
void SubtitleAppearance::setColor(const QString &value, QVLCInteger &variable)
{
    static const QRegularExpression pattern("^#[0-9a-fA-F]{6}$");
    if (available() && pattern.match(value).hasMatch())
    { prepareStyle(); variable.setValue(QColor(value).rgb() & 0xFFFFFF); }
}
void SubtitleAppearance::setTextColor(const QString &value) { setColor(value, m_color); }
void SubtitleAppearance::setOutlineColor(const QString &value) { setColor(value, m_outlineColor); }
void SubtitleAppearance::setOffset(int value) { if (available() && value >= -2000 && value <= 2000) m_offset.setValue(value); }
void SubtitleAppearance::setSizeMode(int value)
{
    if (available() && value >= 0 && value <= 1) { prepareStyle(); m_mode.setValue(value); }
}
void SubtitleAppearance::setFontSize(float value)
{
    const float base = value / scale();
    if (available() && std::isfinite(base) && base >= (sizeMode() ? 1.f : 0.1f) && base <= (sizeMode() ? 512.f : 50.f))
    { prepareStyle(); (sizeMode() ? m_pixels : m_relative).setValue(base); }
}
void SubtitleAppearance::setOutlineSize(float value)
{
    if (available() && std::isfinite(value) && value >= 0.f && value <= 50.f)
    { prepareStyle(); (sizeMode() ? m_pixelOutline : m_relativeOutline).setValue(value); }
}
void SubtitleAppearance::setBackgroundOpacity(float value)
{
    if (available() && std::isfinite(value) && value >= 0.f && value <= 100.f)
    { prepareStyle(); m_background.setValue(qRound(value * 255.f / 100.f)); }
}
