// SPDX-License-Identifier: GPL-2.0-or-later
// Real library origins and rendered metadata; run only after UI access resumes.
void exercisePlaylistOrigin(const QString &mode)
{
    auto *current=context->getCurrentMedia();auto *profiles=context->getPlayerProfiles();
    const auto first=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("first.flac")));
    const auto second=QUrl::fromLocalFile(QDir(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")).filePath(QStringLiteral("second.flac")));
    check(profiles->beginLayoutEdit(),"owned origin fixture starts a real music layout draft");
    check(profiles->layout()->insertTile(1,0,0,ControlListModel::JUXTO_PLAYLIST_NAME),"owned music layout includes Playlist Name metadata");
    check(profiles->saveLayoutDraft(),"checked profile save publishes the metadata tile");
    auto *library=context->getMediaLibrary();
    const auto ready=[&](const QString &name) {
        context->setPlayerView(true);
        wait([&]{return player()->getUrl()==first&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PLAYING;},"real owned audio decodes for the origin metadata",15000);
        wait([&]{return current->playlistName()==name&&!current->busy();},"native current-media worker resolves the explicit origin name");
        auto *bar=named(QStringLiteral("juxtopposedLivePlaybackBar"));
        check(bar&&expression(bar,QStringLiteral("mediaMetadata.playlist")).toString()==name,"actual live bar consumes the native origin name");
        check(expression(bar,QStringLiteral("layoutRow.itemFor(%1).children[0].text").arg(ControlListModel::JUXTO_PLAYLIST_NAME)).toString()==name,"actual metadata label renders the resolved playlist name");
    };
    if(mode==QStringLiteral("playlist-origin-disabled")) {
        check(!library,"Library-off origin case has no library owner");
        context->getIntf()->p_mainPlaylistController->append(QVector<vlc::playlist::Media>{vlc::playlist::Media{first.toString(QUrl::FullyEncoded),{}, {}}},true);
        ready(QString{});
        check(!current->available()&&current->error().isEmpty(),"Library-off source metadata remains empty without enabling or querying a library");
        report.insert("nativePlaylistOriginDisabledAccepted",true);return;
    }
    check(library!=nullptr,"actual library owner exists for the named origins");
    library->addFolder(QUrl::fromLocalFile(qEnvironmentVariable("VLC_OWNED_AUDIO_FOLDER")));
    wait([&]{return library->idle()&&library->homeActivity()->mediaCount()==3;},"native library indexes only the three generated origin fixtures",30000);
    struct Collections {MLItemId a,b,first;bool ready=false;};auto collections=std::make_shared<Collections>();
    const auto aName=QStringLiteral("Owned A <mix> & Friends Ää"),bName=QStringLiteral("Owned B 日本語");
    library->runOnMLThread<Collections>(library,[first,second,aName,bName](vlc_medialibrary_t *ml,Collections &value) {
        ml_unique_ptr<vlc_ml_media_t> one{vlc_ml_get_media_by_mrl(ml,qtu(first.toString(QUrl::FullyEncoded)))};
        ml_unique_ptr<vlc_ml_media_t> two{vlc_ml_get_media_by_mrl(ml,qtu(second.toString(QUrl::FullyEncoded)))};
        ml_unique_ptr<vlc_ml_playlist_t> a{vlc_ml_playlist_create(ml,qtu(aName))},b{vlc_ml_playlist_create(ml,qtu(bName))};
        if(!one||!two||!a||!b)return;
        const int64_t ids[]={one->i_id,two->i_id};
        if(vlc_ml_playlist_append(ml,a->i_id,ids,2)!=VLC_SUCCESS||vlc_ml_playlist_append(ml,b->i_id,ids,1)!=VLC_SUCCESS)return;
        value.a={a->i_id,VLC_ML_PARENT_PLAYLIST};value.b={b->i_id,VLC_ML_PARENT_PLAYLIST};value.first={one->i_id,VLC_ML_PARENT_UNKNOWN};
    },[collections](quint64,Collections &value){*collections=value;collections->ready=true;});
    wait([&]{return collections->ready;},"two real native playlists share the same owned media");
    check(collections->a.id>0&&collections->b.id>0&&collections->a.id!=collections->b.id,"origin collections have distinct actual native identities");
    library->addAndPlay(collections->a);ready(aName);
    check(expression(named(QStringLiteral("juxtopposedLivePlaybackBar")),QStringLiteral("layoutRow.itemFor(%1).children[0].textFormat===Text.PlainText").arg(ControlListModel::JUXTO_PLAYLIST_NAME)).toBool(),"actual playlist name preserves literal markup-like characters");
    {vlc_player_locker lock(context->getIntf()->p_player);vlc_player_Pause(context->getIntf()->p_player);}
    wait([]{return player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;},"primary pauses for metadata-only rename and deletion");
    const auto input=player()->getInput();const auto clock=nativeTime();const auto rate=player()->getRate();const auto volume=player()->getVolume();const auto muted=player()->isMuted();
    const auto unchanged=[&]{return player()->getInput()==input&&nativeTime()==clock&&player()->getRate()==rate&&player()->getVolume()==volume&&player()->isMuted()==muted&&player()->getPlayingState()==PlayerController::PLAYING_STATE_PAUSED;};
    const auto renamed=QStringLiteral("Renamed <mix> & Friends Ää");auto rename=std::make_shared<bool>(false);
    library->runOnMLThread(library,[id=collections->a.id,renamed](vlc_medialibrary_t *ml){vlc_ml_playlist_rename(ml,id,qtu(renamed));},[rename]{*rename=true;});
    wait([&]{return *rename&&current->playlistName()==renamed;},"actual playlist rename event updates the native metadata name");
    check(unchanged(),"renaming the source playlist preserves exact paused primary state");
    capture(QStringLiteral("native-playlist-origin-renamed.png"));
    auto removed=std::make_shared<bool>(false);
    library->runOnMLThread(library,[id=collections->a.id](vlc_medialibrary_t *ml){vlc_ml_playlist_delete(ml,id);},[removed]{*removed=true;});
    wait([&]{return *removed&&current->playlistName().isEmpty()&&context->getIntf()->p_mainPlaylistController->currentSourcePlaylistId()==0;},"deleting the source playlist invalidates its queued origins");
    check(unchanged()&&context->getIntf()->p_mainPlaylistController->count()==2,"library playlist deletion keeps queued media and paused primary alive");
    MLPlaylistModel selected;selected.setMl(library);selected.setParentId(collections->b);
    wait([&]{return selected.rowCount()==1;},"real playlist model exposes its shared owned track");
    selected.addAndPlay(QModelIndexList{selected.index(0,0)});ready(bName);
    check(context->getIntf()->p_mainPlaylistController->currentSourcePlaylistId()==collections->b.id,"model selection carries its actual playlist origin for the same URL");
    capture(QStringLiteral("native-playlist-origin-selected.png"));
    context->getIntf()->p_mainPlaylistController->append(QVector<vlc::playlist::Media>{vlc::playlist::Media{first.toString(QUrl::FullyEncoded),{}, {}}},true);
    ready(QString{});
    check(context->getIntf()->p_mainPlaylistController->currentSourcePlaylistId()==0,"generic playback of the identical URL does not guess a library playlist membership");
    report.insert("nativePlaylistOriginAccepted",true);
}
