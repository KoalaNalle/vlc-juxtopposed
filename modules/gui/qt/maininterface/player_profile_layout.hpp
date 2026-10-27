// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include "dialogs/toolbar/controlbar_profile.hpp"

class PlayerControlbarModel;
// A draft owns separate native column models. Opening, dragging and cancelling
// never mutate the active player; the profile coordinator owns the checked save.
class PlayerProfileLayout final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool editing READ editing NOTIFY editingChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QVariantList palette READ palette CONSTANT FINAL)
    Q_PROPERTY(QVariantMap current READ snapshot NOTIFY changed FINAL)
    Q_PROPERTY(QVariantMap draft READ draftSnapshot NOTIFY draftChanged FINAL)
    Q_PROPERTY(bool defaultVideo READ defaultVideo NOTIFY changed FINAL)
    Q_PROPERTY(PlayerControlbarModel* video READ video CONSTANT FINAL)
    Q_PROPERTY(PlayerControlbarModel* audio READ audio CONSTANT FINAL)
    Q_PROPERTY(PlayerControlbarModel* draftVideo READ draftVideo CONSTANT FINAL)
    Q_PROPERTY(PlayerControlbarModel* draftAudio READ draftAudio CONSTANT FINAL)
    Q_PROPERTY(bool videoTimelineAbove READ videoTimelineAbove NOTIFY changed FINAL)
    Q_PROPERTY(bool audioTimelineAbove READ audioTimelineAbove NOTIFY changed FINAL)
    Q_PROPERTY(bool draftVideoTimelineAbove READ draftVideoTimelineAbove NOTIFY draftChanged FINAL)
    Q_PROPERTY(bool draftAudioTimelineAbove READ draftAudioTimelineAbove NOTIFY draftChanged FINAL)
    Q_MOC_INCLUDE("player/player_controlbar_model.hpp")
public:
    explicit PlayerProfileLayout(QObject *parent = nullptr);
    static QVariantMap defaults();
    static bool validate(const QVariantMap &, QString &error);
    static bool supportedTile(int id);
    static QVariantList palette();
    Q_INVOKABLE QVariantMap tileInfo(int id) const;
    bool defaultVideo() const;
    QVariantMap snapshot() const;
    bool apply(const QVariantMap &, QString &error);
    bool editing() const { return m_editing; }
    QString lastError() const { return m_error; }
    PlayerControlbarModel *video() const;
    PlayerControlbarModel *audio() const;
    PlayerControlbarModel *draftVideo() const;
    PlayerControlbarModel *draftAudio() const;
    bool videoTimelineAbove() const { return m_videoAbove; }
    bool audioTimelineAbove() const { return m_audioAbove; }
    bool draftVideoTimelineAbove() const { return m_draftVideoAbove; }
    bool draftAudioTimelineAbove() const { return m_draftAudioAbove; }
    Q_INVOKABLE void beginEdit();
    Q_INVOKABLE void cancelEdit();
    Q_INVOKABLE QVariantMap draftSnapshot() const;
    Q_INVOKABLE bool insertTile(int player, int column, int index, int id);
    Q_INVOKABLE bool moveTile(int player, int fromColumn, int fromIndex, int toColumn, int toIndex);
    Q_INVOKABLE bool removeTile(int player, int column, int index);
    Q_INVOKABLE bool setTimelineAbove(int player, bool above);
    Q_INVOKABLE void resetDraft();
signals:
    void changed();
    void draftChanged();
    void editingChanged();
    void lastErrorChanged();
private:
    static QVariantMap capture(const ControlbarProfile &, bool videoAbove, bool audioAbove);
    static void load(ControlbarProfile &, const QVariantMap &);
    bool fail(const QString &);
    void setError(const QString &);
    ControlbarProfile m_active, m_draft;
    bool m_editing = false;
    bool m_videoAbove = false, m_audioAbove = false;
    bool m_draftVideoAbove = false, m_draftAudioAbove = false;
    QString m_error;
};
