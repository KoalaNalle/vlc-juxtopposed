// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitle_appearance.hpp"
#include "util/filterchain.hpp"
#include <QByteArrayList>
#include <vlc_modules.h>
#include <QColor>
#include <QRegularExpression>
#include <QSignalBlocker>
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
    , m_subFilters((vout_thread_t *)nullptr, "sub-filter")
    , m_duration((vout_thread_t *)nullptr, "subsdelay-factor")
    , m_durationMode((vout_thread_t *)nullptr, "subsdelay-mode")
    , m_durationOverlap((vout_thread_t *)nullptr, "subsdelay-overlap")
{
    for (auto *value : {&m_enabled, &m_embedded, &m_supported})
        connect(value, &QVLCBool::valueChanged, this, &SubtitleAppearance::stateChanged);
    connect(&m_font, &QVLCString::valueChanged, this, &SubtitleAppearance::stateChanged);
    for (auto *value : {&m_color, &m_outlineColor, &m_mode, &m_background, &m_offset, &m_scale})
        connect(value, &QVLCInteger::valueChanged, this, &SubtitleAppearance::stateChanged);
    for (auto *value : {&m_relative, &m_pixels, &m_relativeOutline, &m_pixelOutline})
        connect(value, &QVLCFloat::valueChanged, this, &SubtitleAppearance::stateChanged);
    if (player) m_scale.resetObject(reinterpret_cast<vlc_object_t *>(player));
    connect(&m_subFilters, &QVLCString::valueChanged, this, &SubtitleAppearance::stateChanged);
    connect(&m_duration, &QVLCFloat::valueChanged, this, &SubtitleAppearance::stateChanged);
    connect(&m_durationMode, &QVLCInteger::valueChanged, this, &SubtitleAppearance::stateChanged);
    connect(&m_durationOverlap, &QVLCInteger::valueChanged, this, &SubtitleAppearance::stateChanged);

}
SubtitleAppearance::~SubtitleAppearance() { releaseOutput(); m_scale.clearObject(); }
void SubtitleAppearance::releaseOutput()
{
    m_proxyOutput = nullptr;
    const char *commands[] = {"subsdelay-factor", "subsdelay-mode", "subsdelay-overlap", "sub-filter"};
    for (unsigned i = 0; i < 4; ++i)
        if (m_commandCallbacks & (1u << i)) var_DelCallback(m_output.get(), commands[i], durationCommand, this);
    m_commandCallbacks = 0;
    m_enabled.clearObject(); m_embedded.clearObject(); m_supported.clearObject(); m_font.clearObject();
    m_color.clearObject(); m_outlineColor.clearObject(); m_mode.clearObject();
    m_background.clearObject(); m_offset.clearObject();
    m_relative.clearObject(); m_pixels.clearObject();
    m_relativeOutline.clearObject(); m_pixelOutline.clearObject();
    m_subFilters.clearObject(); m_duration.clearObject(); m_durationMode.clearObject(); m_durationOverlap.clearObject();
    m_output.reset();
}
void SubtitleAppearance::resetOutput(vout_thread_t *output)
{
    if (output == m_output.get()) return;
    {
        const QSignalBlocker notifications(this);
        releaseOutput();
        m_output.reset(output);
        m_factorCommanded = false; m_modeCommanded = false; m_overlapCommanded = false;
        m_proxyOutput = reinterpret_cast<vlc_object_t *>(output);
        if (output)
        {
            m_enabled.resetObject(output); m_embedded.resetObject(output); m_supported.resetObject(output); m_font.resetObject(output);
            m_color.resetObject(output); m_outlineColor.resetObject(output); m_mode.resetObject(output);
            m_background.resetObject(output); m_offset.resetObject(output);
            m_relative.resetObject(output); m_pixels.resetObject(output);
            m_relativeOutline.resetObject(output); m_pixelOutline.resetObject(output);
            m_subFilters.resetObject(output);
            if (module_exists("subsdelay"))
            {
                // Proxy mappings survive filter removal; our observers own only
                // their additional references, as in filter_AddProxyCallbacks().
                for (const auto &option : {std::pair{"subsdelay-factor", VLC_VAR_FLOAT}, std::pair{"subsdelay-mode", VLC_VAR_INTEGER}, std::pair{"subsdelay-overlap", VLC_VAR_INTEGER}})
                {
                    const bool exists = var_Type(output, option.first) != 0;
                    var_Create(output, option.first, option.second | VLC_VAR_DOINHERIT | VLC_VAR_ISCOMMAND);
                    if (exists) var_Destroy(output, option.first);
                }
                m_duration.resetObject(output); m_durationMode.resetObject(output); m_durationOverlap.resetObject(output);
            }
            const char *commands[] = {"subsdelay-factor", "subsdelay-mode", "subsdelay-overlap", "sub-filter"};
            for (unsigned i = 0; i < 4; ++i)
                if (var_Type(output, commands[i])) {
                    var_AddCallback(output, commands[i], durationCommand, this);
                    m_commandCallbacks |= 1u << i;
                }
        }
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

bool SubtitleAppearance::durationAvailable() const
{
    if (!m_output || !m_player || !module_exists("subsdelay")) return false;
    vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
    return vlc_player_GetSelectedTrack(m_player, SPU_ES) != nullptr;
}
float SubtitleAppearance::durationFactor() const
{
    for (const auto &entry : vlc::qt::filterChainEntries(m_output.get(), "sub-filter"))
        if (entry.name == "subsdelay")
            return entry.options.contains("factor") && !m_factorCommanded.load() ? entry.options["factor"].toFloat()
                                                     : var_GetFloat(m_output.get(), "subsdelay-factor");
    return 0.f;
}
int SubtitleAppearance::durationMode() const
{
    for (const auto &entry : vlc::qt::filterChainEntries(m_output.get(), "sub-filter"))
        if (entry.name == "subsdelay" && entry.options.contains("mode") && !m_modeCommanded.load()) return entry.options["mode"].toInt();
    return m_output && module_exists("subsdelay") ? var_GetInteger(m_output.get(), "subsdelay-mode") : 1;
}
QString SubtitleAppearance::durationUnit() const { return durationMode() == 0 ? QStringLiteral(" s") : QString{}; }
QString SubtitleAppearance::durationDescription() const
{
    const auto mode = durationMode();
    return mode == 0 ? qtr("Extend each subtitle by this many seconds. 0 disables the adjustment; filter changes apply as cues are loaded.")
        : mode == 2 ? qtr("Recalculate subtitle duration from its text and this factor. 0 disables the adjustment; filter changes apply as cues are loaded.")
                    : qtr("Multiply each subtitle's duration by this factor. 0 disables the adjustment; filter changes apply as cues are loaded.");
}
void SubtitleAppearance::setDurationFactor(float value)
{
    if (!durationAvailable() || !std::isfinite(value) || value < 0.f || value > 20.f) return;
    const auto mode = durationMode();
    const auto previousFactor = durationFactor();
    int overlap = var_GetInteger(m_output.get(), "subsdelay-overlap");
    QByteArrayList filters;
    QByteArray durationFilter = "subsdelay";
    int position = -1;
    for (const auto &entry : vlc::qt::filterChainEntries(m_output.get(), "sub-filter"))
    {
        if (entry.name == "subsdelay")
        {
            if (position < 0)
            {
                position = filters.size();
                bool validOverlap = false;
                const int initialOverlap = entry.options.value("overlap").toInt(&validOverlap);
                if (validOverlap && !m_overlapCommanded.load()) overlap = initialOverlap;
            }
        }
        else filters.push_back(entry.original);
    }
    m_durationMode.setValue(mode);
    m_duration.setValue(value > 0.f ? value : previousFactor);
    m_durationOverlap.setValue(overlap);
    if (value > 0.f)
    {
        filters.insert(position < 0 ? filters.size() : position, durationFilter);
    }
    m_subFilters.setValue(QString::fromUtf8(filters.join(':')));
}


bool SubtitleAppearance::validSnapshot(const Snapshot &s)
{
    auto range = [](float value, float minimum, float maximum) {
        return std::isfinite(value) && value >= minimum && value <= maximum;
    };
    return s.font.size() <= 256 && !s.font.contains(QChar::Null)
        && s.text >= 0 && s.text <= 0xFFFFFF && s.outline >= 0 && s.outline <= 0xFFFFFF
        && s.sizeMode >= 0 && s.sizeMode <= 1 && s.background >= 0 && s.background <= 255
        && s.offset >= -2000 && s.offset <= 2000 && s.scale >= 10 && s.scale <= 500
        && range(s.relativeSize, 0.1f, 50.f) && range(s.pixelSize, 1.f, 512.f)
        && range(s.relativeOutline, 0.f, 50.f) && range(s.pixelOutline, 0.f, 50.f)
        && s.durationMode >= 0 && s.durationMode <= 2
        && s.durationOverlap >= 1 && s.durationOverlap <= 4
        && range(s.durationFactor, 0.f, 20.f);
}

SubtitleAppearance::Snapshot SubtitleAppearance::configuredSnapshot(vlc_object_t *source)
{
    Snapshot s;
    if (!source) return s;
    s.enabled = var_InheritBool(source, "sub-text-style-enabled");
    s.embedded = var_InheritBool(source, "sub-text-style-embedded");
    auto font = vlc::wrap_cptr(var_InheritString(source, "sub-text-style-font"));
    s.font = QString::fromUtf8(font.get());
    s.text = var_InheritInteger(source, "sub-text-style-color");
    s.outline = var_InheritInteger(source, "sub-text-style-outline-color");
    s.sizeMode = var_InheritInteger(source, "sub-text-style-size-mode");
    s.background = var_InheritInteger(source, "sub-text-style-background-opacity");
    s.offset = var_InheritInteger(source, "sub-margin");
    s.relativeSize = var_InheritFloat(source, "sub-text-style-relative-size");
    s.pixelSize = var_InheritFloat(source, "sub-text-style-pixel-size");
    s.relativeOutline = var_InheritFloat(source, "sub-text-style-relative-outline");
    s.pixelOutline = var_InheritFloat(source, "sub-text-style-pixel-outline");
    s.scale = var_InheritInteger(source, "sub-text-scale");
    if (module_exists("subsdelay")) {
        s.durationMode = var_InheritInteger(source, "subsdelay-mode");
        s.durationFactor = var_InheritFloat(source, "subsdelay-factor");
        s.durationOverlap = var_InheritInteger(source, "subsdelay-overlap");
        auto chain = vlc::wrap_cptr(var_InheritString(source, "sub-filter"));
        while (chain && *chain) {
            char *name = nullptr;
            config_chain_t *options = nullptr;
            auto next = vlc::wrap_cptr(config_ChainCreate(&name, &options, chain.get()));
            if (name && std::strcmp(name, "subsdelay") == 0) {
                s.durationEnabled = true;
                for (auto *option = options; option; option = option->p_next) {
                    const QString key = QString::fromUtf8(option->psz_name);
                    const QString value = QString::fromUtf8(option->psz_value);
                    if (key == "mode") s.durationMode = value.toInt();
                    if (key == "factor") s.durationFactor = value.toFloat();
                    if (key == "overlap") s.durationOverlap = value.toInt();
                }
            }
            free(name); config_ChainDestroy(options); chain = std::move(next);
        }
    }
    return s;
}

bool SubtitleAppearance::snapshot(Snapshot *result, input_item_t *expectedMedia) const
{
    if (!result || !m_output || !m_player) return false;
    vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
    if (expectedMedia && vlc_player_GetCurrentMedia(m_player) != expectedMedia) return false;
    auto *currentOutput = vlc_player_vout_Hold(m_player);
    const bool correctOutput = currentOutput == m_output.get();
    if (currentOutput) vout_Release(currentOutput);
    if (!correctOutput || !var_GetBool(m_output.get(), "sub-text-style-supported")) return false;
    const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
    if (!track) return false;
    switch (track->fmt.i_codec) {
        case VLC_CODEC_SUBT: case VLC_CODEC_TEXT: case VLC_CODEC_QTXT:
        case VLC_CODEC_TX3G: case VLC_CODEC_USF: case VLC_CODEC_TTML:
        case VLC_CODEC_TTML_TS: case VLC_CODEC_WEBVTT: break;
        default: return false;
    }
    Snapshot s;
    auto *output = m_output.get();
    s.enabled = var_GetBool(output, "sub-text-style-enabled");
    s.embedded = var_GetBool(output, "sub-text-style-embedded");
    auto font = vlc::wrap_cptr(var_GetString(output, "sub-text-style-font"));
    s.font = QString::fromUtf8(font.get());
    s.text = var_GetInteger(output, "sub-text-style-color");
    s.outline = var_GetInteger(output, "sub-text-style-outline-color");
    s.sizeMode = var_GetInteger(output, "sub-text-style-size-mode");
    s.background = var_GetInteger(output, "sub-text-style-background-opacity");
    s.offset = var_GetInteger(output, "sub-margin");
    s.relativeSize = var_GetFloat(output, "sub-text-style-relative-size");
    s.pixelSize = var_GetFloat(output, "sub-text-style-pixel-size");
    s.relativeOutline = var_GetFloat(output, "sub-text-style-relative-outline");
    s.pixelOutline = var_GetFloat(output, "sub-text-style-pixel-outline");
    s.scale = var_GetInteger(reinterpret_cast<vlc_object_t *>(m_player), "sub-text-scale");
    if (module_exists("subsdelay")) {
        s.durationMode = durationMode();
        s.durationFactor = var_GetFloat(output, "subsdelay-factor");
        s.durationOverlap = var_GetInteger(output, "subsdelay-overlap");
        for (const auto &entry : vlc::qt::filterChainEntries(output, "sub-filter"))
            if (entry.name == "subsdelay") {
                s.durationEnabled = true;
                if (entry.options.contains("factor") && !m_factorCommanded.load()) s.durationFactor = entry.options["factor"].toFloat();
                if (entry.options.contains("overlap") && !m_overlapCommanded.load()) s.durationOverlap = entry.options["overlap"].toInt();
                break;
            }
    }
    if (!validSnapshot(s)) return false;
    *result = std::move(s);
    return true;
}

bool SubtitleAppearance::applySnapshot(const Snapshot &s, input_item_t *expectedMedia,
    const std::atomic_uint64_t *generation, uint64_t expectedGeneration)
{
    if (!validSnapshot(s) || !expectedMedia || !m_output || !m_player
        || (s.durationEnabled && !module_exists("subsdelay"))) return false;
    {
        const QSignalBlocker notifications(this);
        vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock> lock(m_player);
        if (vlc_player_GetCurrentMedia(m_player) != expectedMedia
            || (generation && generation->load() != expectedGeneration)) return false;
        auto *currentOutput = vlc_player_vout_Hold(m_player);
        const bool correctOutput = currentOutput == m_output.get();
        if (currentOutput) vout_Release(currentOutput);
        if (!correctOutput || !var_GetBool(m_output.get(), "sub-text-style-supported")) return false;
        const auto *track = vlc_player_GetSelectedTrack(m_player, SPU_ES);
        if (!track) return false;
        switch (track->fmt.i_codec) {
            case VLC_CODEC_SUBT: case VLC_CODEC_TEXT: case VLC_CODEC_QTXT:
            case VLC_CODEC_TX3G: case VLC_CODEC_USF: case VLC_CODEC_TTML:
            case VLC_CODEC_TTML_TS: case VLC_CODEC_WEBVTT: break;
            default: return false;
        }
        QByteArrayList filters;
        int position = -1;
        for (const auto &entry : vlc::qt::filterChainEntries(m_output.get(), "sub-filter")) {
            if (entry.name == "subsdelay") {
                if (position < 0) position = filters.size();
            } else filters.push_back(entry.original);
        }
        if (module_exists("subsdelay")) {
            m_durationMode.setValue(s.durationMode);
            m_duration.setValue(s.durationFactor);
            m_durationOverlap.setValue(s.durationOverlap);
        }
        if (s.durationEnabled) {
            // The proxy values above initialize the filter and remain the live
            // authority. Do not put stale duplicates into an owned inline chain.
            const QByteArray filter = "subsdelay";
            filters.insert(position < 0 ? filters.size() : position, filter);
        }
        m_font.setValue(s.font); m_color.setValue(s.text); m_outlineColor.setValue(s.outline);
        m_mode.setValue(s.sizeMode); m_background.setValue(s.background); m_offset.setValue(s.offset);
        m_relative.setValue(s.relativeSize); m_pixels.setValue(s.pixelSize);
        m_relativeOutline.setValue(s.relativeOutline); m_pixelOutline.setValue(s.pixelOutline);
        m_embedded.setValue(s.embedded); m_enabled.setValue(s.enabled);
        vlc_player_SetSubtitleTextScale(m_player, s.scale);
        m_subFilters.setValue(QString::fromUtf8(filters.join(':')));
    }
    emit stateChanged();
    return true;
}

bool SubtitleAppearance::Snapshot::operator==(const Snapshot &s) const
{
    return enabled == s.enabled && embedded == s.embedded && font == s.font
        && text == s.text && outline == s.outline && sizeMode == s.sizeMode
        && background == s.background && offset == s.offset && scale == s.scale
        && relativeSize == s.relativeSize && pixelSize == s.pixelSize
        && relativeOutline == s.relativeOutline && pixelOutline == s.pixelOutline
        && durationEnabled == s.durationEnabled
        && durationMode == s.durationMode && durationFactor == s.durationFactor
        && durationOverlap == s.durationOverlap;
}

int SubtitleAppearance::durationCommand(vlc_object_t *object, const char *name, vlc_value_t, vlc_value_t, void *opaque)
{
    auto *self = static_cast<SubtitleAppearance *>(opaque);
    if (object != self->m_proxyOutput.load()) return VLC_SUCCESS;
    if (std::strcmp(name, "sub-filter") == 0) {
        self->m_factorCommanded = false; self->m_modeCommanded = false; self->m_overlapCommanded = false;
    } else {
        if (std::strcmp(name, "subsdelay-factor") == 0) self->m_factorCommanded = true;
        if (std::strcmp(name, "subsdelay-mode") == 0) self->m_modeCommanded = true;
        if (std::strcmp(name, "subsdelay-overlap") == 0) self->m_overlapCommanded = true;
    }
    QMetaObject::invokeMethod(self, &SubtitleAppearance::refresh, Qt::QueuedConnection);
    return VLC_SUCCESS;
}
