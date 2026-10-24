// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "privacy_preferences_model.hpp"
#include "qt.hpp"
#include "util/checked_preferences_store.hpp"
#include "util/recent_network_history.hpp"
#include <vlc_configuration.h>
#include <vlc_media_library.h>
#include <vlc_player.h>
#include <QCryptographicHash>
#include <QByteArrayList>
#include <QJsonDocument>
#include <QSettings>
#include <QThread>
#include <utility>

namespace {
bool has(const char *key, int type) { return config_GetType(key) == type; }
bool boolean(const char *key) { return has(key, VLC_VAR_BOOL) && config_GetInt(key) != 0; }
QString text(const char *key)
{
    if (!has(key, VLC_VAR_STRING)) return {};
    char *raw=config_GetPsz(key);
    const auto result=QString::fromUtf8(raw ? raw : "");
    free(raw); return result;
}
struct SubmissionInterfaces { bool enabled=false; QString withoutSubmission; };
SubmissionInterfaces submissionInterfaces()
{
    SubmissionInterfaces result;
    QByteArray remaining=text("extraintf").toUtf8();
    QByteArrayList retained;
    while (!remaining.isEmpty()) {
        char *name=nullptr;config_chain_t *options=nullptr;
        char *next=config_ChainCreate(&name,&options,remaining.constData());
        const bool submission=name&&!strcmp(name,"audioscrobbler");
        const auto consumed=remaining.size()-(next?qsizetype(strlen(next))+1:0);
        result.enabled|=submission;
        if (!submission) retained.append(remaining.left(consumed));
        free(name);config_ChainDestroy(options);
        remaining=next?QByteArray(next):QByteArray{};free(next);
    }
    result.withoutSubmission=QString::fromUtf8(retained.join(':'));
    return result;
}
bool submissionEnabled() { return submissionInterfaces().enabled; }
bool validText(const QVariant &value, qsizetype limit)
{
    if (value.typeId()!=QMetaType::QString) return false;
    const auto s=value.toString(); const auto utf8=s.toUtf8();
    if (utf8.size()>limit || QString::fromUtf8(utf8)!=s) return false;
    for (auto c:s) if (c.unicode()<32 || c.unicode()==127) return false;
    return true;
}
class ClearThread final : public QThread
{
public:
    explicit ClearThread(vlc_player_t *p) : player(p) {}
    int result=VLC_EGENERIC;
private:
    void run() override
    {
        vlc_player_Lock(player);
        result=vlc_player_ClearHistory(player);
        vlc_player_Unlock(player);
    }
    vlc_player_t *player;
};
}
PrivacyPreferencesModel::PrivacyPreferencesModel(qt_intf_t *intf, QSettings *settings,
        LocalHistory local, QObject *parent)
    : QObject(parent),m_intf(intf),m_settings(settings),m_local(std::move(local))
{
    if (m_intf) var_Create(m_intf,"save-recentplay",VLC_VAR_BOOL|VLC_VAR_DOINHERIT);
    m_startup=startupFingerprint(); m_historyApplied=historyFingerprint(); refresh();
}
PrivacyPreferencesModel::~PrivacyPreferencesModel()
{
    m_destroying=true;
    // MainCtx destroys this service while the borrowed player/ML still live.
    if (m_worker) {
        m_worker->disconnect(this); m_worker->wait();
        const int result=static_cast<ClearThread *>(m_worker)->result;
        delete m_worker; m_worker=nullptr;
        if (result==VLC_SUCCESS) finishLocalClear();
    }
    if (m_intf) var_Destroy(m_intf,"save-recentplay");
}
void PrivacyPreferencesModel::setError(const QString &s)
{ if (s!=m_error) { m_error=s; emit lastErrorChanged(); } }
void PrivacyPreferencesModel::setStatus(const QString &s)
{ if (s!=m_status) { m_status=s; emit statusChanged(); } }
bool PrivacyPreferencesModel::fail(const QString &s) { setError(s); return false; }
QByteArray PrivacyPreferencesModel::startupFingerprint() const
{
    if (!m_intf) return {};
    // Never expose credentials in valuesChanged diagnostics, status or logs.
    const QVariantMap snapshot{{QStringLiteral("metadata"),boolean("metadata-network-access")},
        {QStringLiteral("interfaces"),text("extraintf")},{QStringLiteral("user"),text("lastfm-username")},
        {QStringLiteral("password"),text("lastfm-password")},{QStringLiteral("server"),text("scrobbler-url")}};
    return QCryptographicHash::hash(QJsonDocument::fromVariant(snapshot).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
}
QByteArray PrivacyPreferencesModel::historyFingerprint() const
{
    if (!m_intf) return {};
    QByteArray data;data.append(boolean("save-recentplay")?'1':'0');data.append(boolean("remember-playback-pos")?'1':'0');
    char *pattern=config_GetPsz("history-exclude-pattern");data.append(pattern?pattern:"");free(pattern);
    return QCryptographicHash::hash(data,QCryptographicHash::Sha256);
}
bool PrivacyPreferencesModel::applySavedHistoryPolicy()
{
    if (!m_intf || !m_intf->p_player) return true;
    const auto saved=historyFingerprint();
    if (saved==m_historyApplied) return true;
    vlc_player_history_policy next{boolean("save-recentplay"),boolean("remember-playback-pos"),config_GetPsz("history-exclude-pattern")};
    if (!next.exclusion_pattern) next.exclusion_pattern=strdup("");
    if (!next.exclusion_pattern) return fail(tr("Not enough memory to apply the saved history policy."));
    vlc_player_Lock(m_intf->p_player);vlc_player_SwapHistoryPolicy(m_intf->p_player,&next);vlc_player_Unlock(m_intf->p_player);
    free(next.exclusion_pattern);m_historyApplied=saved;
    var_SetBool(m_intf,"save-recentplay",boolean("save-recentplay"));
    if (m_local.invalidatePendingVisits) m_local.invalidatePendingVisits();
    emit historyPolicyChanged();return true;
}
QVariantMap PrivacyPreferencesModel::applyModes() const
{
    return {{QStringLiteral("saveRecentlyPlayed"),QStringLiteral("newInputs")},
        {QStringLiteral("rememberPlaybackPosition"),QStringLiteral("newInputs")},
        {QStringLiteral("historyExclusion"),QStringLiteral("newInputs")},
        {QStringLiteral("clearHistory"),QStringLiteral("immediate")},
        {QStringLiteral("internetMetadata"),QStringLiteral("restart")},
        {QStringLiteral("submitPlayedTracks"),QStringLiteral("restart")},
        {QStringLiteral("lastfmUsername"),QStringLiteral("restart")},
        {QStringLiteral("lastfmPassword"),QStringLiteral("restart")},
        {QStringLiteral("lastfmSubmissionUrl"),QStringLiteral("restart")}};
}
QVariantMap PrivacyPreferencesModel::supportNotes() const
{
    return {{QStringLiteral("saveRecentlyPlayed"),tr("Save automatic playback history and Home folder/playlist visits. Turning this off stops pending writes immediately. Turning it on applies to newly opened items. Existing history, favorites and named playlists are preserved; use Clear to remove history.")},
        {QStringLiteral("rememberPlaybackPosition"),tr("Allow saving and restoring resume positions. Playback's Never/Ask/Always restore preference remains separate. Turning this off stops pending position writes immediately; new activity can still appear in history with no resume position. Turning it on applies to newly opened items.")},
        {QStringLiteral("historyExclusion"),tr("Case-sensitive wildcard substring matched against the URI, decoded local path and item name, including Home folders and playlist names. * matches any sequence; ? matches one Unicode character. Empty excludes nothing. No regular expressions or escape syntax. Existing history is preserved. Changed rules cancel pending writes and apply to newly opened items.")},
        {QStringLiteral("clearHistory"),tr("Clear local and stream playback history, resume positions, Home folder/playlist visits, recent Open Network addresses and the saved file-dialog location. Keep playback, the current queue, library roots, favorites and named playlists. Current and retiring items cannot write cleared activity back. Clearing history does not enable the Library or scan folders. If some history could not be removed, retry to finish clearing it.")},
        {QStringLiteral("internetMetadata"),tr("Permit automatic native title and artwork lookups after restart. Shares Library's metadata preference. Local mode retains embedded metadata, local art and existing cache. This does not submit listening history.")},
        {QStringLiteral("submitPlayedTracks"),tr("The bundled legacy Last.fm service has not passed transport/authentication acceptance. New submission activation is unavailable. An already enabled legacy interface can be turned off; restart VLC to stop it. Other extra interfaces are preserved.")},
        {QStringLiteral("lastfmUsername"),tr("Legacy Last.fm account name. Editing remains unavailable until secure service support is verified.")},
        {QStringLiteral("lastfmPassword"),tr("Passwords are masked. Credential editing is unavailable in this build.")},
        {QStringLiteral("lastfmSubmissionUrl"),tr("The legacy backend expects a host and constructs HTTP internally. This field remains unavailable until URL and secure transport support are verified.")}};
}
void PrivacyPreferencesModel::refresh()
{
    const bool service=m_intf && m_intf->p_player;
    QVariantMap values;
    if (m_intf) values={{QStringLiteral("saveRecentlyPlayed"),boolean("save-recentplay")},
        {QStringLiteral("rememberPlaybackPosition"),boolean("remember-playback-pos")},
        {QStringLiteral("historyExclusion"),text("history-exclude-pattern")},
        {QStringLiteral("internetMetadata"),boolean("metadata-network-access")},
        {QStringLiteral("submitPlayedTracks"),submissionEnabled()},
        {QStringLiteral("lastfmUsername"),text("lastfm-username")},
        {QStringLiteral("lastfmPassword"),text("lastfm-password")},
        {QStringLiteral("lastfmSubmissionUrl"),text("scrobbler-url")}};
    QVariantMap caps{{QStringLiteral("saveRecentlyPlayed"),service&&has("save-recentplay",VLC_VAR_BOOL)},
        {QStringLiteral("rememberPlaybackPosition"),service&&has("remember-playback-pos",VLC_VAR_BOOL)},
        {QStringLiteral("historyExclusion"),service&&has("history-exclude-pattern",VLC_VAR_STRING)},
        {QStringLiteral("clearHistory"),service&&m_settings&&vlc_ml_history_available(m_intf)},
        {QStringLiteral("internetMetadata"),m_intf&&has("metadata-network-access",VLC_VAR_BOOL)},
        {QStringLiteral("submitPlayedTracks"),m_intf&&submissionEnabled()},
        {QStringLiteral("lastfmUsername"),false},{QStringLiteral("lastfmPassword"),false},
        {QStringLiteral("lastfmSubmissionUrl"),false}};
    const bool restart=m_intf&&startupFingerprint()!=m_startup;
    if (values!=m_values || caps!=m_capabilities || restart!=m_restart)
    { m_values=values; m_capabilities=caps; m_restart=restart; emit valuesChanged(); }
}
void PrivacyPreferencesModel::reload()
{
    if (busy() || m_destroying) return;
    setError({});setStatus({});
    applySavedHistoryPolicy();refresh();
}
bool PrivacyPreferencesModel::setValue(const QString &key,const QVariant &value)
{
    if (m_destroying) return false;
    if (busy()) return fail(tr("Wait for history clearing to finish."));
    refresh();
    if (!m_intf || !m_capabilities.value(key).toBool()) return fail(tr("This Privacy setting is unavailable. See its information button for details."));
    QVariantMap changes;
    bool policy=false;
    if (key==QStringLiteral("historyExclusion")) {
        if (!validText(value,4096)) return fail(tr("Use valid text of at most 4096 UTF-8 bytes without control characters."));
        changes[QStringLiteral("history-exclude-pattern")]=value; policy=true;
    } else {
        if (value.typeId()!=QMetaType::Bool) return fail(tr("Use an on/off value."));
        const bool on=value.toBool();
        if (key==QStringLiteral("saveRecentlyPlayed")) { changes[QStringLiteral("save-recentplay")]=on; policy=true; }
        else if (key==QStringLiteral("rememberPlaybackPosition")) { changes[QStringLiteral("remember-playback-pos")]=on; policy=true; }
        else if (key==QStringLiteral("internetMetadata")) changes[QStringLiteral("metadata-network-access")]=on;
        else if (key==QStringLiteral("submitPlayedTracks") && !on) {
            // Native chain parsing preserves colons/quotes inside other
            // interfaces' option blocks, plus their original ordering/bytes.
            changes[QStringLiteral("extraintf")]=submissionInterfaces().withoutSubmission;
        } else return fail(tr("New Last.fm activation is unavailable until secure service support is verified."));
    }
    vlc_player_history_policy next{};
    if (policy) {
        next.save_history=changes.value(QStringLiteral("save-recentplay"),boolean("save-recentplay")).toBool();
        next.remember_position=changes.value(QStringLiteral("remember-playback-pos"),boolean("remember-playback-pos")).toBool();
        const auto pattern=changes.value(QStringLiteral("history-exclude-pattern"),text("history-exclude-pattern")).toString().toUtf8();
        next.exclusion_pattern=strdup(pattern.constData());
        if (!next.exclusion_pattern) return fail(tr("Not enough memory to change history policy."));
    }
    QString error;
    if (!CheckedPreferencesStore(m_intf).saveNative(changes,error)) { free(next.exclusion_pattern); return fail(error); }
    if (policy) {
        vlc_player_Lock(m_intf->p_player);
        vlc_player_SwapHistoryPolicy(m_intf->p_player,&next);
        vlc_player_Unlock(m_intf->p_player);
        free(next.exclusion_pattern);
        m_historyApplied=historyFingerprint();
        // Existing Qt consumers inherit this object variable, not config directly.
        var_SetBool(m_intf,"save-recentplay",boolean("save-recentplay"));
        if (m_local.invalidatePendingVisits) m_local.invalidatePendingVisits();
        emit historyPolicyChanged();
    }
    setError({}); setStatus({}); refresh();
    if (key==QStringLiteral("internetMetadata")) emit metadataPreferenceChanged();
    return true;
}
bool PrivacyPreferencesModel::clearHistory()
{
    if (busy() || m_destroying) return false;
    refresh();
    if (!m_capabilities.value(QStringLiteral("clearHistory")).toBool()) return fail(tr("History clearing is unavailable."));
    setError({}); setStatus({});
    auto *worker=new ClearThread(m_intf->p_player); m_worker=worker;
    connect(worker,&QThread::finished,this,[this,worker] {
        if (m_destroying || m_worker!=worker) return;
        worker->wait();
        const int result=worker->result;
        if (result!=VLC_SUCCESS) fail(tr("Could not clear playback history. Home visits and the saved file location were kept. Retry after resolving the database error."));
        else finishLocalClear();
        m_worker=nullptr; delete worker; emit busyChanged();
    });
    emit busyChanged(); worker->start(); return true;
}
void PrivacyPreferencesModel::finishLocalClear()
{
    QStringList errors;
    if (m_local.clearVisits) { const auto error=m_local.clearVisits(); if (!error.isEmpty()) errors.append(error); }
    QString error;
    if (!RecentNetworkHistory::clear(m_intf,m_settings,error)) errors.append(error);
    else emit networkHistoryCleared();
    if (!CheckedPreferencesStore(m_intf).saveSettings(m_settings,QStringLiteral("filedialog-path"),{},true,error)) errors.append(error);
    else emit fileDialogHistoryCleared();
    if (!errors.isEmpty()) { fail(tr("Playback history was cleared, but some local history could not be cleared. Retry. %1").arg(errors.join(' '))); return; }
    setError({}); setStatus(tr("History cleared."));
}
