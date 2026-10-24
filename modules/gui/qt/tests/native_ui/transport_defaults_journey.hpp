// SPDX-License-Identifier: GPL-2.0-or-later
// Real controller/config/profile/restart regression, private GUI only.
void exerciseTransportDefaults(const QString &mode)
{
    auto *controller=context->getIntf()->p_mainPlaylistController;
    auto *preferences=context->getPlaybackPreferences();
    auto *profiles=context->getPlayerProfiles();
    home();
    const bool initial=mode==QStringLiteral("transport-initial");
    const int expected=initial?0:mode==QStringLiteral("transport-all")?2:
        mode==QStringLiteral("transport-current")?1:0;
    wait([&]{return int(controller->getRepeatMode())==expected;},"native startup restores the exact saved repeat mode");
    check(config_GetInt("repeat")==int(expected==1)&&config_GetInt("loop")==int(expected==2),
          "native startup repeat and loop flags remain distinct booleans");
    check(preferences->values().value(QStringLiteral("repeatMode")).toInt()==expected,
          "Playback settings reflect the actual restored native defaults");
    if(mode==QStringLiteral("transport-none")) {
        check(!controller->isRandom()&&!config_GetInt("random"),"fresh process restores disabled shuffle");
        report.insert("transportDefaultsAccepted",true);return;
    }
    const auto source=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    context->getMediaLibrary()->addAndPlay(source);
    wait([&]{return player()->getUrl()==source&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},
         "real audio decodes before changing transport policy",15000);
    wait([]{return nativeTime()>VLC_TICK_FROM_MS(200);},"real clock advances before changing transport policy");
    const auto input=player()->getInput();const auto clock=nativeTime();
    const int next=initial?2:expected==2?1:0;
    controller->toggleRepeatMode();
    wait([&]{return int(controller->getRepeatMode())==next;},"toolbar repeat action reaches the native playlist");
    check(config_GetInt("repeat")==int(next==1)&&config_GetInt("loop")==int(next==2),
          "toolbar saves Repeat All, Repeat Current and Off as separate native boolean flags");
    wait([&]{return preferences->values().value(QStringLiteral("repeatMode")).toInt()==next;},
         "Playback settings observe the toolbar change without reopening the page");
    if(initial || expected==2) {
        controller->toggleRandom();
        const bool random=initial;
        wait([&]{return controller->isRandom()==random&&preferences->values().value(QStringLiteral("shuffle")).toBool()==random;},
             "shuffle toolbar and Playback settings share the actual native policy");
        check(config_GetInt("random")==int(random),"shuffle writes a native boolean default");
    }
    wait([&]{return !profiles->unsaved()&&!profiles->busy();},"transport defaults checkpoint into the selected profile");
    PlayerProfilesStore saved(context->getIntf()->mainSettings);QString error;
    check(saved.reload({},error)&&saved.indexOf(saved.selectedId())>=0,"saved profile catalog can be read independently");
    const auto playback=saved.profiles()[saved.indexOf(saved.selectedId())].snapshot.value(QStringLiteral("playback")).toMap();
    check(playback.value(QStringLiteral("repeat")).toBool()==(next==1)
          &&playback.value(QStringLiteral("loop")).toBool()==(next==2),
          "selected profile records the two real repeat flags");
    wait([&]{return nativeTime()>clock;},"decoded audio clock continues through transport policy changes");
    check(player()->getInput()==input&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING,
          "transport policy changes preserve the primary input and playing state");
    report.insert("transportDefaultsAccepted",true);
}
