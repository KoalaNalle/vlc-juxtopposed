// SPDX-License-Identifier: GPL-2.0-or-later
// Private ordinary-widget/QML/native-service journey; never in the ordinary GUI.
QObject *profilePopup()
{
    for (auto *object : objects()) if (object->objectName() == QStringLiteral("juxtopposedPlayerProfilesDialog")
        && object->property("visible").toBool()) return object;
    return nullptr;
}
void profileMenuChoice(const QString &text)
{
    click(named(QStringLiteral("juxtopposedStripmenu")));
    wait([] {return menu()!=nullptr;},"actual global menu opens for profile selection");
    auto *global=menu();QAction *entry=nullptr;
    for (auto *action:global->actions()) if(action->menu()&&action->text().startsWith(QStringLiteral("Profile (")))entry=action;
    check(entry!=nullptr,"actual checked Profile submenu exists");
    QTest::mouseClick(global,Qt::LeftButton,Qt::NoModifier,global->actionGeometry(entry).center());
    wait([&]{return entry->menu()->isVisible();},"actual pointer click opens Profile submenu");
    auto *submenu=entry->menu();QAction *choice=nullptr;
    for(auto *action:submenu->actions()) if(action->text()==text)choice=action;
    check(choice&&choice->isEnabled(),"requested native profile action is enabled");
    QTest::mouseClick(submenu,Qt::LeftButton,Qt::NoModifier,submenu->actionGeometry(choice).center());
    wait([] {return !menu();},"profile native menu closes after selection");
}
void exerciseProfiles(const QString &mode)
{
    auto *profiles=context->getPlayerProfiles();auto *provider=DialogsProvider::getInstance();
    check(profiles&&profiles->available()&&!profiles->needsRecovery(),"real MainCtx owns readable checked player profiles");
    home();
    if(mode==QStringLiteral("profiles-reopen")) {
        check(profiles->selectedName()==QStringLiteral("Owned Travel Ää 日本語"),"fresh native OS process restores the selected created profile");
        check(context->getPlaybackPreferences()->values().value(QStringLiteral("defaultPlaybackSpeed")).toFloat()==1.25f,
              "fresh native OS process restores saved playback default");
        float nativeRate;
        {vlc_player_locker lock(context->getIntf()->p_player);nativeRate=vlc_player_GetRate(context->getIntf()->p_player);}
        check(qAbs(nativeRate-1.25f)<0.0001f,"restart-only speed default reaches the actual new native player variable");
        // The empty-input GUI cache intentionally starts at 1. Verify actual
        // playback with fresh media, avoiding a remembered per-media rate.
        const auto source=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("second.flac")));
        context->getMediaLibrary()->addAndPlay(source);
        wait([&]{return player()->getUrl()==source&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},
             "fresh native media starts after profile restart",15000);
        wait([]{return nativeTime()>0&&qAbs(player()->getRate()-1.25f)<0.0001f;},
             "actual decoded playback and GUI timing use the restarted profile speed");
        check(context->getVideoPreferences()->values().value(QStringLiteral("brightness")).toFloat()==1.25f,
              "fresh native OS process restores saved video default");
        check(!profilePopup()&&!provider->qmlDialogVisible(),"completed profiles do not reopen a modal after restart");
        capture(QStringLiteral("native-profile-reopened.png"));report.insert("nativeProfilesReopenAccepted",true);return;
    }
    check(profiles->profiles().size()==4&&profiles->selectedId()==QStringLiteral("movies"),"isolated native catalog begins with the four authored profiles");
    const auto source=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    context->getMediaLibrary()->addAndPlay(source);
    wait([&]{return player()->getUrl()==source&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},
         "owned audio actually decodes before native profile operations",15000);
    wait([]{return nativeTime()>VLC_TICK_FROM_MS(200);},"primary clock advances before profile operations");
    const bool playing=mode==QStringLiteral("profiles-playing");
    if(!playing) {
        {vlc_player_locker lock(context->getIntf()->p_player);vlc_player_Pause(context->getIntf()->p_player);}
        wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"primary playback pauses before profile checks");
    }
    home();const auto input=player()->getInput();const auto clock=nativeTime();const auto position=player()->getPosition();const auto rate=player()->getRate();
    auto volume=[&]{vlc_player_locker lock(context->getIntf()->p_player);return vlc_player_aout_GetVolume(context->getIntf()->p_player);};
    auto mute=[&]{vlc_player_locker lock(context->getIntf()->p_player);return vlc_player_aout_IsMuted(context->getIntf()->p_player);};
    const auto initialVolume=volume();const auto initialMute=mute();
    const auto unchanged=[&]{return player()->getInput()==input&&qAbs(player()->getRate()-rate)<0.0001f
        &&qAbs(volume()-initialVolume)<0.0001&&mute()==initialMute
        &&(playing?player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING&&nativeTime()>=clock
            :player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED&&nativeTime()==clock&&player()->getPosition()==position);};
    const auto globals=context->getGeneralPreferences()->values();const auto privacy=context->getPrivacyPreferences()->values();
    const auto beforeProfiles=context->getIntf()->mainSettings->value(PlayerProfilesStore::settingsKey());
    profileMenuChoice(QStringLiteral("New"));wait([]{return profilePopup()!=nullptr;},"native New opens the movable profile form");
    check(provider->qmlDialogVisible()&&!named(QStringLiteral("playerProfileSave"))->isEnabled(),"empty native profile name cannot save and owns the modal guard");
    click(named(QStringLiteral("playerProfileCancel")));wait([]{return !profilePopup();},"native Cancel closes the profile form");
    check(context->getIntf()->mainSettings->value(PlayerProfilesStore::settingsKey())==beforeProfiles&&unchanged(),
          "native Cancel preserves saved catalog and exact primary media state");
    profileMenuChoice(QStringLiteral("New"));wait([]{return named(QStringLiteral("playerProfileName"))!=nullptr;},"native profile name editor appears");
    named(QStringLiteral("playerProfileName"))->setProperty("text",QStringLiteral("Owned Travel Ää 日本語"));
    capture(QStringLiteral("native-profile-new.png"));click(named(QStringLiteral("playerProfileSave")));
    wait([&]{return !profilePopup()&&profiles->selectedName()==QStringLiteral("Owned Travel Ää 日本語");},"actual Create saves and selects the Unicode profile");
    check(profiles->profiles().size()==5&&!profiles->unsaved()&&unchanged(),"actual native profile creation preserves primary playback");
    check(context->getPlaybackPreferences()->setValue(QStringLiteral("defaultPlaybackSpeed"),1.25),"real Playback owner saves the created profile speed default");
    check(context->getVideoPreferences()->setValue(QStringLiteral("brightness"),1.25),"real Video owner saves the created profile picture default");
    check(context->getAudioPreferences()->setValue(QStringLiteral("audioDelayMs"),100),"real Audio owner saves the created profile delay default");
    check(context->getSubtitlesPreferences()->setValue(QStringLiteral("subtitleDelay"),200),"real Subtitles owner saves the created profile delay default");
    wait([&]{return !profiles->unsaved()&&!profiles->busy();},"typed media defaults checkpoint into the current native profile");
    check(unchanged(),"restart and next-input profile edits preserve current primary input rate clock volume and mute");
    profileMenuChoice(QStringLiteral("Shows"));
    wait([&]{return profiles->selectedId()==QStringLiteral("shows")&&!profiles->needsRecovery();},"actual Shows action finishes its checked native switch");
    check(context->getPlaybackPreferences()->values().value(QStringLiteral("defaultPlaybackSpeed")).toFloat()==1.f
        &&context->getVideoPreferences()->values().value(QStringLiteral("brightness")).toFloat()==1.f
        &&context->getAudioPreferences()->values().value(QStringLiteral("audioDelayMs")).toInt()==0
        &&context->getSubtitlesPreferences()->values().value(QStringLiteral("subtitleDelay")).toInt()==0,
          "actual switch restores the four distinct typed media defaults");
    check(unchanged(),"actual native Shows switch preserves the primary media and its current apply modes");
    profileMenuChoice(QStringLiteral("Owned Travel Ää 日本語"));
    wait([&]{return profiles->selectedName()==QStringLiteral("Owned Travel Ää 日本語")&&!profiles->needsRecovery();},"actual Unicode profile action switches back");
    check(context->getPlaybackPreferences()->values().value(QStringLiteral("defaultPlaybackSpeed")).toFloat()==1.25f
        &&context->getVideoPreferences()->values().value(QStringLiteral("brightness")).toFloat()==1.25f
        &&context->getAudioPreferences()->values().value(QStringLiteral("audioDelayMs")).toInt()==100
        &&context->getSubtitlesPreferences()->values().value(QStringLiteral("subtitleDelay")).toInt()==200,
          "actual switch back retains each independently edited media default");
    check(unchanged()&&context->getGeneralPreferences()->values()==globals&&context->getPrivacyPreferences()->values()==privacy,
          "actual profile switches preserve shared appearance privacy and primary playback state");
    profileMenuChoice(QStringLiteral("New"));wait([]{return named(QStringLiteral("playerProfileName"))!=nullptr;},"duplicate-name native form opens");
    named(QStringLiteral("playerProfileName"))->setProperty("text",QStringLiteral("Movies"));click(named(QStringLiteral("playerProfileSave")));
    check(profilePopup()&&!profiles->lastError().isEmpty()&&profiles->profiles().size()==5&&unchanged(),"duplicate native name remains editable without changing accepted profiles");
    capture(QStringLiteral("native-profile-save-error.png"));click(named(QStringLiteral("playerProfileCancel")));
    wait([]{return !profilePopup();},"duplicate-name cancellation closes normally");
    if(playing)wait([&]{return nativeTime()>clock+VLC_TICK_FROM_MS(500);},"real primary clock continues through all playing profile operations");
    report.insert("nativeProfilesAccepted",true);report.insert("primaryPlaying",playing);
}
