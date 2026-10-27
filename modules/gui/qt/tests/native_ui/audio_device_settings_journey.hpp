// SPDX-License-Identifier: GPL-2.0-or-later
// Actual Audio editors, owned config and real output; private GUI only.
void exerciseAudioDeviceSettings(const QString &mode)
{
    auto *audio = context->getAudioPreferences();
    check(audio != nullptr, "actual MainCtx owns the Audio service");
    const bool unmanaged = mode.endsWith(QStringLiteral("-unmanaged"));
    const bool stale = mode.endsWith(QStringLiteral("-stale"));
    const bool reopen = mode.endsWith(QStringLiteral("-reopen"));
    const bool playing = mode.endsWith(QStringLiteral("-playing"));
    auto *provider = DialogsProvider::getInstance();
    const auto open = [&] {
        check(QMetaObject::invokeMethod(provider,"showGlobalSettingsDialog"), "native route opens the actual settings host");
        wait([]{return named(QStringLiteral("globalCategoryaudio"));}, "actual Audio category button materializes");
        click(named(QStringLiteral("globalCategoryaudio")));
        wait([]{return named(QStringLiteral("juxtopposedAudioSettings"));}, "actual Audio page mounts its production service");
    };
    const auto reveal = [&](const QString &name) {
        auto *item = named(name);check(item,"actual Audio editor exists");
        item->forceActiveFocus(Qt::TabFocusReason);QTest::qWait(120);return item;
    };
    const auto type = [&](const QString &text) {
        click(reveal(QStringLiteral("audiostartupVolume")));
        QTest::keyClick(window,Qt::Key_A,Qt::ControlModifier);
        for (const auto digit : text) QTest::keyClick(window,static_cast<Qt::Key>(Qt::Key_0+digit.digitValue()));
        QTest::keyClick(window,Qt::Key_Return);QTest::qWait(100);
    };
    const auto select = [&](int index) {
        auto *combo = reveal(QStringLiteral("audiooutputDevice"));click(combo);
        wait([&]{return expression(combo,QStringLiteral("popup.visible")).toBool();}, "actual device popup opens");
        QTest::keyClick(window,Qt::Key_Home);
        for(int i=0;i<index;++i) QTest::keyClick(window,Qt::Key_Down);
        QTest::keyClick(window,Qt::Key_Return);QTest::qWait(120);
        check(combo->property("currentIndex").toInt()==index,"actual keyboard popup selection commits its indexed endpoint");
    };
    const auto dismiss = [&] {
        QTest::keyClick(window,Qt::Key_Escape);
        wait([&]{return !provider->qmlDialogVisible();},"actual immediate-write Audio settings dismiss normally");
    };
    const auto bytes = [](const QString &path) {QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray{};};
    const auto config = qEnvironmentVariable("VLC_OWNED_SETTINGS_CONFIG");
    check(!config.isEmpty() && QFileInfo(config).isFile(),"owned native config exists");
    check(!audio->capabilities().value("passthrough").toBool(),"receiver passthrough remains independently unavailable");
    if (unmanaged) {
        const auto before=bytes(config);
        check(!audio->capabilities().value("outputDevice").toBool()
              && !audio->capabilities().value("startupVolume").toBool(),"unmanaged output refuses Windows-only capabilities");
        open();
        for(const auto *name:{"audiooutputDevice","audiostartupVolume","audioFixedVolume"})
            check(!named(QString::fromLatin1(name))->isEnabled(),"unmanaged output disables the actual paired editor or fixed policy");
        check(named(QStringLiteral("audiorememberVolume"))->isEnabled(),"independent Remember policy remains available");
        check(!audio->setValue(QStringLiteral("startupVolume"),40.) && bytes(config)==before,
              "unavailable native writer refuses a Windows volume change without saving");
        dismiss();report.insert("nativeAudioDeviceSettingsAccepted",true);return;
    }
    check(audio->capabilities().value("outputDevice").toBool()
          && audio->capabilities().value("startupVolume").toBool(),"Windows endpoint and fixed-volume capabilities are linked");
    const auto devices=audio->devices();
    check(devices.size()>1 && devices.first().toMap().value("value").toString().isEmpty(),
          "real enumeration contains System default and an active render endpoint");
    if(stale) {
        check(audio->values().value("outputDevice").toString()==QStringLiteral("owned-disconnected-endpoint"),
              "owned stale endpoint is preserved before an explicit replacement");
        open();
        check(named(QStringLiteral("audiooutputDevice"))->property("currentIndex").toInt()==-1,
              "actual device combo leaves a missing saved endpoint unselected");
        select(0);
        check(audio->values().value("outputDevice").toString().isEmpty() && audio->lastError().isEmpty()
              && audio->restartRequired(),"actual System default selection repairs the saved endpoint with restart feedback");
        dismiss();report.insert("nativeAudioDeviceSettingsAccepted",true);return;
    }
    const auto url=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("owned-settings.wav")));
    context->getIntf()->p_mainPlaylistController->append(
        QVector<vlc::playlist::Media>{vlc::playlist::Media{url.toString(QUrl::FullyEncoded),{}, {}}},true);
    context->setPlayerView(true);
    wait([&]{vlc_player_locker lock(context->getIntf()->p_player);
        const auto *stats=vlc_player_GetStatistics(context->getIntf()->p_player);
        return player()->getUrl()==url && stats && stats->i_decoded_audio>0 && stats->i_played_abuffers>0;},
         "actual main player decodes and plays owned silence through a real audio output",15000);
    const auto device = [&] {
        auto *output=vlc_player_aout_Hold(context->getIntf()->p_player);if(!output)return QString{};
        char *raw=aout_DeviceGet(output);const auto result=QString::fromUtf8(raw?raw:"");free(raw);aout_Release(output);return result;
    };
    const auto savedDevice=audio->values().value("outputDevice").toString();
    if(reopen || playing) {
        wait([&]{return device()==savedDevice && std::abs(player()->getVolume()-.65f)<.002f;},
             "cold real output consumes the saved explicit endpoint and sixty-five percent volume",10000);
        check(!audio->restartRequired() && !audio->values().value("rememberVolume").toBool(),
              "cold fixed-volume policy has no stale restart warning");
    }
    if(!playing) {vlc_player_locker lock(context->getIntf()->p_player);vlc_player_Pause(context->getIntf()->p_player);}
    wait([&]{return player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED);},
         "primary reaches the requested playing or paused state");
    if(!playing) {
        auto last=nativeTime();QElapsedTimer stable;stable.start();
        wait([&]{const auto now=nativeTime();if(now!=last){last=now;stable.restart();}return stable.elapsed()>1000;},
             "paused native clock settles before Audio defaults editing",5000);
    }
    auto *playerNative=context->getIntf()->p_player;
    std::unique_ptr<audio_output_t,decltype(&aout_Release)> initialOutput(vlc_player_aout_Hold(playerNative),aout_Release);
    check(initialOutput!=nullptr,"actual audio output owner is available");
    const auto input=player()->getInput();const auto clock=nativeTime();const auto volume=player()->getVolume();
    const auto rate=player()->getRate();const auto mute=player()->isMuted();const auto initialDevice=device();
    QElapsedTimer editingTime;editingTime.start();
    const auto unchanged=[&] {
        auto *output=vlc_player_aout_Hold(playerNative);const bool same=output==initialOutput.get();if(output)aout_Release(output);
        return same && device()==initialDevice && player()->getInput()==input && player()->getVolume()==volume
            && player()->getRate()==rate && player()->isMuted()==mute
            && player()->getPlayingState()==(playing?PlayerController::PLAYING_STATE_PLAYING:PlayerController::PLAYING_STATE_PAUSED)
            && (playing ? (nativeTime()>=clock && std::abs(nativeTime()-clock
                - vlc_tick_t(VLC_TICK_FROM_MS(editingTime.elapsed())*rate))<VLC_TICK_FROM_MS(1500)) : nativeTime()==clock);
    };
    QTest::qWait(150);check(unchanged(),"no-write audio output input volume and clock control passes");
    open();
    auto *combo=reveal(QStringLiteral("audiooutputDevice"));
    auto *field=reveal(QStringLiteral("audiostartupVolume"));
    check(combo->isEnabled() && field->isEnabled() && named(QStringLiteral("audioFixedVolume"))->isEnabled(),
          "actual endpoint fixed-policy and numeric editors are enabled");
    int selected=1;
    for(int i=1;i<devices.size();++i)
        if(devices[i].toMap().value("value").toString()==initialDevice)selected=i;
    const auto chosen=devices[selected].toMap().value("value").toString();
    if(reopen) {
        check(std::abs(field->property("text").toString().remove(QLatin1Char('%')).toDouble()-65.)<.0001,
              "cold actual editor displays the saved fixed volume");
        type(QStringLiteral("75"));check(audio->restartRequired() && unchanged(),"changed startup number preserves the paused output and requests restart");
        type(QStringLiteral("65"));check(!audio->restartRequired(),"restoring the original startup number clears restart feedback");
    } else {
        if(playing) {select(0);check(audio->restartRequired() && unchanged(),"System default is saved without restarting the current playing output");}
        select(selected);
        check(audio->values().value("outputDevice").toString()==chosen && audio->lastError().isEmpty(),
              "actual endpoint editor saves an enumerated real device identifier");
        const bool remember=audio->values().value("rememberVolume").toBool();
        type(QStringLiteral("40"));
        check(audio->values().value("rememberVolume").toBool()==remember
              && std::abs(audio->values().value("startupVolume").toDouble()-40.)<.0001 && unchanged(),
              "preparing the startup number preserves Remember policy and current output");
        type(QStringLiteral("126"));
        check(std::abs(audio->values().value("startupVolume").toDouble()-40.)<.0001 && !audio->lastError().isEmpty(),
              "actual numeric editor refuses volume above the native one-hundred-twenty-five percent limit");
        click(reveal(QStringLiteral("audioFixedVolume")));
        check(!audio->values().value("rememberVolume").toBool() && unchanged(),
              "actual fixed-startup policy saves without changing the current audio output");
        struct SharingLock {
            HANDLE handle;
            ~SharingLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
        } lock{CreateFileW(config.toStdWString().c_str(),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr)};
        check(lock.handle!=INVALID_HANDLE_VALUE,"owned config locks for a genuine native write failure");
        type(QStringLiteral("65"));
        const bool failed=std::abs(audio->values().value("startupVolume").toDouble()-40.)<.0001 && !audio->lastError().isEmpty();
        CloseHandle(lock.handle);lock.handle=INVALID_HANDLE_VALUE;
        check(failed,"real locked-save failure rolls back policy and exposes retry feedback");
        type(QStringLiteral("65"));
        check(std::abs(audio->values().value("startupVolume").toDouble()-65.)<.0001 && audio->lastError().isEmpty(),
              "same actual startup-volume editor retries successfully after config unlock");
    }
    click(reveal(QStringLiteral("audiorememberVolume")));
    check(audio->values().value("rememberVolume").toBool() && unchanged(),"actual Remember action preserves current playback");
    auto *fixed=reveal(QStringLiteral("audioFixedVolume"));
    QTest::keyClick(window,Qt::Key_Space);QTest::qWait(100);
    check(!audio->values().value("rememberVolume").toBool() && fixed->hasActiveFocus() && unchanged(),
          "keyboard fixed-policy activation preserves exact current output and primary state");
    if(reopen || playing)check(!audio->restartRequired(),"restoring the cold startup policies clears restart feedback");
    else check(audio->restartRequired(),"changed endpoint and fixed policy retain truthful restart feedback");
    capture(QStringLiteral("native-audio-device-settings.png")); // Owned hardware labels stay local.
    dismiss();open();field=reveal(QStringLiteral("audiostartupVolume"));
    check(std::abs(field->property("text").toString().remove(QLatin1Char('%')).toDouble()-65.)<.0001,
          "real dialog reopen retains the saved startup number");
    dismiss();check(unchanged(),"Audio editor validation retry and reopen preserve primary and output identity");
    report.insert("selectedEndpointSha256",QString::fromLatin1(QCryptographicHash::hash(chosen.toUtf8(),QCryptographicHash::Sha256).toHex()));
    report.insert("activeEndpointVerifiedOnColdStart",reopen||playing);
    report.insert("nativeAudioDeviceSettingsAccepted",true);
}
