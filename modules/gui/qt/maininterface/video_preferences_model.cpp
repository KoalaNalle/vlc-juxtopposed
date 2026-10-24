// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "video_preferences_model.hpp"
#include "qt.hpp"
// NEW isolated float-compatible helper proposal shared with Global Audio.
// The accepted General/Interface store remains unchanged.
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_vout_display.h>
#include <vlc_cxx_helpers.hpp>
#include <QByteArrayList>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <cmath>
#include <climits>
#include <cstring>
#include <utility>

namespace {
struct Option { const char *key, *native; int type; };
constexpr Option options[] = {
    {"enableVideo","video",VLC_VAR_BOOL}, {"startFullscreen","fullscreen",VLC_VAR_BOOL},
    {"disableScreensaver","disable-screensaver",VLC_VAR_INTEGER}, {"wallpaper","video-wallpaper",VLC_VAR_BOOL},
    {"hue","hue",VLC_VAR_FLOAT}, {"brightness","brightness",VLC_VAR_FLOAT},
    {"contrast","contrast",VLC_VAR_FLOAT}, {"saturation","saturation",VLC_VAR_FLOAT}, {"gamma","gamma",VLC_VAR_FLOAT},
    {"deinterlace","deinterlace",VLC_VAR_INTEGER}, {"deinterlaceMode","deinterlace-mode",VLC_VAR_STRING},
    {"aspectRatio","aspect-ratio",VLC_VAR_STRING}, {"crop","crop",VLC_VAR_STRING},
    {"scaleToWindow","autoscale",VLC_VAR_BOOL}, {"zoom","zoom",VLC_VAR_FLOAT}, {"position","align",VLC_VAR_INTEGER},
    {"snapshotDirectory","snapshot-path",VLC_VAR_STRING}, {"snapshotPrefix","snapshot-prefix",VLC_VAR_STRING},
    {"snapshotFormat","snapshot-format",VLC_VAR_STRING}, {"snapshotSize","snapshot-width",VLC_VAR_INTEGER},
    {"snapshotSequential","snapshot-sequential",VLC_VAR_BOOL}, {"snapshotPreview","snapshot-preview",VLC_VAR_BOOL},
    {"customAspectRatios","custom-aspect-ratios",VLC_VAR_STRING}, {"customCropRatios","custom-crop-ratios",VLC_VAR_STRING},
    {"monitorPixelAspectRatio","monitor-par",VLC_VAR_STRING}, {"timeShiftBufferSize","input-timeshift-granularity",VLC_VAR_INTEGER}
};
constexpr const char *pictureKeys[] = {"hue","brightness","contrast","saturation","gamma"};
bool pictureValue(const QString &key,float value)
{
    const float lo=key==QStringLiteral("hue")?-180.f:key==QStringLiteral("gamma")?.01f:0.f;
    const float hi=key==QStringLiteral("hue")?180.f:key==QStringLiteral("gamma")?10.f:key==QStringLiteral("saturation")?3.f:2.f;
    return std::isfinite(value)&&value>=lo&&value<=hi;
}
QString nativeText(const char *name)
{
    if (config_GetType(name)!=VLC_VAR_STRING) return {};
    char *raw=config_GetPsz(name);const auto value=QString::fromUtf8(raw?raw:"");free(raw);return value;
}
bool validText(const QString &text, qsizetype maximum=4096)
{
    const auto utf8=text.toUtf8();
    if (utf8.size()>maximum || QString::fromUtf8(utf8)!=text) return false;
    for (auto ch:text) if (ch.unicode()<32 || ch.unicode()==127) return false;
    return true;
}
bool integer(const QVariant &input,qlonglong lo,qlonglong hi,qlonglong &out)
{
    switch(input.typeId()) {
    case QMetaType::Int:case QMetaType::UInt:case QMetaType::LongLong:case QMetaType::ULongLong:
    case QMetaType::Float:case QMetaType::Double:break;
    default:return false;
    }
    const double n=input.toDouble();
    if (!std::isfinite(n)||std::floor(n)!=n||n<lo||n>hi) return false;
    out=qlonglong(n);return true;
}
bool number(const QVariant &input,double lo,double hi,float &out)
{
    switch(input.typeId()) {
    case QMetaType::Int:case QMetaType::UInt:case QMetaType::LongLong:case QMetaType::ULongLong:
    case QMetaType::Float:case QMetaType::Double:break;
    default:return false;
    }
    // Native bounds are floats too: QVariant(float(.01)) must not be rejected
    // merely because its exact binary value lies below the decimal .01 double.
    const double n=input.toDouble();if(!std::isfinite(n)||n<double(float(lo))||n>double(float(hi)))return false;
    out=float(n);return std::isfinite(out);
}
bool ratio(const QString &text,bool empty=true)
{
    if (empty&&text.isEmpty())return true;
    static const QRegularExpression pattern(QStringLiteral("^[1-9][0-9]{0,5}:[1-9][0-9]{0,5}$"));
    return pattern.match(text).hasMatch();
}
bool ratios(const QString &text)
{
    if (text.isEmpty())return true;
    const auto entries=text.split(',',Qt::KeepEmptyParts);
    if(entries.size()>16)return false;
    for(const auto &entry:entries)if(!ratio(entry,false))return false;
    return true;
}
QVariantList nativeChoices(const char *native,int type)
{
    QVariantList result;
    if(config_GetType(native)!=type)return result;
    char **names=nullptr;
    if(type==VLC_VAR_INTEGER) {
        int64_t *values=nullptr;const auto count=config_GetIntChoices(native,&values,&names);
        for(ssize_t i=0;i<count;++i){result.append(QVariantMap{{QStringLiteral("value"),qlonglong(values[i])},{QStringLiteral("name"),QString::fromUtf8(names[i]?names[i]:"")}});free(names[i]);}
        free(values);
    } else {
        char **values=nullptr;const auto count=config_GetPszChoices(native,&values,&names);
        for(ssize_t i=0;i<count;++i){result.append(QVariantMap{{QStringLiteral("value"),QString::fromUtf8(values[i]?values[i]:"")},{QStringLiteral("name"),QString::fromUtf8(names[i]?names[i]:"")}});free(values[i]);free(names[i]);}
        free(values);
    }
    free(names);return result;
}
bool listed(const QVariantList &choices,const QVariant &value)
{
    for(const auto &item:choices)if(item.toMap().value(QStringLiteral("value"))==value)return true;
    return false;
}
QVariantList ratioChoices(const QString &custom,bool crop)
{
    QStringList values=crop?QStringList{QString{},QStringLiteral("16:10"),QStringLiteral("16:9"),QStringLiteral("4:3"),QStringLiteral("185:100"),QStringLiteral("221:100"),QStringLiteral("235:100"),QStringLiteral("239:100"),QStringLiteral("5:3"),QStringLiteral("5:4"),QStringLiteral("1:1")}
        :QStringList{QString{},QStringLiteral("16:9"),QStringLiteral("4:3"),QStringLiteral("1:1"),QStringLiteral("16:10"),QStringLiteral("221:100"),QStringLiteral("235:100"),QStringLiteral("239:100"),QStringLiteral("5:4"),QStringLiteral("fill")};
    if(ratios(custom))for(const auto &entry:custom.split(',',Qt::SkipEmptyParts))if(!values.contains(entry))values.append(entry);
    QVariantList choices;
    for(const auto &value:values)choices.append(QVariantMap{{QStringLiteral("value"),value},{QStringLiteral("name"),value.isEmpty()?QObject::tr("Automatic"):value==QStringLiteral("fill")?QObject::tr("Fill window"):value}});
    return choices;
}
struct PictureChain {bool editable=true;QByteArray enabled,removed;QVariantMap effective;};
PictureChain pictureChain()
{
    PictureChain result;QByteArrayList enabled,removed;
    for(const auto *key:pictureKeys)if(config_GetType(key)==VLC_VAR_FLOAT)result.effective[QString::fromLatin1(key)]=config_GetFloat(key);
    auto remaining=vlc::wrap_cptr(config_GetPsz("video-filter"));unsigned adjusts=0;
    while(remaining&&*remaining) {
        char *name=nullptr;config_chain_t *opts=nullptr;
        auto next=vlc::wrap_cptr(config_ChainCreate(&name,&opts,remaining.get()));
        const size_t length=strlen(remaining.get())-(next?strlen(next.get())+1:0);
        const QByteArray original(remaining.get(),length);
        if(name&&strcmp(name,"adjust")==0) {
            if(++adjusts>1)result.editable=false;
            for(auto *entry=opts;entry;entry=entry->p_next) {
                const auto key=QString::fromUtf8(entry->psz_name?entry->psz_name:"");bool known=false;
                for(const auto *owned:pictureKeys)known|=key==QString::fromLatin1(owned);
                bool ok=false;const auto value=QString::fromUtf8(entry->psz_value?entry->psz_value:"").toFloat(&ok);
                if(!known||!ok||!pictureValue(key,value))result.editable=false;
                else result.effective[key]=value;
            }
            enabled.append("adjust");
        } else {enabled.append(original);removed.append(original);}
        free(name);config_ChainDestroy(opts);remaining=std::move(next);
    }
    if(!adjusts)enabled.append("adjust");
    for(auto it=result.effective.cbegin();it!=result.effective.cend();++it)
        if(!pictureValue(it.key(),it.value().toFloat()))result.editable=false;
    result.enabled=enabled.join(':');result.removed=removed.join(':');return result;
}
}

VideoPreferencesModel::VideoPreferencesModel(qt_intf_t *intf,QObject *parent)
    :VideoPreferencesModel(intf,Consumers{},parent){}
VideoPreferencesModel::VideoPreferencesModel(qt_intf_t *intf,Consumers consumers,QObject *parent)
    :QObject(parent),m_intf(intf),m_consumers(consumers){m_startupFingerprint=configurationFingerprint();refresh();}
VideoPreferencesModel::~VideoPreferencesModel()=default;
void VideoPreferencesModel::setError(const QString &error){if(error!=m_error){m_error=error;emit lastErrorChanged();}}
bool VideoPreferencesModel::fail(const QString &error){setError(error);return false;}
int VideoPreferencesModel::nativeScreensaverMode()const
{
    if(config_GetType("disable-screensaver")!=VLC_VAR_INTEGER)return -1;
    const auto mode=config_GetInt("disable-screensaver");
    return mode==0||mode==1||mode==2?int(mode):-1;
}
void VideoPreferencesModel::refresh()
{
    QVariantMap values,caps,choices;
    for(const auto &option:options) {
        const auto key=QString::fromLatin1(option.key);const bool present=config_GetType(option.native)==option.type;
        caps[key]=m_intf&&present;
        values[key]=option.type==VLC_VAR_STRING?QVariant(nativeText(option.native))
            :option.type==VLC_VAR_FLOAT?QVariant(present?config_GetFloat(option.native):0.f)
            :option.type==VLC_VAR_BOOL?QVariant(present&&config_GetInt(option.native)!=0)
            :QVariant(qlonglong(present?config_GetInt(option.native):-1));
    }
    const auto chain=pictureChain();bool picture=m_intf&&module_exists("adjust")&&config_GetType("video-filter")==VLC_VAR_STRING&&chain.editable;
    for(const auto *key:pictureKeys)picture&=config_GetType(key)==VLC_VAR_FLOAT;
    for(const auto *key:pictureKeys){values[QString::fromLatin1(key)]=chain.effective.value(QString::fromLatin1(key));caps[QString::fromLatin1(key)]=picture;}
    caps[QStringLiteral("resetPicture")]=picture;
    values[QStringLiteral("disableScreensaver")]=nativeScreensaverMode()>0;
    caps[QStringLiteral("disableScreensaver")]=caps.value(QStringLiteral("disableScreensaver")).toBool()
        &&(nativeScreensaverMode()==0||nativeScreensaverMode()==1||nativeScreensaverMode()==2);
#ifdef _WIN32
    caps[QStringLiteral("wallpaper")]=caps.value(QStringLiteral("wallpaper")).toBool()&&m_consumers.desktopWallpaper&&module_exists("win32_window");
#else
    caps[QStringLiteral("wallpaper")]=false;
#endif
    const bool deinterlace=module_exists("deinterlace");
    caps[QStringLiteral("deinterlace")]=caps.value(QStringLiteral("deinterlace")).toBool()&&deinterlace;
    caps[QStringLiteral("deinterlaceMode")]=caps.value(QStringLiteral("deinterlaceMode")).toBool()&&deinterlace;
    for(const auto *key:{"deinterlace","position"})choices[QString::fromLatin1(key)]=nativeChoices(strcmp(key,"position")==0?"align":key,VLC_VAR_INTEGER);
    choices[QStringLiteral("deinterlaceMode")]=nativeChoices("deinterlace-mode",VLC_VAR_STRING);
    QVariantList formats;
    for(const auto &item:nativeChoices("snapshot-format",VLC_VAR_STRING)) {
        const auto name=item.toMap().value(QStringLiteral("value")).toString();
        if((name==QStringLiteral("png")&&module_exists("png"))||(name==QStringLiteral("jpg")&&module_exists("jpeg")))formats.append(item);
    }
    choices[QStringLiteral("snapshotFormat")]=formats;
    caps[QStringLiteral("snapshotFormat")]=caps.value(QStringLiteral("snapshotFormat")).toBool()&&!formats.isEmpty();
    choices[QStringLiteral("aspectRatio")]=ratioChoices(nativeText("custom-aspect-ratios"),false);
    choices[QStringLiteral("crop")]=ratioChoices(nativeText("custom-crop-ratios"),true);
    QVariantList zooms;
    for(const auto &pair:{std::pair{"fit","Fit"},std::pair{"0.25","25%"},std::pair{"0.5","50%"},std::pair{"1","100%"},std::pair{"2","200%"}})
        zooms.append(QVariantMap{{QStringLiteral("value"),QString::fromLatin1(pair.first)},{QStringLiteral("name"),tr(pair.second)}});
    choices[QStringLiteral("zoom")]=zooms;
    const bool zoomTypes=config_GetType("zoom")==VLC_VAR_FLOAT&&config_GetType("fit")==VLC_VAR_INTEGER&&config_GetType("autoscale")==VLC_VAR_BOOL;
    caps[QStringLiteral("zoom")]=m_intf&&zoomTypes;
    if(zoomTypes)values[QStringLiteral("zoom")]=config_GetInt("autoscale")
        ?(config_GetInt("fit")==VLC_VIDEO_FIT_SMALLER&&config_GetFloat("zoom")==1.f?QStringLiteral("fit"):QStringLiteral("custom"))
        :QString::number(config_GetFloat("zoom"),'g',9);
    const qlonglong width=config_GetType("snapshot-width")==VLC_VAR_INTEGER?config_GetInt("snapshot-width"):-1;
    const qlonglong height=config_GetType("snapshot-height")==VLC_VAR_INTEGER?config_GetInt("snapshot-height"):-1;
    values[QStringLiteral("snapshotSize")]=width==-1&&height==-1?QStringLiteral("source"):QString::number(width)+QLatin1Char('x')+QString::number(height);
    caps[QStringLiteral("snapshotSize")]=caps.value(QStringLiteral("snapshotSize")).toBool()&&config_GetType("snapshot-height")==VLC_VAR_INTEGER;
    QVariantList sizes;
    for(const auto *size:{"source","640x360","1280x720","1920x1080"})sizes.append(QVariantMap{{QStringLiteral("value"),QString::fromLatin1(size)},{QStringLiteral("name"),strcmp(size,"source")==0?tr("Same as video"):QString::fromLatin1(size)}});
    choices[QStringLiteral("snapshotSize")]=sizes;
    QVariantList chunks;
    for(const qlonglong size:{-1ll,16ll*1024*1024,50ll*1024*1024,100ll*1024*1024})chunks.append(QVariantMap{{QStringLiteral("value"),size},{QStringLiteral("name"),size<0?tr("Native default (50 MiB per file)"):tr("%1 MiB per file").arg(size/(1024*1024))}});
    choices[QStringLiteral("timeShiftBufferSize")]=chunks;
    if(choices!=m_choices){m_choices=choices;emit choicesChanged();}
    if(caps!=m_capabilities){m_capabilities=caps;emit capabilitiesChanged();}
    if(values!=m_values){m_values=values;emit valuesChanged();}
    const bool restart=configurationFingerprint()!=m_startupFingerprint;
    if(restart!=m_restartRequired){m_restartRequired=restart;emit restartRequiredChanged();}
}
QByteArray VideoPreferencesModel::configurationFingerprint()const
{
    QVariantMap state;
    for(const auto &option:options) {
        if(strncmp(option.key,"snapshot",8)==0||strcmp(option.key,"enableVideo")==0||strcmp(option.key,"timeShiftBufferSize")==0)continue;
        const auto key=QString::fromLatin1(option.native);
        state[key]=option.type==VLC_VAR_STRING?QVariant(nativeText(option.native))
            :option.type==VLC_VAR_FLOAT?QVariant(config_GetType(option.native)==VLC_VAR_FLOAT?config_GetFloat(option.native):0.f)
            :QVariant(qlonglong(config_GetType(option.native)==option.type?config_GetInt(option.native):-1));
    }
    state[QStringLiteral("video-filter")]=nativeText("video-filter");
    state[QStringLiteral("fit")]=qlonglong(config_GetType("fit")==VLC_VAR_INTEGER?config_GetInt("fit"):-1);
    return QCryptographicHash::hash(QJsonDocument::fromVariant(state).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
QVariantMap VideoPreferencesModel::applyModes()const
{
    // The main player creates a dummy vout and real outputs inherit its cached
    // defaults. Merely saving config cannot promise a new output sees changes.
    QVariantMap result;for(const auto &option:options)result[QString::fromLatin1(option.key)]=QStringLiteral("restart");
    result[QStringLiteral("enableVideo")]=QStringLiteral("nextInput");
    result[QStringLiteral("timeShiftBufferSize")]=QStringLiteral("nextInput");
    for(const auto *key:{"snapshotDirectory","snapshotPrefix","snapshotFormat","snapshotSize","snapshotSequential","snapshotPreview"})result[QString::fromLatin1(key)]=QStringLiteral("nextSnapshot");
    result[QStringLiteral("resetPicture")]=QStringLiteral("restart");return result;
}
QVariantMap VideoPreferencesModel::supportNotes()const
{
    return {{QStringLiteral("enableVideo"),tr("Default for the next input. It does not stop an already playing video's output.")},
        {QStringLiteral("startFullscreen"),tr("Default after restarting VLC. Current fullscreen controls remain separate.")},
        {QStringLiteral("disableScreensaver"),nativeScreensaverMode()<0?tr("An unsupported native screensaver policy is preserved. Edit it in Advanced before using this switch."):nativeScreensaverMode()==2?tr("The existing native policy is fullscreen only. Turning this switch on explicitly selects all video playback; off selects never. Restart VLC after changing it."):tr("Uses the native window's playback power-inhibition policy after restarting VLC. It does not change Windows security settings.")},
        {QStringLiteral("wallpaper"),tr("Uses the native Windows desktop-parented video window, not a wallpaper image file. Requires verified native desktop-window support and restarting VLC.")},
        {QStringLiteral("picture"),tr("Saved picture defaults use the actual adjust filter after restarting VLC. Other filters are preserved. Multiple adjust stages or unknown inline options must be edited in Advanced.")},
        {QStringLiteral("resetPicture"),tr("Restore the five native picture defaults and remove their adjust stage, preserving unrelated filters. Restart VLC to apply them.")},
        {QStringLiteral("framing"),tr("Saved defaults apply after restarting VLC. Fit enables inside-window scaling; a numeric zoom selects native-size scaling and turns automatic window scaling off.")},
        {QStringLiteral("snapshotSize"),tr("Requested output dimensions for the next native snapshot. Same as video removes both dimension overrides. Available encoding modules can limit formats.")},
        {QStringLiteral("snapshotPreview"),tr("The native snapshot preview is a short picture-in-picture overlay in the video output, not a new dialog. It requires visible video output; dummy/headless output cannot prove its appearance.")},
        {QStringLiteral("customAspectRatios"),tr("Comma-separated positive integer ratios, for example 16:9,21:9. Added to native aspect choices after restarting VLC.")},
        {QStringLiteral("customCropRatios"),tr("Comma-separated positive integer ratios, for example 16:9,4:3. Added to native crop choices after restarting VLC.")},
        {QStringLiteral("monitorPixelAspectRatio"),tr("Pixel aspect ratio after restarting VLC, for example 1:1. Empty keeps the native default.")},
        {QStringLiteral("timeShiftBufferSize"),tr("Maximum size of each temporary time-shift file, in bytes. This is the same setting as Playback's time-shift chunk size; it does not limit the total buffered duration or disk use. Next input only.")}};
}
void VideoPreferencesModel::reload(){if(m_pickerActive){fail(tr("Finish choosing the snapshot folder first."));return;}setError({});refresh();}
bool VideoPreferencesModel::commit(const QVariantMap &changes)
{
    QString error;if(!CheckedPreferencesStore(m_intf).saveNative(changes,error))return fail(error);
    refresh();return true;
}
bool VideoPreferencesModel::setValue(const QString &key,const QVariant &input)
{
    setError({});if(m_pickerActive)return fail(tr("Finish choosing the snapshot folder first."));
    const Option *option=nullptr;for(const auto &candidate:options)if(key==QString::fromLatin1(candidate.key)){option=&candidate;break;}
    if(!option)return fail(tr("Unknown Video preference."));
    refresh();if(!m_capabilities.value(key).toBool())return fail(tr("This Video preference is unavailable in this build or requires native verification."));
    QVariantMap changes;
    const auto native=QString::fromLatin1(option->native);
    if(key==QStringLiteral("disableScreensaver")) {
        if(input.typeId()!=QMetaType::Bool)return fail(tr("This preference requires an on or off value."));
        changes[native]=qlonglong(input.toBool()?1:0);
    } else if(key==QStringLiteral("zoom")) {
        if(input.typeId()!=QMetaType::QString||!listed(m_choices.value(key).toList(),input))return fail(tr("Choose Fit or a supported zoom percentage."));
        const bool fit=input.toString()==QStringLiteral("fit");
        changes[native]=fit?1.f:input.toString().toFloat();changes[QStringLiteral("autoscale")]=fit;
        if(fit)changes[QStringLiteral("fit")]=qlonglong(VLC_VIDEO_FIT_SMALLER);
    } else if(option->type==VLC_VAR_FLOAT) {
        float value;const double lo=key==QStringLiteral("hue")?-180.:key==QStringLiteral("gamma")?.01:0.;
        const double hi=key==QStringLiteral("hue")?180.:key==QStringLiteral("gamma")?10.:key==QStringLiteral("saturation")?3.:2.;
        if(!number(input,lo,hi,value))return fail(tr("Enter a finite picture value in the native supported range."));
        const auto chain=pictureChain();changes=chain.effective;changes[native]=value;
        changes[QStringLiteral("video-filter")]=QString::fromUtf8(chain.enabled);
    } else if(key==QStringLiteral("snapshotSize")) {
        if(input.typeId()!=QMetaType::QString)return fail(tr("Choose Same as video or width x height."));
        const auto value=input.toString();qlonglong width=-1,height=-1;
        if(value!=QStringLiteral("source")) {
            const auto parts=value.split('x');bool wok=false,hok=false;
            if(parts.size()==2){width=parts[0].toLongLong(&wok);height=parts[1].toLongLong(&hok);}
            if(!wok||!hok||width<1||height<1||width>16384||height>16384||width*height>67108864)return fail(tr("Use positive dimensions up to 16384, with at most 64 megapixels."));
        }
        changes[QStringLiteral("snapshot-width")]=width;changes[QStringLiteral("snapshot-height")]=height;
    } else if(option->type==VLC_VAR_BOOL) {
        if(input.typeId()!=QMetaType::Bool)return fail(tr("This preference requires an on or off value."));
        changes[native]=input;
    } else if(option->type==VLC_VAR_INTEGER) {
        qlonglong value;if(!integer(input,-1,INT_MAX,value))return fail(tr("Enter a whole number in the native supported range."));
        if(m_choices.contains(key)&&key!=QStringLiteral("timeShiftBufferSize")&&!listed(m_choices.value(key).toList(),QVariant(value)))return fail(tr("Choose one of this build's supported values."));
        if(key==QStringLiteral("timeShiftBufferSize")&&value!=-1&&value<1024*1024)return fail(tr("Time-shift files must be at least 1 MiB, or use -1 for the native default."));
        changes[native]=value;
    } else {
        if(input.typeId()!=QMetaType::QString||!validText(input.toString()))return fail(tr("Enter valid UTF-8 text without control characters."));
        const auto value=input.toString();
        if(key==QStringLiteral("snapshotDirectory")) {
            const QFileInfo info(value);if(!value.isEmpty()&&(!info.isAbsolute()||!info.isDir()||!info.isReadable()||!info.isWritable()||value.contains(QStringLiteral("://"))))return fail(tr("Choose an existing writable local snapshot folder, or leave it empty."));
        } else if(key==QStringLiteral("snapshotPrefix")) {
            if(!validText(value,256)||value.contains(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]"))))return fail(tr("Use a filename prefix without path separators or reserved filename characters."));
        } else if(key==QStringLiteral("customAspectRatios")||key==QStringLiteral("customCropRatios")) {
            if(!ratios(value))return fail(tr("Use up to 16 comma-separated ratios such as 16:9,21:9, without spaces."));
        } else if(key==QStringLiteral("aspectRatio")||key==QStringLiteral("crop")||key==QStringLiteral("monitorPixelAspectRatio")) {
            if(!(key==QStringLiteral("aspectRatio")&&value==QStringLiteral("fill"))&&!ratio(value))return fail(tr("Use a positive integer ratio such as 16:9, or leave it empty."));
        } else if(!listed(m_choices.value(key).toList(),input))return fail(tr("Choose one of this build's supported values."));
        changes[native]=value;
    }
    return commit(changes);
}
bool VideoPreferencesModel::resetPicture()
{
    setError({});if(m_pickerActive)return fail(tr("Finish choosing the snapshot folder first."));refresh();
    if(!m_capabilities.value(QStringLiteral("resetPicture")).toBool())return fail(tr("The native picture filter is unavailable or has a complex chain."));
    QVariantMap changes;for(const auto *key:pictureKeys)changes[QString::fromLatin1(key)]=strcmp(key,"hue")==0?0.f:1.f;
    changes[QStringLiteral("video-filter")]=QString::fromUtf8(pictureChain().removed);return commit(changes);
}
bool VideoPreferencesModel::chooseSnapshotDirectory()
{
    setError({});if(m_pickerActive)return fail(tr("A snapshot folder chooser is already open."));refresh();
    if(!m_capabilities.value(QStringLiteral("snapshotDirectory")).toBool()||!qobject_cast<QApplication *>(QCoreApplication::instance()))return fail(tr("The native snapshot folder chooser is unavailable in this session."));
    m_pickerActive=true;emit pickerActiveChanged();QPointer<VideoPreferencesModel> lifetime(this);
    const auto selected=QFileDialog::getExistingDirectory(nullptr,tr("Choose snapshot folder"),m_values.value(QStringLiteral("snapshotDirectory")).toString());
    if(!lifetime)return false;m_pickerActive=false;emit pickerActiveChanged();if(selected.isEmpty())return true;
    return setValue(QStringLiteral("snapshotDirectory"),selected);
}
