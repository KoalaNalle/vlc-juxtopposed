// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "subtitles_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/subtitle_preset_model.hpp"
#include "player/subtitle_appearance.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_iso_lang.h>
#include <vlc_player.h>
using vlc_player_locker = vlc_locker<vlc_player_t, vlc_player_Lock, vlc_player_Unlock>;
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <cmath>
#include <climits>
#include <cstring>

namespace {
struct Option { const char *key, *native; int type; };
constexpr Option direct[] = {
    {"enableSubtitles","spu",VLC_VAR_BOOL}, {"preferredLanguage","sub-language",VLC_VAR_STRING},
    {"textEncoding","subsdec-encoding",VLC_VAR_STRING}, {"autoLoad","sub-autodetect-file",VLC_VAR_BOOL},
    {"filenameMatching","sub-autodetect-fuzzy",VLC_VAR_INTEGER},
    {"bold","freetype-bold",VLC_VAR_BOOL}, {"textOpacity","freetype-opacity",VLC_VAR_INTEGER},
    {"shadow","freetype-shadow-opacity",VLC_VAR_INTEGER}, {"shadowColor","freetype-shadow-color",VLC_VAR_INTEGER},
    {"shadowDistance","freetype-shadow-distance",VLC_VAR_FLOAT},
    {"backgroundColor","freetype-background-color",VLC_VAR_INTEGER},
    {"verticalPosition","sub-primary-alignment",VLC_VAR_INTEGER}, {"subtitleDelay","sub-delay",VLC_VAR_INTEGER},
    {"showFeedback","osd-shortcut-feedback",VLC_VAR_BOOL}, {"feedbackDuration","osd-feedback-duration",VLC_VAR_INTEGER},
    {"showFilename","video-title-show",VLC_VAR_BOOL}, {"filenameDuration","video-title-timeout",VLC_VAR_INTEGER},
    {"searchFolders","sub-autodetect-path",VLC_VAR_STRING}, {"shadowAngle","freetype-shadow-angle",VLC_VAR_FLOAT},
    {"textDirection","freetype-text-direction",VLC_VAR_INTEGER}, {"monospaceFont","freetype-monofont",VLC_VAR_STRING}
};
constexpr const char *styleKeys[]={"fontFamily","sizeMode","fontSize","textColor","outlineThickness","outlineColor","background","backgroundOpacity"};
constexpr const char *styleNative[]={"sub-text-style-enabled","sub-text-style-embedded","sub-text-style-font","sub-text-style-color","sub-text-style-outline-color","sub-text-style-size-mode","sub-text-style-relative-size","sub-text-style-pixel-size","sub-text-style-relative-outline","sub-text-style-pixel-outline","sub-text-style-background-opacity"};
QString text(const char *name)
{
    if(config_GetType(name)!=VLC_VAR_STRING)return {};
    char *raw=config_GetPsz(name); const auto result=QString::fromUtf8(raw?raw:"");free(raw);return result;
}
QVariant raw(const char *name,int type)
{
    if(!type||config_GetType(name)!=type)return {};
    return type==VLC_VAR_STRING?QVariant(text(name)):type==VLC_VAR_FLOAT?(std::isfinite(config_GetFloat(name))?QVariant(config_GetFloat(name)):QVariant{})
        :type==VLC_VAR_BOOL?QVariant(config_GetInt(name)!=0):QVariant(qlonglong(config_GetInt(name)));
}
bool clean(const QString &value,qsizetype bytes=4096)
{
    const auto utf8=value.toUtf8();if(utf8.size()>bytes||QString::fromUtf8(utf8)!=value)return false;
    for(const auto c:value)if(c.unicode()<32||c.unicode()==127)return false;
    return true;
}
bool numeric(const QVariant &value,double lo,double hi,double &out,bool integral=false)
{
    switch(value.typeId()) {
    case QMetaType::Int:case QMetaType::UInt:case QMetaType::LongLong:case QMetaType::ULongLong:
    case QMetaType::Float:case QMetaType::Double:break;
    default:return false;
    }
    // QML supplies doubles at the declared decimal bounds; native float
    // callers supply rounded float bounds. Accept each type's real boundary.
    const bool nativeFloat=value.typeId()==QMetaType::Float;
    out=value.toDouble();return std::isfinite(out)&&out>=(nativeFloat?double(float(lo)):lo)
        &&out<=(nativeFloat?double(float(hi)):hi)&&(!integral||std::floor(out)==out);
}
bool language(const QString &value)
{
    if(!clean(value,256))return false;
    if(value.isEmpty())return true;
    const auto parts=value.split(',',Qt::KeepEmptyParts);if(parts.size()>16)return false;
    for(const auto &part:parts) {
        const auto code=part.toUtf8();
        if(part!=QStringLiteral("any")&&part!=QStringLiteral("none")&&!vlc_find_iso639(code.constData(),false))return false;
    }
    return true;
}
bool folders(const QString &value)
{
    if(!clean(value)||value.contains(QStringLiteral("://")))return false;
    if(value.isEmpty())return true;
    const auto parts=value.split(',',Qt::KeepEmptyParts);if(parts.size()>16)return false;
    for(qsizetype i=0;i<parts.size();++i) {
        auto path=parts.at(i);
        // Match core's spaces-after-comma grammar, including its shipped default.
        if(i>0)while(path.startsWith(QChar(' ')))path.remove(0,1);
        if(path.isEmpty()||path!=path.trimmed()||(!QFileInfo(path).isAbsolute()&&!path.startsWith(QChar('.'))))return false;
        // Relative folders resolve against the FUTURE media directory in core.
        if(QFileInfo(path).isAbsolute()&&(!QFileInfo(path).isDir()||!QFileInfo(path).isReadable()))return false;
    }
    return true;
}
QString color(int rgb){return QStringLiteral("#%1").arg(rgb,6,16,QChar('0')).toUpper();}
bool colorInput(const QVariant &value,int &rgb)
{
    static const QRegularExpression re(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    if(value.typeId()!=QMetaType::QString||!re.match(value.toString()).hasMatch())return false;
    rgb=value.toString().mid(1).toInt(nullptr,16);return true;
}
SubtitleAppearance::Snapshot style(vlc_object_t *source)
{
    auto s=SubtitleAppearance::configuredSnapshot(source);
    if(!s.enabled) {
        s.font=text("freetype-font");s.text=config_GetInt("freetype-color");s.outline=config_GetInt("freetype-outline-color");
        s.background=config_GetInt("freetype-background-opacity");s.relativeOutline=config_GetInt("freetype-outline-thickness");
    }
    return s;
}
QVariantMap styleChanges(const SubtitleAppearance::Snapshot &s)
{
    return {{QStringLiteral("sub-text-style-enabled"),true},{QStringLiteral("sub-text-style-embedded"),s.embedded},
        {QStringLiteral("sub-text-style-font"),s.font},{QStringLiteral("sub-text-style-color"),s.text},
        {QStringLiteral("sub-text-style-outline-color"),s.outline},{QStringLiteral("sub-text-style-size-mode"),s.sizeMode},
        {QStringLiteral("sub-text-style-relative-size"),s.relativeSize},{QStringLiteral("sub-text-style-pixel-size"),s.pixelSize},
        {QStringLiteral("sub-text-style-relative-outline"),s.relativeOutline},{QStringLiteral("sub-text-style-pixel-outline"),s.pixelOutline},
        {QStringLiteral("sub-text-style-background-opacity"),s.background}};
}
QVariantList intChoices(const char *native)
{
    QVariantList choices;if(config_GetType(native)!=VLC_VAR_INTEGER)return choices;
    int64_t *values=nullptr;char **names=nullptr;const auto n=config_GetIntChoices(native,&values,&names);
    for(ssize_t i=0;i<n;++i){choices.append(QVariantMap{{QStringLiteral("value"),qlonglong(values[i])},{QStringLiteral("name"),QString::fromUtf8(names[i]?names[i]:"")}});free(names[i]);}
    free(names);free(values);return choices;
}
QVariantList stringChoices(const char *native)
{
    QVariantList choices;if(config_GetType(native)!=VLC_VAR_STRING)return choices;
    char **values=nullptr,**names=nullptr;const auto n=config_GetPszChoices(native,&values,&names);
    for(ssize_t i=0;i<n;++i){choices.append(QVariantMap{{QStringLiteral("value"),QString::fromUtf8(values[i]?values[i]:"")},{QStringLiteral("name"),QString::fromUtf8(names[i]?names[i]:"")}});free(names[i]);free(values[i]);}
    free(names);free(values);return choices;
}
bool listed(const QVariantList &choices,const QVariant &value)
{
    for(const auto &entry:choices)if(entry.toMap().value(QStringLiteral("value"))==value)return true;return false;
}
}
SubtitlesPreferencesModel::SubtitlesPreferencesModel(qt_intf_t *intf,QObject *parent)
    :SubtitlesPreferencesModel(intf,nullptr,Consumers{},parent){}
SubtitlesPreferencesModel::SubtitlesPreferencesModel(qt_intf_t *intf,SubtitlePresetModel *presets,Consumers consumers,QObject *parent)
    :QObject(parent),m_intf(intf),m_presets(presets),m_consumers(consumers)
{m_startupFingerprint=configurationFingerprint();refresh();}
SubtitlesPreferencesModel::~SubtitlesPreferencesModel()=default;
void SubtitlesPreferencesModel::setError(const QString &value){if(value!=m_error){m_error=value;emit lastErrorChanged();}}
bool SubtitlesPreferencesModel::fail(const QString &error){setError(error);return false;}
void SubtitlesPreferencesModel::reload(){refresh();setError({});}
void SubtitlesPreferencesModel::refresh()
{
    QVariantMap values,caps,choices;
    for(const auto &option:direct){const auto key=QString::fromLatin1(option.key);values[key]=raw(option.native,option.type);caps[key]=m_intf&&config_GetType(option.native)==option.type;}
    const bool renderer=module_exists("freetype");
    for(const auto &option:direct)if(strncmp(option.native,"freetype-",9)==0)caps[QString::fromLatin1(option.key)]=caps.value(QString::fromLatin1(option.key)).toBool()&&renderer;
    const auto s=style(m_intf?VLC_OBJECT(m_intf):nullptr);bool styleAvailable=m_intf&&renderer&&SubtitleAppearance::validSnapshot(s);
    for(const auto *native:styleNative)styleAvailable&=config_GetType(native)!=0;
    for(const auto *key:styleKeys)caps[QString::fromLatin1(key)]=styleAvailable;
    values[QStringLiteral("fontFamily")]=s.font;values[QStringLiteral("sizeMode")]=s.sizeMode;
    const float fontSize=s.sizeMode?s.pixelSize:s.relativeSize;
    values[QStringLiteral("fontSize")]=std::isfinite(fontSize)?QVariant(fontSize):QVariant{};
    values[QStringLiteral("textColor")]=color(s.text);values[QStringLiteral("outlineColor")]=color(s.outline);
    const float outline=s.sizeMode?s.pixelOutline:s.relativeOutline;
    values[QStringLiteral("outlineThickness")]=std::isfinite(outline)?QVariant(outline):QVariant{};
    values[QStringLiteral("background")]=s.background>0?QStringLiteral("box"):QStringLiteral("none");
    values[QStringLiteral("backgroundOpacity")]=s.background*100./255.;
    values[QStringLiteral("textOpacity")]=config_GetType("freetype-opacity")==VLC_VAR_INTEGER?config_GetInt("freetype-opacity")*100./255.:0.;
    values[QStringLiteral("shadow")]=config_GetType("freetype-shadow-opacity")==VLC_VAR_INTEGER&&config_GetInt("freetype-shadow-opacity")>0;
    for(const auto *key:{"shadowColor","backgroundColor"})for(const auto &option:direct)if(strcmp(option.key,key)==0)values[QString::fromLatin1(key)]=color(config_GetType(option.native)==VLC_VAR_INTEGER?config_GetInt(option.native):0);
    const auto delay=config_GetType("sub-delay")==VLC_VAR_INTEGER?config_GetInt("sub-delay"):0;
    values[QStringLiteral("subtitleDelay")]=delay>=LLONG_MIN/100&&delay<=LLONG_MAX/100
        ?QVariant(qlonglong(delay*100)):QVariant(QString::number(delay)+QStringLiteral("00"));
    caps[QStringLiteral("preferredLanguage")]=caps.value(QStringLiteral("preferredLanguage")).toBool()&&m_intf&&m_intf->p_player;
    caps[QStringLiteral("verticalPosition")]=caps.value(QStringLiteral("verticalPosition")).toBool()&&m_consumers.primaryAlignment;
    caps[QStringLiteral("feedbackDuration")]=caps.value(QStringLiteral("feedbackDuration")).toBool()&&m_consumers.feedbackDuration;
    const bool osd=config_GetType("osd")==VLC_VAR_BOOL&&config_GetInt("osd")!=0;
    values[QStringLiteral("showFeedback")]=osd&&values.value(QStringLiteral("showFeedback")).toBool();
    caps[QStringLiteral("showFeedback")]=caps.value(QStringLiteral("showFeedback")).toBool()&&m_consumers.shortcutFeedback&&osd;
    caps[QStringLiteral("showFilename")]=caps.value(QStringLiteral("showFilename")).toBool()&&osd;
    values[QStringLiteral("ignoreAssStyling")]=false;caps[QStringLiteral("ignoreAssStyling")]=false;
    values[QStringLiteral("visualizationSize")]=QStringLiteral("native");caps[QStringLiteral("visualizationSize")]=false;
    choices[QStringLiteral("filenameMatching")]=intChoices("sub-autodetect-fuzzy");
    choices[QStringLiteral("textEncoding")]=stringChoices("subsdec-encoding");
    choices[QStringLiteral("textDirection")]=intChoices("freetype-text-direction");
    choices[QStringLiteral("verticalPosition")]=QVariantList{
        QVariantMap{{QStringLiteral("value"),-1},{QStringLiteral("name"),tr("Native subtitle placement")}},
        QVariantMap{{QStringLiteral("value"),4},{QStringLiteral("name"),tr("Top")}},
        QVariantMap{{QStringLiteral("value"),0},{QStringLiteral("name"),tr("Center")}},
        QVariantMap{{QStringLiteral("value"),8},{QStringLiteral("name"),tr("Bottom")}}};
    choices[QStringLiteral("sizeMode")]=QVariantList{QVariantMap{{QStringLiteral("value"),0},{QStringLiteral("name"),tr("Relative")}},QVariantMap{{QStringLiteral("value"),1},{QStringLiteral("name"),tr("Pixels")}}};
    choices[QStringLiteral("background")]=QVariantList{QVariantMap{{QStringLiteral("value"),QStringLiteral("none")},{QStringLiteral("name"),tr("None")}},QVariantMap{{QStringLiteral("value"),QStringLiteral("box")},{QStringLiteral("name"),tr("Box")}}};
    QVariantList languages{QVariantMap{{QStringLiteral("value"),QString{}},{QStringLiteral("name"),tr("Automatic (native track selection)")}},
        QVariantMap{{QStringLiteral("value"),QStringLiteral("any")},{QStringLiteral("name"),tr("Any available language")}},
        QVariantMap{{QStringLiteral("value"),QStringLiteral("none")},{QStringLiteral("name"),tr("No fallback subtitle")}}};
    QSet<QString> listedCodes;
    for(char a='a';a<='z';++a)for(char b='a';b<='z';++b) {
        const char code[]={a,b,'\0'};const auto *record=vlc_find_iso639(code,false);if(!record)continue;
        const auto canonical=QString::fromLatin1(record->psz_iso639_2T);
        if(canonical.isEmpty()||listedCodes.contains(canonical))continue;
        listedCodes.insert(canonical);languages.append(QVariantMap{{QStringLiteral("value"),canonical},{QStringLiteral("name"),QString::fromUtf8(record->psz_eng_name)}});
    }
    // Keep a saved two-letter/three-letter alias or ordered list visible; do not
    // normalize an existing native profile merely to match dropdown choices.
    const auto savedLanguage=values.value(QStringLiteral("preferredLanguage")).toString();
    if(!savedLanguage.isEmpty()&&language(savedLanguage)&&!listed(languages,savedLanguage)) {
        const auto encoded=savedLanguage.toUtf8();const auto *record=vlc_find_iso639(encoded.constData(),false);
        languages.append(QVariantMap{{QStringLiteral("value"),savedLanguage},{QStringLiteral("name"),record?QString::fromUtf8(record->psz_eng_name):tr("Saved language order")}});
    }
    choices[QStringLiteral("preferredLanguage")]=languages;
    if(values!=m_values){m_values=values;emit valuesChanged();}if(caps!=m_capabilities){m_capabilities=caps;emit capabilitiesChanged();}
    if(choices!=m_choices){m_choices=choices;emit choicesChanged();}
    const bool restart=configurationFingerprint()!=m_startupFingerprint;
    if(restart!=m_restartRequired){m_restartRequired=restart;emit restartRequiredChanged();}
}
QByteArray SubtitlesPreferencesModel::configurationFingerprint()const
{
    QVariantMap state;
    for(const auto &option:direct) {
        if(strcmp(option.key,"preferredLanguage")==0||strcmp(option.key,"textEncoding")==0||strcmp(option.key,"autoLoad")==0||strcmp(option.key,"filenameMatching")==0||strcmp(option.key,"subtitleDelay")==0||strcmp(option.key,"showFeedback")==0||strcmp(option.key,"feedbackDuration")==0||strcmp(option.key,"searchFolders")==0||strcmp(option.key,"backgroundColor")==0)continue;
        state[QString::fromLatin1(option.native)]=raw(option.native,option.type);
    }
    for(const auto *native:styleNative)state[QString::fromLatin1(native)]=raw(native,config_GetType(native));
    return QCryptographicHash::hash(QJsonDocument::fromVariant(state).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
QVariantMap SubtitlesPreferencesModel::applyModes()const
{
    QVariantMap modes;for(const auto &option:direct)modes[QString::fromLatin1(option.key)]=QStringLiteral("restart");
    for(const auto *key:styleKeys)modes[QString::fromLatin1(key)]=QStringLiteral("restart");
    for(const auto *key:{"preferredLanguage","textEncoding","autoLoad","filenameMatching","subtitleDelay","searchFolders"})modes[QString::fromLatin1(key)]=QStringLiteral("nextInput");
    for(const auto *key:{"showFeedback","feedbackDuration"})modes[QString::fromLatin1(key)]=QStringLiteral("nextFeedback");
    modes[QStringLiteral("backgroundColor")]=QStringLiteral("rendererUpdate");
    modes[QStringLiteral("ignoreAssStyling")]=QStringLiteral("unsupported");modes[QStringLiteral("visualizationSize")]=QStringLiteral("unsupported");return modes;
}
QVariantMap SubtitlesPreferencesModel::supportNotes()const
{
    return {{QStringLiteral("preferredLanguage"),tr("Automatic uses VLC's native track selection, not the operating system language. A comma-separated list of ISO language codes is also accepted; any allows a fallback and none disables fallback.")},
        {QStringLiteral("appearance"),tr("Saved text-subtitle defaults apply after restarting VLC. Current-media appearance, saved presets and subtitle duration adjustments remain separate. Existing embedded formatting is preserved.")},
        {QStringLiteral("fontSize"),tr("Relative size is a percentage of video height; pixels use source video pixels. Existing subtitle scale remains independent. Native defaults are 6.25 percent and 18 pixels; no obsolete automatic-size option is written.")},
        {QStringLiteral("outlineThickness"),tr("Relative outline is a percentage of font size; pixel mode uses pixels. Both inactive unit values are retained.")},
        {QStringLiteral("background"),tr("None sets opacity to zero. Box retains a positive opacity, or starts at approximately 50 percent. Background opacity controls the same native setting.")},
        {QStringLiteral("backgroundColor"),tr("The native text renderer rereads this color while rendering. It may update existing text and on-screen messages; it does not change ASS bitmap styling.")},
        {QStringLiteral("opacity"),tr("Opacity is stored in the native 0–255 steps. The displayed percentage reflects that quantization.")},
        {QStringLiteral("verticalPosition"),tr("Default placement for relative primary subtitles after restarting VLC. Absolute subtitle positions, ASS drawings, secondary subtitles and on-screen messages retain their own placement. Requires the verified primary-placement consumer.")},
        {QStringLiteral("subtitleDelay"),tr("Default delay for the next media, in milliseconds at 100 ms steps. Current-media timing remains separate.")},
        {QStringLiteral("feedbackDuration"),tr("Milliseconds for shortcut text, icons and sliders. Native default (-1) preserves the original 1 second text and 1.2 second widget durations. Requires the verified duration consumer.")},
        {QStringLiteral("showFeedback"),tr("Controls shortcut text, icons and sliders without disabling filename messages. Requires the verified shortcut consumer. An existing disabled global On Screen Display master policy is preserved; enable that policy in Advanced before changing shortcut or filename visibility here.")},
        {QStringLiteral("searchFolders"),tr("Comma-separated local folders. Absolute folders must exist; ./subtitles and .\\subs resolve beside each future media file. Commas cannot occur inside a folder name.")},
        {QStringLiteral("ignoreAssStyling"),tr("Unavailable: native libass renders styled ASS/SSA bitmaps. Plain-text embedded formatting and the USF formatting option cannot remove ASS styling.")},
        {QStringLiteral("visualizationSize"),tr("Unavailable: this renderer derives its canvas from actual video/display dimensions. No independent subtitle visualization-size consumer is present.")}};
}
bool SubtitlesPreferencesModel::commit(const QVariantMap &changes,const QString &key)
{
    QString error;if(!CheckedPreferencesStore(m_intf).saveNative(changes,error))return fail(error);
    if(key==QStringLiteral("preferredLanguage")&&m_intf->p_player) {
        const auto value=text("sub-language").toUtf8();vlc_player_locker lock(m_intf->p_player);
        vlc_player_SelectSubtitleLanguage(m_intf->p_player,value.constData());
    }
    // Recompute only the Default preset; never reload/discard saved or live state.
    if(m_presets)m_presets->refreshDefaults();
    refresh();setError({});return true;
}
void SubtitlesPreferencesModel::profileDefaultsChanged(const QStringList &nativeKeys)
{
    if (m_intf && m_intf->p_player && nativeKeys.contains(QStringLiteral("sub-language"))) {
        const auto value=text("sub-language").toUtf8();vlc_player_locker lock(m_intf->p_player);
        vlc_player_SelectSubtitleLanguage(m_intf->p_player,value.constData());
    }
    if (m_presets) m_presets->refreshDefaults();
    refresh();setError({});
}
bool SubtitlesPreferencesModel::setValue(const QString &key,const QVariant &value)
{
    refresh();if(!m_intf||!m_capabilities.value(key).toBool())return fail(tr("This subtitle setting is unavailable in the current native build."));
    bool isStyle=false;for(const auto *name:styleKeys)isStyle|=key==QString::fromLatin1(name);
    if(isStyle) {
        auto s=style(VLC_OBJECT(m_intf));double n=0;int rgb=0;
        if(key==QStringLiteral("fontFamily")) {
            if(value.typeId()!=QMetaType::QString||!clean(value.toString(),256))return fail(tr("Enter a valid font family."));s.font=value.toString();
        } else if(key==QStringLiteral("sizeMode")) {
            if(!numeric(value,0,1,n,true))return fail(tr("Select Relative or Pixels."));s.sizeMode=int(n);
        } else if(key==QStringLiteral("fontSize")) {
            if(!numeric(value,s.sizeMode?1:.1,s.sizeMode?512:50,n))return fail(tr("Font size is outside the native range."));(s.sizeMode?s.pixelSize:s.relativeSize)=float(n);
        } else if(key==QStringLiteral("outlineThickness")) {
            if(!numeric(value,0,50,n))return fail(tr("Outline thickness must be between 0 and 50."));(s.sizeMode?s.pixelOutline:s.relativeOutline)=float(n);
        } else if(key==QStringLiteral("textColor")||key==QStringLiteral("outlineColor")) {
            if(!colorInput(value,rgb))return fail(tr("Use a six-digit RGB color such as #FFFFFF."));(key==QStringLiteral("textColor")?s.text:s.outline)=rgb;
        } else if(key==QStringLiteral("background")) {
            if(value.typeId()!=QMetaType::QString||(value.toString()!=QStringLiteral("none")&&value.toString()!=QStringLiteral("box")))return fail(tr("Select None or Box."));
            s.background=value.toString()==QStringLiteral("none")?0:s.background>0?s.background:128;
        } else {
            if(!numeric(value,0,100,n))return fail(tr("Opacity must be between 0 and 100 percent."));s.background=qRound(n*255./100.);
        }
        if(!SubtitleAppearance::validSnapshot(s))return fail(tr("Existing subtitle defaults require correction in Advanced before editing appearance."));
        auto changes=styleChanges(s);
        if(key==QStringLiteral("fontFamily"))changes[QStringLiteral("freetype-font")]=s.font;
        return commit(changes,key);
    }
    const Option *option=nullptr;for(const auto &item:direct)if(key==QString::fromLatin1(item.key)){option=&item;break;}
    if(!option)return fail(tr("Unknown subtitle setting."));
    QVariant native;double n=0;int rgb=0;
    if(option->type==VLC_VAR_BOOL) {
        if(value.typeId()!=QMetaType::Bool)return fail(tr("This setting requires a true or false value."));native=value;
    } else if(key==QStringLiteral("shadow")) {
        if(value.typeId()!=QMetaType::Bool)return fail(tr("Shadow requires a true or false value."));
        native=qlonglong(value.toBool()?(config_GetInt(option->native)>0?config_GetInt(option->native):128):0);
    } else if(key==QStringLiteral("textOpacity")) {
        if(!numeric(value,0,100,n))return fail(tr("Opacity must be between 0 and 100 percent."));native=qRound(n*255./100.);
    } else if(key==QStringLiteral("shadowColor")||key==QStringLiteral("backgroundColor")) {
        if(!colorInput(value,rgb))return fail(tr("Use a six-digit RGB color."));native=rgb;
    } else if(option->type==VLC_VAR_FLOAT) {
        if(!numeric(value,key==QStringLiteral("shadowAngle")?-360:0,key==QStringLiteral("shadowAngle")?360:1,n))return fail(tr("Shadow value is outside the native range."));native=QVariant::fromValue(float(n));
    } else if(option->type==VLC_VAR_STRING) {
        if(value.typeId()!=QMetaType::QString||!clean(value.toString()))return fail(tr("Enter valid text without control characters."));
        const auto s=value.toString();
        if(key==QStringLiteral("preferredLanguage")&&!language(s))return fail(tr("Use ISO language codes separated by commas."));
        if(key==QStringLiteral("textEncoding")&&!s.isEmpty()&&!listed(m_choices.value(key).toList(),s))return fail(tr("Select a supported native text encoding."));
        if(key==QStringLiteral("searchFolders")&&!folders(s))return fail(tr("Use existing absolute folders or ./relative folders, separated by commas."));
        if(key==QStringLiteral("monospaceFont")&&!clean(s,256))return fail(tr("Enter a valid monospace font family."));
#ifdef _WIN32
        if(key==QStringLiteral("searchFolders")) {
            // Core converts each discovered path to a URI using native separators.
            // Keep comma grammar, whitespace and Unicode exactly; normalize paths only.
            auto paths=s.split(QChar(','),Qt::KeepEmptyParts);
            for(auto &path:paths)path=QDir::toNativeSeparators(path);
            native=paths.join(QChar(','));
        } else
#endif
            native=s;
    } else {
        double lo=0,hi=0;
        if(key==QStringLiteral("filenameMatching")){lo=0;hi=4;}
        else if(key==QStringLiteral("textDirection")){lo=0;hi=2;}
        else if(key==QStringLiteral("verticalPosition")){lo=-1;hi=8;}
        else if(key==QStringLiteral("subtitleDelay")){lo=-5000;hi=5000;}
        else if(key==QStringLiteral("feedbackDuration")){lo=500;hi=5000;}
        else if(key==QStringLiteral("filenameDuration")){lo=1000;hi=10000;}
        else return fail(tr("Unsupported integer setting."));
        if(!numeric(value,lo,hi,n,true))return fail(tr("Value is outside this setting's range."));
        if(key==QStringLiteral("verticalPosition")&&n!=-1&&n!=0&&n!=4&&n!=8)return fail(tr("Select a native subtitle position."));
        if(key==QStringLiteral("subtitleDelay")&&qlonglong(n)%100)return fail(tr("Subtitle delay uses 100 ms steps."));
        native=qlonglong(key==QStringLiteral("subtitleDelay")?n/100:n);
    }
    return commit({{QString::fromLatin1(option->native),native}},key);
}
