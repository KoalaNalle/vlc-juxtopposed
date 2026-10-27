// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef VLC_TEST_PLAYBACK_PREFERENCES_HPP
#define VLC_TEST_PLAYBACK_PREFERENCES_HPP
#include <QObject>
#include <QString>
#include <QTemporaryDir>
#include <memory>
#include "maininterface/playback_preferences_model.hpp"
struct libvlc_instance_t;
struct qt_intf_t;
class TestPlaybackPreferences final : public QObject
{
    Q_OBJECT
    QTemporaryDir m_temp;
    QString m_root, m_case, m_config;
    libvlc_instance_t *m_instance = nullptr;
    qt_intf_t *m_intf = nullptr;
    std::unique_ptr<PlaybackPreferencesModel> m_model;
    bool openNative();
    void closeNative();
private slots:
    void initTestCase();
    void init();
    void cleanup();
    void exactApiReadOnlyAndCapabilities();
    void strictInvalidWritesPreserveState();
    void nativeEnumChoices_data();
    void nativeEnumChoices();
    void liveOrderUsesCorrectIndependentNativeBits();
    void liveStartPausedAndFinishPrecedence();
    void quarterSpeedPersistenceAndRestart_data();
    void quarterSpeedPersistenceAndRestart();
    void corkStartupAndExistingUnknownDefaultsPreserved();
    void trackIndexesClearOnlyTheirExplicitIdOverride();
    void nativeMenuCodesAndLegacyListPreservation();
    void nativeInputDefaultsAndMixedOpticalPaths();
    void timeShiftNativeBytesAndDirectory();
    void preciseSeekingRequiresActualConsumerGate();
    void thumbnailPolicyPersistenceAndGate();
    void saveFailurePreservesDiskConfigAndLiveState_data();
    void saveFailurePreservesDiskConfigAndLiveState();
    void missingServiceAndActualPickerBoundary();
};
#endif
