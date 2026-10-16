// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "library_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include <vlc_configuration.h>
#include <vlc_media_library.h>
#include <vlc_modules.h>
#include <QCryptographicHash>
#include <QJsonDocument>
namespace {
bool has(const char *name,int type){return config_GetType(name)==type;}
bool boolean(const char *name){return has(name,VLC_VAR_BOOL)&&config_GetInt(name)!=0;}
QString string(const char *name)
{if(!has(name,VLC_VAR_STRING))return {};char *raw=config_GetPsz(name);const auto value=QString::fromUtf8(raw?raw:"");free(raw);return value;}
int policy(const char *name){return has(name,VLC_VAR_INTEGER)?config_GetInt(name):-1;}
QVariantList recursionChoices()
{
    QVariantList rows;if(!has("recursive",VLC_VAR_STRING))return rows;
    char **values=nullptr,**names=nullptr;const auto count=config_GetPszChoices("recursive",&values,&names);
    for(ssize_t i=0;i<count;++i) {
        const auto value=QString::fromUtf8(values[i]?values[i]:"");
        const auto name=value==QStringLiteral("none")?QObject::tr("Do not expand folders automatically")
            :value==QStringLiteral("collapse")?QObject::tr("Expand opened folders only")
            :value==QStringLiteral("expand")?QObject::tr("Include subfolders"):QString::fromUtf8(names[i]?names[i]:"");
        rows.append(QVariantMap{{QStringLiteral("value"),value},{QStringLiteral("name"),name}});free(values[i]);free(names[i]);
    }
    free(values);free(names);return rows;
}
bool listed(const QVariantList &choices,const QVariant &value)
{for(const auto &entry:choices)if(entry.toMap().value(QStringLiteral("value"))==value)return true;return false;}
bool text(const QVariant &value)
{
    if(value.typeId()!=QMetaType::QString)return false;const auto input=value.toString();const auto utf8=input.toUtf8();
    if(utf8.size()>64||QString::fromUtf8(utf8)!=input)return false;
    for(const auto c:input)if(c.unicode()<32||c.unicode()==127)return false;return true;
}
bool fileManagerPreference()
{
    const int saved=policy("one-instance-file-manager-policy");
    return saved==0?false:saved==1?true:boolean("one-instance")||boolean("one-instance-when-started-from-file");
}
}
LibraryPreferencesModel::LibraryPreferencesModel(qt_intf_t *intf,QObject *parent)
    :LibraryPreferencesModel(intf,Consumers{},parent){}
LibraryPreferencesModel::LibraryPreferencesModel(qt_intf_t *intf,Consumers consumers,QObject *parent)
    :QObject(parent),m_intf(intf),m_consumers(consumers){m_startup=fingerprint();refresh();}
void LibraryPreferencesModel::setError(const QString &value){if(m_error!=value){m_error=value;emit lastErrorChanged();}}
bool LibraryPreferencesModel::fail(const QString &value){setError(value);return false;}
bool LibraryPreferencesModel::libraryAvailable()const{return m_intf&&vlc_ml_instance_get(m_intf)!=nullptr;}
int LibraryPreferencesModel::nativeFileManagerPolicy()const{return m_intf?policy("one-instance-file-manager-policy"):-1;}
int LibraryPreferencesModel::nativeOpenMode()const{return m_intf?policy("playlist-open-mode"):-1;}
bool LibraryPreferencesModel::legacyFileManagerOnly()const
{const int saved=nativeFileManagerPolicy();return m_intf&&saved!=0&&saved!=1&&!boolean("one-instance")&&boolean("one-instance-when-started-from-file");}
QVariantMap LibraryPreferencesModel::applyModes()const
{
    return {{QStringLiteral("singleInstance"),QStringLiteral("restart")},{QStringLiteral("fileManagerSingleInstance"),QStringLiteral("restart")},
        {QStringLiteral("enqueueInSingleInstance"),QStringLiteral("nextExternalOpen")},{QStringLiteral("folderRecursion"),QStringLiteral("restart")},
        {QStringLiteral("showHiddenFiles"),QStringLiteral("nextFolderOpen")},{QStringLiteral("metadataSource"),QStringLiteral("restart")},
        {QStringLiteral("useMediaLibrary"),QStringLiteral("restart")}};
}
QVariantMap LibraryPreferencesModel::supportNotes()const
{
    return {{QStringLiteral("singleInstance"),tr("Use one VLC instance after restart. Your existing file-manager-only legacy policy is preserved until an explicit opening-policy edit.")},
        {QStringLiteral("fileManagerSingleInstance"),tr("When one-instance mode is enabled, also send files opened from the file manager to that instance. Turning this off allows a separate file-manager launch. The saved choice can remain checked while its parent is off.")},
        {QStringLiteral("enqueueInSingleInstance"),tr("On appends files and keeps the current item. Off replaces the current queue and starts the newly opened files. A legacy Play-without-replacing policy is shown as custom until you choose On or Off. Existing named playlists are not changed.")},
        {QStringLiteral("folderRecursion"),tr("Controls automatic expansion of folders in the playback queue after restart. None keeps an unopened folder as one queue item; Collapse expands the opened folder and keeps its subfolders collapsed until played; Expand automatically expands all subfolders. Media-library indexing follows its existing explicit roots instead of this queue setting.")},
        {QStringLiteral("showHiddenFiles"),tr("Show hidden entries when VLC browses or expands a folder. This does not change Windows Explorer settings or remove media already indexed in the library.")},
        {QStringLiteral("metadataSource"),tr("File/local mode uses embedded metadata and existing local artwork or cache. Internet mode also permits automatic native metadata/artwork lookup after restart. This shares the Privacy internet-metadata preference; it does not submit listening history.")},
        {QStringLiteral("useMediaLibrary"),tr("Start VLC's existing media library after restart. Turning it off keeps the database, chosen folders, playlists, favorites and playback history intact. It does not clear history or add default folders immediately.")}};
}
QByteArray LibraryPreferencesModel::fingerprint()const
{
    if(!m_intf)return {};QVariantMap saved;
    for(const auto *key:{"one-instance","one-instance-when-started-from-file","media-library","metadata-network-access"})saved[QString::fromLatin1(key)]=boolean(key);
    saved[QStringLiteral("fileManagerPolicy")]=nativeFileManagerPolicy();saved[QStringLiteral("recursive")]=string("recursive");
    return QCryptographicHash::hash(QJsonDocument::fromVariant(saved).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
void LibraryPreferencesModel::refresh()
{
    const bool service=m_intf!=nullptr;const int open=nativeOpenMode();
    QVariantMap values{{QStringLiteral("singleInstance"),service?QVariant(boolean("one-instance")):QVariant{}},
        {QStringLiteral("fileManagerSingleInstance"),service?QVariant(fileManagerPreference()):QVariant{}},
        {QStringLiteral("enqueueInSingleInstance"),!service?QVariant{}:open==0?QVariant(false):open==1||boolean("playlist-enqueue")?QVariant(true):QVariant{}},
        {QStringLiteral("folderRecursion"),service&&has("recursive",VLC_VAR_STRING)?QVariant(string("recursive")):QVariant{}},
        {QStringLiteral("showHiddenFiles"),service&&has("show-hiddenfiles",VLC_VAR_BOOL)?QVariant(boolean("show-hiddenfiles")):QVariant{}},
        {QStringLiteral("metadataSource"),service&&has("metadata-network-access",VLC_VAR_BOOL)?QVariant(boolean("metadata-network-access")?QStringLiteral("network"):QStringLiteral("local")):QVariant{}},
        {QStringLiteral("useMediaLibrary"),service&&has("media-library",VLC_VAR_BOOL)?QVariant(boolean("media-library")):QVariant{}}};
    bool instance=false,queue=false,hidden=service&&has("show-hiddenfiles",VLC_VAR_BOOL);
#ifdef _WIN32
    instance=service&&m_consumers.instancePolicy&&has("one-instance",VLC_VAR_BOOL)&&has("one-instance-when-started-from-file",VLC_VAR_BOOL)&&has("one-instance-file-manager-policy",VLC_VAR_INTEGER);
    queue=service&&m_consumers.openingQueue&&has("playlist-enqueue",VLC_VAR_BOOL)&&has("playlist-open-mode",VLC_VAR_INTEGER);
    hidden=hidden&&m_consumers.hiddenAttributes;
#endif
    QVariantMap choices{
        {QStringLiteral("folderRecursion"), service ? recursionChoices() : QVariantList{}},
        {QStringLiteral("metadataSource"), QVariantList{
            QVariantMap{{QStringLiteral("value"), QStringLiteral("local")},
                        {QStringLiteral("name"), tr("From file and local cache")}},
            QVariantMap{{QStringLiteral("value"), QStringLiteral("network")},
                        {QStringLiteral("name"), tr("Also allow internet lookups")}}
        }}
    };
    QVariantMap caps{{QStringLiteral("singleInstance"),instance},{QStringLiteral("fileManagerSingleInstance"),instance&&boolean("one-instance")},
        {QStringLiteral("enqueueInSingleInstance"),queue&&boolean("one-instance")},{QStringLiteral("folderRecursion"),service&&has("recursive",VLC_VAR_STRING)&&!choices.value(QStringLiteral("folderRecursion")).toList().isEmpty()},
        {QStringLiteral("showHiddenFiles"),hidden},
        {QStringLiteral("metadataSource"),service&&m_consumers.metadataFetch&&has("metadata-network-access",VLC_VAR_BOOL)},
        {QStringLiteral("useMediaLibrary"),service&&has("media-library",VLC_VAR_BOOL)&&module_exists("medialibrary")}};
    const bool available=libraryAvailable(),legacy=legacyFileManagerOnly();const int file=nativeFileManagerPolicy();
    const bool changed=values!=m_values||available!=m_lastLibraryAvailable||legacy!=m_lastLegacyFileOnly||file!=m_lastFilePolicy||open!=m_lastOpenMode;
    m_values=values;m_lastLibraryAvailable=available;m_lastLegacyFileOnly=legacy;m_lastFilePolicy=file;m_lastOpenMode=open;
    if(caps!=m_capabilities){m_capabilities=caps;emit capabilitiesChanged();}
    if(choices!=m_choices){m_choices=choices;emit choicesChanged();}
    if(changed)emit valuesChanged();const bool restart=service&&fingerprint()!=m_startup;if(restart!=m_restart){m_restart=restart;emit restartRequiredChanged();}
}
void LibraryPreferencesModel::reload(){setError({});refresh();}
bool LibraryPreferencesModel::setValue(const QString &key,const QVariant &value)
{
    refresh();if(!m_intf||!m_capabilities.value(key).toBool())return fail(tr("This library setting is unavailable until its real native consumer has been verified."));
    QVariantMap changes;
    if(key==QStringLiteral("folderRecursion")||key==QStringLiteral("metadataSource")) {
        if(!text(value)||!listed(m_choices.value(key).toList(),value))return fail(tr("Choose one of the supported native policies."));
        if(key==QStringLiteral("folderRecursion"))changes[QStringLiteral("recursive")]=value.toString();
        else changes[QStringLiteral("metadata-network-access")]=value.toString()==QStringLiteral("network");
    } else {
        if(value.typeId()!=QMetaType::Bool)return fail(tr("Use an on/off value."));const bool enabled=value.toBool();
        if(key==QStringLiteral("singleInstance")) {
            changes[QStringLiteral("one-instance")]=enabled;
            // An explicit master edit adopts the authored hierarchical policy,
            // while preserving the original legacy flag as fallback data.
            changes[QStringLiteral("one-instance-file-manager-policy")]=fileManagerPreference()?1:0;
        } else if(key==QStringLiteral("fileManagerSingleInstance"))changes[QStringLiteral("one-instance-file-manager-policy")]=enabled?1:0;
        else if(key==QStringLiteral("enqueueInSingleInstance")){changes[QStringLiteral("playlist-open-mode")]=enabled?1:0;changes[QStringLiteral("playlist-enqueue")]=enabled;}
        else if(key==QStringLiteral("showHiddenFiles"))changes[QStringLiteral("show-hiddenfiles")]=enabled;
        else if(key==QStringLiteral("useMediaLibrary"))changes[QStringLiteral("media-library")]=enabled;
        else return fail(tr("Unknown library setting."));
    }
    QString error;if(!CheckedPreferencesStore(m_intf).saveNative(changes,error))return fail(error);setError({});refresh();return true;
}
