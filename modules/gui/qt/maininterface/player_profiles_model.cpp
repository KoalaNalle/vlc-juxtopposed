// SPDX-License-Identifier: GPL-2.0-or-later
#include "player_profiles_model.hpp"
#include "player_profile_layout.hpp"
#include <QScopedValueRollback>

namespace {
QVariantMap defaultsPart(QVariantMap snapshot) { snapshot.remove(QStringLiteral("layout")); return snapshot; }
}
PlayerProfilesModel::PlayerProfilesModel(qt_intf_t *intf, QSettings *settings, QObject *parent)
    : QObject(parent), m_store(settings), m_preferences(intf), m_layout(new PlayerProfileLayout(this))
{
    m_checkpoint.setSingleShot(true);
    connect(&m_checkpoint,&QTimer::timeout,this,[this] {checkpoint();});
    QVariantMap current; QString error;
    if (!capture(current,error) || !m_store.reload(current,error)) {m_error = error; return;}
    const int selected = m_store.indexOf(m_store.selectedId());
    auto layout = m_store.profiles()[selected].snapshot.value(QStringLiteral("layout")).toMap();
    if (const auto &pending = m_store.pendingSwitch()) {
        // An interrupted switch may already have saved all target native
        // defaults. Match that layout without silently completing the journal.
        const auto actual = defaultsPart(current);
        if (actual == defaultsPart(pending->after)) layout = pending->after.value(QStringLiteral("layout")).toMap();
        else if (actual == defaultsPart(pending->before)) layout = pending->before.value(QStringLiteral("layout")).toMap();
        m_error = tr("A profile switch was interrupted. Finish the switch or restore the previous profile before editing.");
    }
    if (!m_layout->apply(layout,error)) {m_error = error; return;}
    m_valid = true;
}
QVariantList PlayerProfilesModel::profiles() const
{
    QVariantList result;
    for (const auto &profile : m_store.profiles()) result.append(QVariantMap{
        {QStringLiteral("id"),profile.id},{QStringLiteral("name"),profile.name}});
    return result;
}
QString PlayerProfilesModel::selectedName() const
{
    const int selected = m_store.indexOf(m_store.selectedId());
    return selected < 0 ? QString{} : m_store.profiles()[selected].name;
}
bool PlayerProfilesModel::capture(QVariantMap &result, QString &error) const
{
    if (!m_preferences.snapshot(result,error)) return false;
    result.insert(QStringLiteral("layout"),m_layout->snapshot()); return true;
}
bool PlayerProfilesModel::fail(const QString &error) {m_error = error; emit stateChanged(); return false;}
bool PlayerProfilesModel::editable(bool allowDraft)
{
    if (!available()) return fail(tr("The saved player profiles could not be read. Their data has been preserved."));
    if (busy()) return fail(tr("Finish the current player settings operation first."));
    if (needsRecovery()) return fail(tr("Finish or restore the interrupted profile switch first."));
    if (!allowDraft && m_layout->editing()) return fail(tr("Save or cancel Customize Player first."));
    return true;
}
void PlayerProfilesModel::setExternalBusy(bool busy)
{ if (m_externalBusy != busy) {m_externalBusy = busy; emit stateChanged();} }
void PlayerProfilesModel::preferencesChanged()
{
    if (m_busy || !available() || needsRecovery()) return;
    m_unsaved = true; emit stateChanged(); m_checkpoint.start(0);
}
bool PlayerProfilesModel::checkpoint()
{
    if (m_busy || !available() || needsRecovery()) return false;
    QVariantMap current; QString error;
    if (!capture(current,error) || !m_store.checkpoint(current,error)) {m_unsaved = true; return fail(error);}
    m_unsaved = false; m_error.clear(); emit stateChanged(); return true;
}
bool PlayerProfilesModel::retrySave()
{
    if (!editable(true)) return false;
    m_checkpoint.stop(); return checkpoint();
}
bool PlayerProfilesModel::createProfile(const QString &name)
{
    if (!editable()) return false;
    m_checkpoint.stop(); QVariantMap current; QString id,error;
    if (!capture(current,error) || !m_store.create(name,current,id,error)) return fail(error);
    m_unsaved = false; m_error.clear(); emit stateChanged(); return true;
}
bool PlayerProfilesModel::selectProfile(const QString &id)
{
    if (!editable()) return false;
    m_checkpoint.stop();
    if (id == selectedId()) return checkpoint();
    const int target = m_store.indexOf(id);
    if (target < 0) return fail(tr("This player profile is no longer available."));
    const auto snapshot = m_store.profiles()[target].snapshot; QString error; QVariantMap changes,current;
    if (!PlayerProfileLayout::validate(snapshot.value(QStringLiteral("layout")).toMap(),error)
        || !m_preferences.prepare(defaultsPart(snapshot),changes,error) || !capture(current,error)) return fail(error);
    if (!m_store.beginSwitch(id,current,error)) return fail(error);
    emit stateChanged(); return applyPending(true);
}
bool PlayerProfilesModel::applyPending(bool forward)
{
    if (!available()) return fail(tr("The saved player profiles could not be read. Their data has been preserved."));
    if (busy()) return fail(tr("Finish the current player settings operation first."));
    if (!needsRecovery()) return fail(tr("There is no interrupted profile switch to recover."));
    const auto target = forward ? m_store.pendingSwitch()->after : m_store.pendingSwitch()->before;
    QString error; QVariantMap changes;
    if (!PlayerProfileLayout::validate(target.value(QStringLiteral("layout")).toMap(),error)
        || !m_preferences.prepare(defaultsPart(target),changes,error)) return fail(error);
    const bool finished = [&] {
        QScopedValueRollback<bool> operation(m_busy,true);
        if (!m_preferences.apply(defaultsPart(target),error)) return false;
        // Shape validation preceded the native write; this in-memory load has
        // no I/O or asynchronous input/reopen operation.
        if (!m_layout->apply(target.value(QStringLiteral("layout")).toMap(),error)) return false;
        emit defaultsApplied(changes.keys());
        QVariantMap actual;
        if (!capture(actual,error)) return false;
        return forward ? m_store.finishSwitch(actual,error) : m_store.cancelSwitch(actual,error);
    }();
    if (!finished) return fail(error + QLatin1Char(' ') + tr("The recovery record is retained; retry or restore the previous profile."));
    m_unsaved = false; m_error.clear(); emit stateChanged(); return true;
}
bool PlayerProfilesModel::retrySwitch() {return applyPending(true);}
bool PlayerProfilesModel::restorePreviousProfile() {return applyPending(false);}
bool PlayerProfilesModel::beginLayoutEdit()
{
    if (!editable()) return false;
    m_layout->beginEdit(); m_error.clear(); emit stateChanged(); return true;
}
bool PlayerProfilesModel::saveLayoutDraft()
{
    if (!editable(true)) return false;
    if (!m_layout->editing()) return fail(tr("Open Customize Player first."));
    m_checkpoint.stop(); QString error; QVariantMap current;
    const auto draft = m_layout->draftSnapshot();
    if (!PlayerProfileLayout::validate(draft,error) || !capture(current,error)) return fail(error);
    current[QStringLiteral("layout")] = draft;
    // Publish the active native models only after the profile save succeeds.
    // A failed save leaves the current player and the editable draft intact.
    if (!m_store.checkpoint(current,error)) return fail(error);
    if (!m_layout->apply(draft,error)) return fail(error);
    m_layout->cancelEdit(); m_unsaved = false; m_error.clear(); emit stateChanged(); return true;
}
