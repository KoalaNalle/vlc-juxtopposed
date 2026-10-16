// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "maininterface/library_preferences_model.hpp"
#include "qt.hpp"
#include "maininterface/compositor.hpp"
#include "medialibrary/mlhomeactivitydata.hpp"
#include <vlc/vlc.h>
#include <vlc_configuration.h>
#include <vlc_interface.h>
#include <vlc_input_item.h>
#include <vlc_media_library.h>
#include <vlc_modules.h>
#include <vlc_playlist.h>
#include <vlc_preparser.h>
#include <vlc_player.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QMutex>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <memory>
#ifdef _WIN32
# include <windows.h>
#endif
extern "C" vlc_object_t *vlc_test_library_object(libvlc_instance_t *);
extern "C" bool vlc_test_library_single_instance(vlc_object_t *);
extern "C" int vlc_test_library_open_mode(vlc_object_t *);

class TestLibraryPreferences final:public QObject
{
    Q_OBJECT
    QTemporaryDir m_temp;
    QString m_output,m_case,m_config,m_userdata,m_media;
    libvlc_instance_t *m_instance=nullptr;
    qt_intf_t *m_intf=nullptr;
    intf_thread_t *m_queueIntf=nullptr;
    std::unique_ptr<LibraryPreferencesModel> m_model;
    QLibrary m_winmsg;
#ifdef _WIN32
    using CopyData=LRESULT (*)(vlc_object_t *,const void *,size_t);
    CopyData m_copy=nullptr;
#endif
    static QByteArray bytes(const QString &path){QFile file(path);return file.open(QIODevice::ReadOnly)?file.readAll():QByteArray{};}
    static bool write(const QString &path,const QByteArray &value){QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write(value)==value.size();}
    static QString uri(const QString &path){return QString::fromUtf8(QUrl::fromLocalFile(path).toEncoded());}
    static void environment(const char *name,const QByteArray &value)
    {
        qputenv(name,value);
#ifdef _WIN32
        // Qt/CRT environment updates alone are not reliably inherited by the
        // real CreateProcessW preparser. Update its OS environment as well.
        const auto key=QString::fromLatin1(name),text=QString::fromUtf8(value);
        SetEnvironmentVariableW(reinterpret_cast<LPCWSTR>(key.utf16()),reinterpret_cast<LPCWSTR>(text.utf16()));
#endif
    }
    static void unsetEnvironment(const char *name)
    {
        qunsetenv(name);
#ifdef _WIN32
        const auto key=QString::fromLatin1(name);
        SetEnvironmentVariableW(reinterpret_cast<LPCWSTR>(key.utf16()),nullptr);
#endif
    }
    static QString text(const char *name){char *raw=config_GetPsz(name);const auto out=QString::fromUtf8(raw?raw:"");free(raw);return out;}
    static LibraryPreferencesModel::Consumers verified(){LibraryPreferencesModel::Consumers c;c.instancePolicy=c.openingQueue=c.metadataFetch=c.hiddenAttributes=true;return c;}
    vlc_object_t *root()const{return vlc_test_library_object(m_instance);}
    vlc_playlist_t *playlist()const{return vlc_intf_GetMainPlaylist(m_queueIntf);}
    vlc_medialibrary_t *library()const{return vlc_ml_instance_get(m_intf);}
    void closeNative(){m_model.reset();if(m_queueIntf){vlc_object_delete(m_queueIntf);m_queueIntf=nullptr;}if(m_intf){vlc_object_delete(m_intf);m_intf=nullptr;}if(m_instance){libvlc_release(m_instance);m_instance=nullptr;}}
    bool openNative(bool ml=false,bool automatic=false,bool network=false,const QByteArray &recursion={},int timeout=-1)
    {
        QList<QByteArray> args={QByteArray("--config=")+m_config.toUtf8(),"--no-ignore-config","--quiet",
            "--no-one-instance","--no-one-instance-when-started-from-file","--extraintf=","--control=",
            "--no-ml-network-discovery","--aout=dummy","--vout=dummy","--no-video",
            "--fetch-art-threads=1",
            ml?"--media-library":"--no-media-library",automatic?"--auto-preparse":"--no-auto-preparse",
            network?"--metadata-network-access":"--no-metadata-network-access"};
        if(!recursion.isEmpty())args.append(QByteArray("--recursive=")+recursion);
        if(timeout>=0)args.append(QByteArray("--preparse-timeout=")+QByteArray::number(timeout));
        QList<const char *> argv;for(const auto &arg:args)argv.append(arg.constData());
        m_instance=libvlc_new(int(argv.size()),argv.data());if(!m_instance)return false;
        m_intf=vlc_object_create<qt_intf_t>(root());m_queueIntf=vlc_object_create<intf_thread_t>(root());return m_intf&&m_queueIntf;
    }
    void makeModel(bool enabled=true){m_model=std::make_unique<LibraryPreferencesModel>(m_intf,enabled?verified():LibraryPreferencesModel::Consumers{});}
    QString copy(const QString &source,const QString &relative)
    {const auto target=QDir(m_case).filePath(relative);if(!QDir().mkpath(QFileInfo(target).absolutePath())||!QFile::copy(QDir(m_media).filePath(source),target))return {};return target;}
    QStringList queueUris()const
    {
        QStringList rows;auto *pl=playlist();vlc_playlist_Lock(pl);
        for(size_t i=0;i<vlc_playlist_Count(pl);++i){char *u=input_item_GetURI(vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,i)));rows.append(QString::fromUtf8(u?u:""));free(u);}
        vlc_playlist_Unlock(pl);return rows;
    }
    bool append(const QString &url)
    {const auto raw=url.toUtf8();auto *item=input_item_New(raw.constData(),nullptr);if(!item)return false;auto *pl=playlist();vlc_playlist_Lock(pl);const auto result=vlc_playlist_AppendOne(pl,item);vlc_playlist_Unlock(pl);input_item_Release(item);return result==VLC_SUCCESS;}
    bool fixtureProviders()
    {
        for(const auto *cap:{"meta fetcher","art finder"}) {
            module_t **modules=nullptr;size_t strict=0;const auto count=vlc_module_match(cap,nullptr,false,&modules,&strict);
            const bool owned=count>0&&QString::fromUtf8(module_get_object(modules[0]))==QStringLiteral("library_fixture_fetcher")&&module_get_score(modules[0])==1000000;
            free(modules);if(!owned)return false;
        }
        var_Create(root(),"library-fixture-active",VLC_VAR_BOOL);var_SetBool(root(),"library-fixture-active",true);
        for(const auto *name:{"library-fixture-local","library-fixture-network"}){var_Create(root(),name,VLC_VAR_INTEGER);var_SetInteger(root(),name,0);}
        var_Create(root(),"library-fixture-art",VLC_VAR_STRING);const auto art=uri(QDir(m_media).filePath(QStringLiteral("art.png"))).toUtf8();var_SetString(root(),"library-fixture-art",art.constData());return true;
    }
    struct Parse {
        QSemaphore done;QMutex lock;QStringList urls;int result=VLC_EGENERIC;bool sameRoot=true;
        static void collect(input_item_node_t *node,QStringList &rows){for(int i=0;i<node->i_children;++i){auto *child=node->pp_children[i];char *u=input_item_GetURI(child->p_item);rows.append(QString::fromUtf8(u?u:""));free(u);collect(child,rows);}}
        static void subtree(vlc_preparser_req *request,input_item_node_t *node,void *opaque){auto &p=*static_cast<Parse *>(opaque);{QMutexLocker hold(&p.lock);p.sameRoot=p.sameRoot&&node->p_item==vlc_preparser_req_GetItem(request);collect(node,p.urls);}input_item_node_Delete(node);}
        static void ended(vlc_preparser_req *,int status,void *opaque){auto &p=*static_cast<Parse *>(opaque);p.result=status;p.done.release();}
    };
    int providerCalls(bool network)const
    {
        return bytes(qEnvironmentVariable("VLC_LIBRARY_FIXTURE_LOG")
            + (network ? QStringLiteral("-network.log") : QStringLiteral("-local.log"))).count('\n');
    }
    QString queuedTitle()const
    {
        auto *pl=playlist();vlc_playlist_Lock(pl);
        char *raw=vlc_playlist_Count(pl)?input_item_GetTitle(vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,0))):nullptr;
        const auto title=QString::fromUtf8(raw?raw:"");free(raw);vlc_playlist_Unlock(pl);return title;
    }
    bool providerInterrupted()const
    {return !bytes(qEnvironmentVariable("VLC_LIBRARY_FIXTURE_LOG")+QStringLiteral("-interrupted.log")).isEmpty();}
    struct FetchResult {
        QSemaphore done;
        int result=VLC_EGENERIC;
        int calls=0;
        static void ended(vlc_preparser_req *,int status,void *opaque) {
            auto &state=*static_cast<FetchResult *>(opaque);
            state.result=status;++state.calls;state.done.release();
        }
    };
    bool parseFolder(const QString &folder,bool forceHidden,QStringList &urls,bool external=false,bool localFetch=false)
    {
        const int parseOptions=VLC_PREPARSER_TYPE_PARSE|(localFetch?VLC_PREPARSER_TYPE_FETCHMETA_LOCAL:0);
        const vlc_preparser_cfg cfg={parseOptions,1,0,VLC_TICK_FROM_SEC(10),external};
        auto *parser=vlc_preparser_New(root(),&cfg);if(!parser)return false;
        const auto raw=uri(folder).toUtf8();auto *item=input_item_New(raw.constData(),nullptr);
        if(!item){vlc_preparser_Delete(parser);return false;}
        if(forceHidden)input_item_AddOption(item,"show-hiddenfiles",VLC_INPUT_OPTION_TRUSTED);
        static const vlc_preparser_cbs callbacks={Parse::ended,Parse::subtree,nullptr};
        Parse state;auto *request=vlc_preparser_req_NewParse(parser,item,parseOptions|VLC_PREPARSER_OPTION_SUBITEMS,&callbacks,&state);
        const bool submitted=request&&vlc_preparser_Submit(parser,request)==VLC_SUCCESS;
        const bool finished=submitted&&state.done.tryAcquire(1,15000);
        if(request){if(submitted&&!finished)vlc_preparser_Cancel(parser,request);vlc_preparser_req_Release(request);}
        vlc_preparser_Delete(parser);input_item_Release(item);
        urls=state.urls;return finished&&state.result==VLC_SUCCESS&&state.sameRoot;
    }
    static QByteArray payload(const QList<QByteArray> &args,int mode)
    {QByteArray out;const int count=int(args.size());out.append(reinterpret_cast<const char *>(&count),sizeof(count));out.append(reinterpret_cast<const char *>(&mode),sizeof(mode));for(const auto &arg:args){const size_t size=size_t(arg.size()+1);out.append(reinterpret_cast<const char *>(&size),sizeof(size));out.append(arg);out.append('\0');}return out;}
private slots:
    void initTestCase()
    {
        QVERIFY(m_temp.isValid());m_output=qEnvironmentVariable("VLC_LIBRARY_OUTPUT");
        if(m_output.isEmpty())m_output=m_temp.path();else{QVERIFY(QFileInfo(m_output).isAbsolute()&&!QFileInfo::exists(m_output));QVERIFY(QDir().mkpath(m_output));}
        m_media=qEnvironmentVariable("VLC_LIBRARY_MEDIA");QVERIFY(QFileInfo(m_media).isAbsolute());
        for(const auto *file:{"one.wav","two.wav","visible.wav","hidden.wav",".dot.wav","sub/child.wav","art.png","fixture-identities.json"})QVERIFY(QFileInfo::exists(QDir(m_media).filePath(QString::fromLatin1(file))));
#ifdef _WIN32
        const auto plugin=qEnvironmentVariable("VLC_LIBRARY_WINMSG_PLUGIN");QVERIFY(QFileInfo(plugin).isAbsolute()&&QFileInfo::exists(plugin));
        m_winmsg.setFileName(plugin);QVERIFY2(m_winmsg.load(),qPrintable(m_winmsg.errorString()));
        m_copy=reinterpret_cast<CopyData>(m_winmsg.resolve("vlc_test_library_copydata"));QVERIFY(m_copy);
        QVERIFY(write(QDir(m_output).filePath(QStringLiteral("winmsg-sha256.txt")),QCryptographicHash::hash(bytes(plugin),QCryptographicHash::Sha256).toHex()));
        QVariantMap loaded;
        for(const auto *name:{L"libvlc.dll",L"libvlccore.dll"}){const auto module=GetModuleHandleW(name);QVERIFY(module);wchar_t path[32768];const auto length=GetModuleFileNameW(module,path,32768);QVERIFY(length>0&&length<32768);const auto file=QString::fromWCharArray(path,int(length));loaded[QString::fromWCharArray(name)]=QVariantMap{{QStringLiteral("path"),file},{QStringLiteral("sha256"),QString::fromLatin1(QCryptographicHash::hash(bytes(file),QCryptographicHash::Sha256).toHex())}};}
        QVERIFY(write(QDir(m_output).filePath(QStringLiteral("loaded-core-identities.json")),QJsonDocument::fromVariant(loaded).toJson()));
#else
        QFAIL("This desired native pair targets the configured Windows package; IPC is not claimed on other platforms.");
#endif
    }
    void init()
    {
        m_case=QDir(m_output).filePath(QString::fromLatin1(QTest::currentTestFunction()));if(const auto *tag=QTest::currentDataTag();tag&&*tag)m_case+='/'+QString::fromLatin1(tag);
        QVERIFY(QDir().mkpath(m_case));m_config=QDir(m_case).filePath(QStringLiteral("vlcrc"));m_userdata=QDir(m_case).filePath(QStringLiteral("userdata"));QVERIFY(QDir().mkpath(m_userdata));environment("VLC_USERDATA_PATH",m_userdata.toUtf8());
        environment("VLC_LIBRARY_FIXTURE_LOG",QDir(m_case).filePath(QStringLiteral("provider")).toUtf8());
        environment("VLC_LIBRARY_FIXTURE_ART",uri(QDir(m_media).filePath(QStringLiteral("art.png"))).toUtf8());
        unsetEnvironment("VLC_LIBRARY_FIXTURE_DELAY_MS");
        QVERIFY(openNative());makeModel();
    }
    void cleanup(){if(m_instance){QVariantMap trace{{QStringLiteral("values"),m_model?m_model->values():QVariantMap{}},{QStringLiteral("queue"),m_queueIntf?queueUris():QStringList{}}};for(const auto *name:{"library-fixture-local","library-fixture-network"})if(var_Type(root(),name))trace[QString::fromLatin1(name)]=qlonglong(var_GetInteger(root(),name));write(QDir(m_case).filePath(QStringLiteral("native-trace.json")),QJsonDocument::fromVariant(trace).toJson());}closeNative();}
    void exactApiAndDefaultGates()
    {
        makeModel(false);QCOMPARE(m_model->values().size(),7);QCOMPARE(m_model->values().keys(),m_model->capabilities().keys());QCOMPARE(m_model->applyModes().size(),7);QVERIFY(!m_model->libraryAvailable());QVERIFY(!m_model->restartRequired());
        for(const auto *key:{"singleInstance","fileManagerSingleInstance","enqueueInSingleInstance","metadataSource","showHiddenFiles"})QVERIFY(!m_model->capabilities().value(QString::fromLatin1(key)).toBool());
        QVERIFY(!m_model->setValue(QStringLiteral("singleInstance"),true));QVERIFY(!m_model->lastError().isEmpty());m_model->reload();QVERIFY(m_model->lastError().isEmpty());
    }
    void legacyOpeningProfileRemainsUnmapped()
    {
        config_PutInt("one-instance",false);config_PutInt("one-instance-when-started-from-file",true);config_PutInt("one-instance-file-manager-policy",-1);config_PutInt("playlist-open-mode",-1);config_PutInt("playlist-enqueue",false);makeModel();
        QVERIFY(m_model->legacyFileManagerOnly());QVERIFY(m_model->values().value(QStringLiteral("fileManagerSingleInstance")).toBool());QVERIFY(!m_model->values().value(QStringLiteral("enqueueInSingleInstance")).isValid());
        QVERIFY(m_model->setValue(QStringLiteral("singleInstance"),true));QCOMPARE(config_GetInt("one-instance-file-manager-policy"),int64_t(1));QCOMPARE(config_GetInt("one-instance-when-started-from-file"),int64_t(1));QVERIFY(m_model->restartRequired());
        QVERIFY(m_model->setValue(QStringLiteral("fileManagerSingleInstance"),false));QCOMPARE(config_GetInt("one-instance-file-manager-policy"),int64_t(0));
        QVERIFY(m_model->setValue(QStringLiteral("enqueueInSingleInstance"),false));QCOMPARE(config_GetInt("playlist-open-mode"),int64_t(0));QVERIFY(bytes(m_config).contains("playlist-open-mode=0"));
    }
    void typedInvalidInputsLeaveEverythingUnchanged()
    {
        QVERIFY(m_model->setValue(QStringLiteral("singleInstance"),true));const auto disk=bytes(m_config);const auto values=m_model->values();QSignalSpy changed(m_model.get(),&LibraryPreferencesModel::valuesChanged);
        const QList<QPair<QString,QVariant>> bad={{QStringLiteral("singleInstance"),1},{QStringLiteral("fileManagerSingleInstance"),QStringLiteral("true")},{QStringLiteral("enqueueInSingleInstance"),0.0},{QStringLiteral("showHiddenFiles"),QVariant{}},{QStringLiteral("folderRecursion"),true},{QStringLiteral("folderRecursion"),QStringLiteral("all")},{QStringLiteral("folderRecursion"),QString(80,QChar('x'))},{QStringLiteral("folderRecursion"),QStringLiteral("expand\n")},{QStringLiteral("folderRecursion"),QString(QChar(0xd800))},{QStringLiteral("metadataSource"),QStringLiteral("https://example.invalid")},{QStringLiteral("metadataSource"),QByteArray("local")},{QStringLiteral("useMediaLibrary"),QStringLiteral("false")},{QStringLiteral("not-a-setting"),false}};
        for(const auto &entry:bad){QVERIFY(!m_model->setValue(entry.first,entry.second));QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(bytes(m_config),disk);QCOMPARE(m_model->values(),values);}
        QCOMPARE(changed.size(),0);m_model->reload();QVERIFY(m_model->lastError().isEmpty());
    }
    void genuineNativeStorageAndFreshReads_data(){QTest::addColumn<QString>("key");QTest::addColumn<QVariant>("value");QTest::addColumn<QByteArray>("native");QTest::addColumn<QByteArray>("saved");QTest::addColumn<bool>("restart");QTest::newRow("recursion")<<QStringLiteral("folderRecursion")<<QVariant(QStringLiteral("expand"))<<QByteArray("recursive")<<QByteArray("expand")<<true;QTest::newRow("hidden")<<QStringLiteral("showHiddenFiles")<<QVariant(true)<<QByteArray("show-hiddenfiles")<<QByteArray("1")<<false;QTest::newRow("metadata")<<QStringLiteral("metadataSource")<<QVariant(QStringLiteral("network"))<<QByteArray("metadata-network-access")<<QByteArray("1")<<true;QTest::newRow("library")<<QStringLiteral("useMediaLibrary")<<QVariant(true)<<QByteArray("media-library")<<QByteArray("1")<<true;}
    void genuineNativeStorageAndFreshReads()
    {
        QFETCH(QString,key);QFETCH(QVariant,value);QFETCH(QByteArray,native);QFETCH(QByteArray,saved);QFETCH(bool,restart);
        const auto unrelated=config_GetInt("qt-max-volume");QVERIFY(m_model->capabilities().value(key).toBool());QVERIFY2(m_model->setValue(key,value),qPrintable(m_model->lastError()));QCOMPARE(m_model->values().value(key),value);QVERIFY(bytes(m_config).contains(QByteArray(native+'='+saved)));QCOMPARE(config_GetInt("qt-max-volume"),unrelated);QCOMPARE(m_model->restartRequired(),restart);
        closeNative();QVERIFY(openNative());makeModel();QCOMPARE(m_model->values().value(key),value);QVERIFY(!m_model->restartRequired());
    }
    void realLockedSaveRollbackAndRetry()
    {
#ifdef _WIN32
        QVERIFY(m_model->setValue(QStringLiteral("singleInstance"),true));QVERIFY(m_model->setValue(QStringLiteral("enqueueInSingleInstance"),true));const auto previous=m_model->values();const auto disk=bytes(m_config);const auto oldPolicy=config_GetInt("one-instance-file-manager-policy"),oldMode=config_GetInt("playlist-open-mode"),oldEnqueue=config_GetInt("playlist-enqueue");QSignalSpy changed(m_model.get(),&LibraryPreferencesModel::valuesChanged);
        HANDLE lock=CreateFileW(reinterpret_cast<LPCWSTR>(m_config.utf16()),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);QVERIFY(lock!=INVALID_HANDLE_VALUE);
        const bool accepted=m_model->setValue(QStringLiteral("enqueueInSingleInstance"),false);CloseHandle(lock);
        QVERIFY(!accepted);QVERIFY(!m_model->lastError().isEmpty());QCOMPARE(m_model->values(),previous);QCOMPARE(bytes(m_config),disk);QCOMPARE(config_GetInt("one-instance-file-manager-policy"),oldPolicy);QCOMPARE(config_GetInt("playlist-open-mode"),oldMode);QCOMPARE(config_GetInt("playlist-enqueue"),oldEnqueue);QCOMPARE(changed.size(),0);
        m_model->reload();QVERIFY(m_model->lastError().isEmpty());QVERIFY(m_model->setValue(QStringLiteral("enqueueInSingleInstance"),false));QVERIFY(bytes(m_config)!=disk);QCOMPARE(config_GetInt("playlist-open-mode"),int64_t(0));QCOMPARE(config_GetInt("playlist-enqueue"),int64_t(0));QCOMPARE(changed.size(),1);
#endif
    }
    void actualInstanceDecision_data()
    {
        QTest::addColumn<bool>("master");QTest::addColumn<bool>("legacy");QTest::addColumn<bool>("file");QTest::addColumn<int>("policy");QTest::addColumn<bool>("expected");
        QTest::newRow("legacy-file-only")<<false<<true<<true<<-1<<true;QTest::newRow("legacy-master")<<true<<false<<true<<-1<<true;QTest::newRow("legacy-cli")<<false<<true<<false<<-1<<false;
        QTest::newRow("exclude-file")<<true<<true<<true<<0<<false;QTest::newRow("exclude-cli")<<true<<true<<false<<0<<true;QTest::newRow("follow-master-off")<<false<<true<<true<<1<<false;QTest::newRow("follow-master-on")<<true<<false<<true<<1<<true;QTest::newRow("unknown-preserved")<<false<<true<<true<<42<<true;
    }
    void actualInstanceDecision(){QFETCH(bool,master);QFETCH(bool,legacy);QFETCH(bool,file);QFETCH(int,policy);QFETCH(bool,expected);var_SetBool(root(),"one-instance",master);var_SetBool(root(),"one-instance-when-started-from-file",legacy);var_Create(root(),"started-from-file",VLC_VAR_BOOL);var_SetBool(root(),"started-from-file",file);var_Create(root(),"one-instance-file-manager-policy",VLC_VAR_INTEGER);var_SetInteger(root(),"one-instance-file-manager-policy",policy);QCOMPARE(vlc_test_library_single_instance(root()),expected);}
    void actualSenderMode_data(){QTest::addColumn<int>("policy");QTest::addColumn<bool>("legacy");QTest::addColumn<int>("expected");QTest::newRow("legacy-play")<<-1<<false<<0;QTest::newRow("legacy-append")<<-1<<true<<1;QTest::newRow("replace")<<0<<false<<2;QTest::newRow("append")<<1<<false<<3;}
    void actualSenderMode(){QFETCH(int,policy);QFETCH(bool,legacy);QFETCH(int,expected);var_Create(root(),"playlist-open-mode",VLC_VAR_INTEGER);var_SetInteger(root(),"playlist-open-mode",policy);var_Create(root(),"playlist-enqueue",VLC_VAR_BOOL);var_SetBool(root(),"playlist-enqueue",legacy);QCOMPARE(vlc_test_library_open_mode(root()),expected);}
    void actualReceiverQueue_data(){QTest::addColumn<bool>("replace");QTest::newRow("replace")<<true;QTest::newRow("append")<<false;}
    void actualReceiverQueue()
    {
#ifdef _WIN32
        QFETCH(bool,replace);const auto first=copy(QStringLiteral("one.wav"),QStringLiteral("old-a.wav")),second=copy(QStringLiteral("two.wav"),QStringLiteral("old-b.wav")),next=copy(QStringLiteral("one.wav"),QStringLiteral("new-a.wav")),last=copy(QStringLiteral("two.wav"),QStringLiteral("new-b.wav"));QVERIFY(!first.isEmpty()&&!second.isEmpty()&&!next.isEmpty()&&!last.isEmpty());QVERIFY(append(uri(first)));QVERIFY(append(uri(second)));auto *pl=playlist();vlc_playlist_Lock(pl);const int selected=vlc_playlist_GoTo(pl,1);vlc_playlist_Unlock(pl);QCOMPARE(selected,VLC_SUCCESS);
        const auto data=payload({uri(next).toUtf8(),QByteArray(":start-time=0"),uri(last).toUtf8()},replace?2:3);m_copy(root(),data.constData(),size_t(data.size()));
        const QStringList expected=replace?QStringList{uri(next),uri(last)}:QStringList{uri(first),uri(second),uri(next),uri(last)};QCOMPARE(queueUris(),expected);
        vlc_playlist_Lock(pl);const auto index=vlc_playlist_GetCurrentIndex(pl);vlc_playlist_Unlock(pl);QCOMPARE(index,ssize_t(replace?0:1));
        vlc_playlist_Lock(pl);auto *newItem=vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,replace?0:2));input_item_Hold(newItem);vlc_playlist_Unlock(pl);vlc_mutex_lock(&newItem->lock);const auto options=newItem->i_options;const auto option=options>0?QByteArray(newItem->ppsz_options[0]):QByteArray{};vlc_mutex_unlock(&newItem->lock);input_item_Release(newItem);QCOMPARE(options,1);QCOMPARE(option,QByteArray(":start-time=0"));
#endif
    }
    void invalidExplicitOpenDoesNotMutate_data(){QTest::addColumn<int>("kind");QTest::newRow("leading-option")<<0;QTest::newRow("trailing-bytes")<<1;QTest::newRow("embedded-null")<<2;}
    void invalidExplicitOpenDoesNotMutate()
    {
#ifdef _WIN32
        QFETCH(int,kind);const auto first=copy(QStringLiteral("one.wav"),QStringLiteral("old.wav"));QVERIFY(!first.isEmpty());QVERIFY(append(uri(first)));const auto previous=queueUris();QByteArray arg=kind==0?QByteArray(":start-time=0"):uri(first).toUtf8();if(kind==2){arg.append('\0');arg.append("extra");}auto data=payload({arg},2);if(kind==1)data.append("extra");m_copy(root(),data.constData(),size_t(data.size()));QCOMPARE(queueUris(),previous);
#endif
    }
    void hiddenNativeFolderEntries_data()
    {
        QTest::addColumn<bool>("show");QTest::addColumn<bool>("explicitOverride");QTest::addColumn<bool>("external");
        QTest::newRow("hidden-off")<<false<<false<<false;
        QTest::newRow("hidden-on")<<true<<false<<false;
        QTest::newRow("ml-style-explicit-override")<<false<<true<<false;
        QTest::newRow("external-hidden-off")<<false<<false<<true;
        QTest::newRow("external-hidden-on")<<true<<false<<true;
        QTest::newRow("external-explicit-override")<<false<<true<<true;
    }
    void hiddenNativeFolderEntries()
    {
        QFETCH(bool,show);QFETCH(bool,explicitOverride);QFETCH(bool,external);const auto normal=copy(QStringLiteral("visible.wav"),QStringLiteral("folder-Ää/visible.wav")),dot=copy(QStringLiteral(".dot.wav"),QStringLiteral("folder-Ää/.dot.wav")),hidden=copy(QStringLiteral("hidden.wav"),QStringLiteral("folder-Ää/hidden-音乐.wav"));QVERIFY(!normal.isEmpty()&&!dot.isEmpty()&&!hidden.isEmpty());
#ifdef _WIN32
        QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(hidden.utf16()),FILE_ATTRIBUTE_HIDDEN));
#endif
        QVERIFY(m_model->setValue(QStringLiteral("showHiddenFiles"),show));QStringList rows;const bool parsed=parseFolder(QFileInfo(normal).absolutePath(),explicitOverride,rows,external);
#ifdef _WIN32
        const bool restored=SetFileAttributesW(reinterpret_cast<LPCWSTR>(hidden.utf16()),FILE_ATTRIBUTE_NORMAL);QVERIFY(restored);
#endif
        QVERIFY(parsed);QVERIFY(rows.contains(uri(normal)));QCOMPARE(rows.contains(uri(dot)),show||explicitOverride);QCOMPARE(rows.contains(uri(hidden)),show||explicitOverride);QCOMPARE(rows.size(),show||explicitOverride?3:1);
    }
    void playingDirectoryUsesHiddenPreference_data()
    {
        QTest::addColumn<bool>("show");
        QTest::newRow("hidden-off") << false;
        QTest::newRow("hidden-on") << true;
    }
    void playingDirectoryUsesHiddenPreference()
    {
        QFETCH(bool, show);
        const auto normal=copy(QStringLiteral("visible.wav"),QStringLiteral("playing-folder/visible.wav"));
        const auto dot=copy(QStringLiteral(".dot.wav"),QStringLiteral("playing-folder/.dot.wav"));
        const auto hidden=copy(QStringLiteral("hidden.wav"),QStringLiteral("playing-folder/hidden.wav"));
        QVERIFY(!normal.isEmpty()&&!dot.isEmpty()&&!hidden.isEmpty());
#ifdef _WIN32
        QVERIFY(SetFileAttributesW(reinterpret_cast<LPCWSTR>(hidden.utf16()),FILE_ATTRIBUTE_HIDDEN));
#endif
        QVERIFY(m_model->setValue(QStringLiteral("showHiddenFiles"),show));
        QCOMPARE(var_InheritBool(root(),"show-hiddenfiles"),show);
        QUrl directory=QUrl::fromLocalFile(QFileInfo(normal).absolutePath());
        directory.setScheme(QStringLiteral("directory"));
        QVERIFY(append(QString::fromUtf8(directory.toEncoded())));
        auto *pl=playlist();vlc_playlist_Lock(pl);
        const int result=vlc_playlist_Start(pl);vlc_playlist_Unlock(pl);
        QCOMPARE(result,VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(queueUris().contains(uri(normal)),15000);
        const auto rows=queueUris();
        QCOMPARE(rows.contains(uri(dot)),show);
        QCOMPARE(rows.contains(uri(hidden)),show);
        QCOMPARE(rows.size(),show?3:1);
    }
    void realQueueRecursion_data(){QTest::addColumn<QByteArray>("mode");QTest::newRow("none")<<QByteArray("none");QTest::newRow("collapse")<<QByteArray("collapse");QTest::newRow("expand")<<QByteArray("expand");}
    void externalFolderSubtreeRetainsSubmittedItem()
    {
#ifdef HAVE_VLC_EXTERNAL_PREPARSER
        const auto file=copy(QStringLiteral("visible.wav"),QStringLiteral("external-folder/top.wav"));
        QVERIFY(!file.isEmpty());
        QStringList rows;
        QVERIFY(parseFolder(QFileInfo(file).absolutePath(),false,rows,true));
        QCOMPARE(rows,QStringList{uri(file)});
#endif
    }
    void externalDirectoryPolicyChangesBetweenRequests()
    {
#ifdef HAVE_VLC_EXTERNAL_PREPARSER
        const auto normal=copy(QStringLiteral("visible.wav"),QStringLiteral("reused-parser/visible.wav"));
        const auto dot=copy(QStringLiteral(".dot.wav"),QStringLiteral("reused-parser/.dot.wav"));
        QVERIFY(!normal.isEmpty()&&!dot.isEmpty());
        const vlc_preparser_cfg cfg={VLC_PREPARSER_TYPE_PARSE,1,0,VLC_TICK_FROM_SEC(10),true};
        std::unique_ptr<vlc_preparser_t,decltype(&vlc_preparser_Delete)> parser(
            vlc_preparser_New(root(),&cfg),vlc_preparser_Delete);
        QVERIFY(parser);
        for(int phase=0;phase<3;++phase) {
            const bool show=phase==0,override=phase==2;
            QVERIFY(m_model->setValue(QStringLiteral("showHiddenFiles"),show));
            const auto raw=uri(QFileInfo(normal).absolutePath()).toUtf8();
            std::unique_ptr<input_item_t,decltype(&input_item_Release)> item(
                input_item_New(raw.constData(),nullptr),input_item_Release);
            QVERIFY(item);
            if(override)QCOMPARE(input_item_AddOption(item.get(),"show-hiddenfiles",VLC_INPUT_OPTION_TRUSTED),VLC_SUCCESS);
            const int originalOptions=item->i_options;
            static const vlc_preparser_cbs callbacks={Parse::ended,Parse::subtree,nullptr};
            Parse state;
            auto *request=vlc_preparser_req_NewParse(parser.get(),item.get(),
                VLC_PREPARSER_TYPE_PARSE|VLC_PREPARSER_OPTION_SUBITEMS,&callbacks,&state);
            QVERIFY(request);
            const bool submitted=vlc_preparser_Submit(parser.get(),request)==VLC_SUCCESS;
            const bool finished=submitted&&state.done.tryAcquire(1,15000);
            if(submitted&&!finished)vlc_preparser_Cancel(parser.get(),request);
            vlc_preparser_req_Release(request);
            if(!finished)parser.reset(); // Join callbacks while their stack state remains alive.
            QVERIFY(finished);
            QCOMPARE(state.result,VLC_SUCCESS);
            QVERIFY(state.sameRoot);
            QCOMPARE(item->i_options,originalOptions);
            QVERIFY(state.urls.contains(uri(normal)));
            QCOMPARE(state.urls.contains(uri(dot)),show||override);
            QCOMPARE(state.urls.size(),show||override?2:1);
        }
#endif
    }
    void externalFolderExpansionDoesNotWaitForOptionalArtwork()
    {
#ifdef HAVE_VLC_EXTERNAL_PREPARSER
        const auto file=copy(QStringLiteral("visible.wav"),QStringLiteral("external-local-folder/top.wav"));
        QVERIFY(!file.isEmpty());
        QStringList rows;
        QVERIFY(parseFolder(QFileInfo(file).absolutePath(),false,rows,true,true));
        QCOMPARE(rows,QStringList{uri(file)});
#endif
    }
    void realQueueRecursion()
    {
        QFETCH(QByteArray,mode);const auto top=copy(QStringLiteral("visible.wav"),QStringLiteral("folder/top.wav")),child=copy(QStringLiteral("sub/child.wav"),QStringLiteral("folder/sub/child.wav"));QVERIFY(!top.isEmpty()&&!child.isEmpty());closeNative();QVERIFY(openNative(false,true,false,mode));makeModel();QVERIFY(fixtureProviders());const auto folder=QFileInfo(top).absolutePath(),sub=QFileInfo(child).absolutePath();QVERIFY(append(uri(folder)));
        if(mode=="none"){QTest::qWait(1500);QCOMPARE(queueUris(),QStringList{uri(folder)});}
        else if(mode=="collapse"){QTRY_VERIFY_WITH_TIMEOUT(queueUris().contains(uri(top)),15000);QTest::qWait(500);const auto rows=queueUris();QCOMPARE(rows.size(),2);QVERIFY(rows.contains(uri(sub)));QVERIFY(!rows.contains(uri(child)));}
        else {QTRY_VERIFY_WITH_TIMEOUT(queueUris().contains(uri(child)),15000);const auto rows=queueUris();QCOMPARE(rows.size(),2);QVERIFY(rows.contains(uri(top)));QVERIFY(!rows.contains(uri(sub)));}
    }
    void automaticQueueMetadataScope_data(){QTest::addColumn<bool>("network");QTest::newRow("local")<<false;QTest::newRow("network")<<true;}
    void metadataFetchLifecycle_data()
    {
        QTest::addColumn<QString>("operation");
        for (const char *name : {"success", "timeout", "parse-timeout", "active-cancel", "queued-cancel", "delete-active"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void metadataFetchLifecycle()
    {
        QFETCH(QString,operation);
        QVERIFY(fixtureProviders());
        if (operation != QStringLiteral("success"))
            environment("VLC_LIBRARY_FIXTURE_DELAY_MS","15000");
        else
            environment("VLC_LIBRARY_FIXTURE_DELAY_MS","250");
        FetchResult first,second; // Must outlive parser destruction, including failed assertions.
        const bool parsed=operation==QStringLiteral("parse-timeout");
        const bool timeout=operation.contains(QStringLiteral("timeout"));
        const vlc_preparser_cfg cfg={VLC_PREPARSER_TYPE_FETCHMETA_ALL|(parsed?VLC_PREPARSER_TYPE_PARSE:0),1,0,
            VLC_TICK_FROM_MS(timeout?250:10000),false};
        std::unique_ptr<vlc_preparser_t,decltype(&vlc_preparser_Delete)> parser(
            vlc_preparser_New(root(),&cfg),vlc_preparser_Delete);
        QVERIFY(parser);
        const auto raw=uri(QDir(m_media).filePath(QStringLiteral("one.wav"))).toUtf8();
        std::unique_ptr<input_item_t,decltype(&input_item_Release)> item(
            input_item_New(raw.constData(),nullptr),input_item_Release);
        QVERIFY(item);input_item_SetTitle(item.get(),"Owned base title");
        QVERIFY(!input_item_IsPreparsed(item.get()));
        static const vlc_preparser_cbs callbacks={FetchResult::ended,nullptr,nullptr};
        std::unique_ptr<vlc_preparser_req,decltype(&vlc_preparser_req_Release)> request(
            vlc_preparser_req_NewParse(parser.get(),item.get(),VLC_PREPARSER_TYPE_FETCHMETA_NET|(parsed?VLC_PREPARSER_TYPE_PARSE:0),&callbacks,&first),vlc_preparser_req_Release);
        QVERIFY(request);
        QElapsedTimer elapsed;elapsed.start();
        QCOMPARE(vlc_preparser_Submit(parser.get(),request.get()),VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,2000);
        if (operation == QStringLiteral("success"))
            input_item_SetDuration(item.get(),VLC_TICK_FROM_SEC(42));
        if(operation==QStringLiteral("queued-cancel")) {
            std::unique_ptr<vlc_preparser_req,decltype(&vlc_preparser_req_Release)> queued(
                vlc_preparser_req_NewParse(parser.get(),item.get(),VLC_PREPARSER_TYPE_FETCHMETA_NET,&callbacks,&second),vlc_preparser_req_Release);
            QVERIFY(queued);QCOMPARE(vlc_preparser_Submit(parser.get(),queued.get()),VLC_SUCCESS);
            QCOMPARE(vlc_preparser_Cancel(parser.get(),queued.get()),size_t(1));
            QVERIFY(second.done.tryAcquire(1,500));QCOMPARE(second.result,-EINTR);QCOMPARE(second.calls,1);
            QCOMPARE(providerCalls(true),1);QVERIFY(!first.done.tryAcquire(1));
        }
        if(operation==QStringLiteral("delete-active"))
            parser.reset();
        else if(operation==QStringLiteral("active-cancel")||operation==QStringLiteral("queued-cancel"))
            QCOMPARE(vlc_preparser_Cancel(parser.get(),request.get()),size_t(1));
        QVERIFY(first.done.tryAcquire(1,2000));QCOMPARE(first.calls,1);
        QCOMPARE(first.result,operation==QStringLiteral("success")?VLC_SUCCESS:
            timeout?VLC_ETIMEOUT:-EINTR);
        QVERIFY2(elapsed.elapsed()<2500,"Cooperative provider must terminate before its 15-second delay");
        char *title=input_item_GetTitle(item.get());
        const auto result=QString::fromUtf8(title?title:"");free(title);
        QCOMPARE(result,operation==QStringLiteral("success")?QStringLiteral("Owned network metadata fixture"):
            parsed?QStringLiteral("Owned embedded title"):QStringLiteral("Owned base title"));
        QCOMPARE(input_item_IsArtFetched(item.get()),operation==QStringLiteral("success"));
        // Fetching titles/art alone must not make the queue skip file parsing.
        if (!parsed)
            QVERIFY(!input_item_IsPreparsed(item.get()));
        if (operation == QStringLiteral("success"))
            QCOMPARE(input_item_GetDuration(item.get()),VLC_TICK_FROM_SEC(42));
        if(operation==QStringLiteral("success")) {
            char *art=input_item_GetArtURL(item.get());const auto artwork=QString::fromUtf8(art?art:"");free(art);
            QCOMPARE(artwork,uri(QDir(m_media).filePath(QStringLiteral("art.png"))));
        }
        if(parsed) {
            QCOMPARE(input_item_GetDuration(item.get()),VLC_TICK_FROM_SEC(60));
            vlc_mutex_lock(&item->lock);const auto tracks=item->es_vec.size;vlc_mutex_unlock(&item->lock);
            QCOMPARE(tracks,size_t(1));
        }
        parser.reset();QCOMPARE(first.calls,1);
    }
    void automaticQueueMetadataScope()
    {
        QFETCH(bool,network);const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("metadata/queue.wav"));QVERIFY(!file.isEmpty());closeNative();QVERIFY(openNative(false,true,network));makeModel();QVERIFY2(fixtureProviders(),"Owned local-only providers must lead both candidate lists before any request");QVERIFY(append(uri(file)));auto *pl=playlist();
        auto title=[pl](){vlc_playlist_Lock(pl);auto *media=vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,0));char *raw=input_item_GetTitle(media);const auto t=QString::fromUtf8(raw?raw:"");free(raw);vlc_playlist_Unlock(pl);return t;};
        if(network){QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,15000);QTRY_COMPARE_WITH_TIMEOUT(title(),QStringLiteral("Owned network metadata fixture"),5000);
            vlc_playlist_Lock(pl);auto *media=vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,0));
            const bool fetched=input_item_IsArtFetched(media);char *raw=input_item_GetArtURL(media);const auto art=QString::fromUtf8(raw?raw:"");free(raw);vlc_playlist_Unlock(pl);
            QVERIFY(fetched);QCOMPARE(art,uri(QDir(m_media).filePath(QStringLiteral("art.png"))));}
        else{QTRY_VERIFY_WITH_TIMEOUT(providerCalls(false)>0,15000);QCOMPARE(providerCalls(true),0);QCOMPARE(title(),QStringLiteral("Owned embedded title"));}
    }
    void automaticLibraryMetadataScope_data(){automaticQueueMetadataScope_data();}
    void automaticLibraryMetadataScope()
    {
        QFETCH(bool,network);const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("ml-root/owned.wav"));QVERIFY(!file.isEmpty());closeNative();QVERIFY(openNative(true,false,network));makeModel();QVERIFY(library());QVERIFY2(fixtureProviders(),"Owned local-only providers must lead both candidate lists before discovery");QCOMPARE(vlc_ml_count_media(library(),nullptr),size_t(0));const auto folder=uri(QFileInfo(file).absolutePath()).toUtf8(),mediaUri=uri(file).toUtf8();QCOMPARE(vlc_ml_add_folder(library(),folder.constData()),VLC_SUCCESS);
        auto title=[this,&mediaUri](){auto *media=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());if(!media)return QString{};const auto t=QString::fromUtf8(media->psz_title?media->psz_title:"");vlc_ml_media_release(media);return t;};
        const auto expected=network?QStringLiteral("Owned network metadata fixture"):QStringLiteral("Owned embedded title");QTRY_COMPARE_WITH_TIMEOUT(title(),expected,30000);if(network)QVERIFY(providerCalls(true)>0);else QCOMPARE(providerCalls(true),0);
        if(network) {
            auto *media=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());QVERIFY(media);
            const auto subtype=media->i_subtype;const auto albumId=media->album_track.i_album_id;vlc_ml_media_release(media);
            QCOMPARE(subtype,VLC_ML_MEDIA_SUBTYPE_ALBUMTRACK);QVERIFY(albumId>0);
            auto *album=vlc_ml_get_album(library(),albumId);QVERIFY(album);
            const auto art=QString::fromUtf8(album->thumbnails[VLC_ML_THUMBNAIL_SMALL].psz_mrl?album->thumbnails[VLC_ML_THUMBNAIL_SMALL].psz_mrl:"");
            const auto status=album->thumbnails[VLC_ML_THUMBNAIL_SMALL].i_status;vlc_ml_album_release(album);
            QCOMPARE(status,VLC_ML_THUMBNAIL_STATUS_AVAILABLE);
            // The library owns a relocated copy; check its bytes rather than
            // requiring the original cache URI to survive that transfer.
            QFile saved(QUrl(art).toLocalFile()),source(QDir(m_media).filePath(QStringLiteral("art.png")));
            QVERIFY(saved.open(QIODevice::ReadOnly));QVERIFY(source.open(QIODevice::ReadOnly));
            QCOMPARE(saved.readAll(),source.readAll());
        }
    }
    void libraryEnrichmentTimeoutKeepsEmbeddedMetadataAndTracks()
    {
        const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("timeout-root/owned.wav"));QVERIFY(!file.isEmpty());
        closeNative();environment("VLC_LIBRARY_FIXTURE_DELAY_MS","15000");
        QVERIFY(openNative(true,false,true));makeModel();QVERIFY(library());QVERIFY(fixtureProviders());
        const auto folder=uri(QFileInfo(file).absolutePath()).toUtf8(),mediaUri=uri(file).toUtf8();
        QCOMPARE(vlc_ml_add_folder(library(),folder.constData()),VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,5000);
        const auto titleReady=[this,&mediaUri](){auto *m=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());if(!m)return QString{};const auto title=QString::fromUtf8(m->psz_title);vlc_ml_media_release(m);return title;};
        QTRY_COMPARE_WITH_TIMEOUT(titleReady(),QStringLiteral("Owned embedded title"),12000);
        QCOMPARE(vlc_ml_count_media(library(),nullptr),size_t(1));
        auto *media=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());QVERIFY(media);
        const auto title=QString::fromUtf8(media->psz_title);const auto duration=media->i_duration;
        const auto tracks=media->p_tracks?media->p_tracks->i_nb_items:0;
        vlc_ml_media_release(media);
        QCOMPARE(title,QStringLiteral("Owned embedded title"));QCOMPARE(duration,int64_t(60000));QCOMPARE(tracks,size_t(1));
    }
    void libraryShutdownDuringEnrichmentCanResume()
    {
        const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("shutdown-root/owned.wav"));QVERIFY(!file.isEmpty());
        closeNative();environment("VLC_LIBRARY_FIXTURE_DELAY_MS","15000");
        QVERIFY(openNative(true,false,true));makeModel();QVERIFY(library());QVERIFY(fixtureProviders());
        const auto folder=uri(QFileInfo(file).absolutePath()).toUtf8(),mediaUri=uri(file).toUtf8();
        QCOMPARE(vlc_ml_add_folder(library(),folder.constData()),VLC_SUCCESS);
        QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,5000);
        QElapsedTimer timer;timer.start();closeNative();QVERIFY(timer.elapsed()<2500);
        unsetEnvironment("VLC_LIBRARY_FIXTURE_DELAY_MS");
        QVERIFY(openNative(true,false,false));makeModel();QVERIFY(library());QVERIFY(fixtureProviders());
        const auto titleReady=[this,&mediaUri](){auto *m=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());if(!m)return QString{};const auto title=QString::fromUtf8(m->psz_title);vlc_ml_media_release(m);return title;};
        QTRY_COMPARE_WITH_TIMEOUT(titleReady(),QStringLiteral("Owned embedded title"),30000);
        QCOMPARE(vlc_ml_count_media(library(),nullptr),size_t(1));
        auto *media=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());QVERIFY(media);
        const auto title=QString::fromUtf8(media->psz_title);const auto tracks=media->p_tracks?media->p_tracks->i_nb_items:0;
        vlc_ml_media_release(media);
        QCOMPARE(title,QStringLiteral("Owned embedded title"));QCOMPARE(tracks,size_t(1));
    }
    void externalMetadataProviderBridge_data(){automaticQueueMetadataScope_data();}
    void externalMetadataProviderBridge()
    {
        QFETCH(bool,network);QVERIFY(fixtureProviders());
        FetchResult state;
        const int options=VLC_PREPARSER_TYPE_PARSE|VLC_PREPARSER_TYPE_FETCHMETA_LOCAL|(network?VLC_PREPARSER_TYPE_FETCHMETA_NET:0);
        const vlc_preparser_cfg cfg={VLC_PREPARSER_TYPE_PARSE|VLC_PREPARSER_TYPE_FETCHMETA_ALL,1,0,VLC_TICK_FROM_SEC(5),true};
        std::unique_ptr<vlc_preparser_t,decltype(&vlc_preparser_Delete)> parser(vlc_preparser_New(root(),&cfg),vlc_preparser_Delete);
        QVERIFY(parser);
        const auto url=uri(QDir(m_media).filePath(QStringLiteral("one.wav"))).toUtf8();
        std::unique_ptr<input_item_t,decltype(&input_item_Release)> item(input_item_New(url.constData(),nullptr),input_item_Release);QVERIFY(item);
        static const vlc_preparser_cbs callbacks={FetchResult::ended,nullptr,nullptr};
        std::unique_ptr<vlc_preparser_req,decltype(&vlc_preparser_req_Release)> request(vlc_preparser_req_NewParse(parser.get(),item.get(),options,&callbacks,&state),vlc_preparser_req_Release);QVERIFY(request);
        QCOMPARE(vlc_preparser_Submit(parser.get(),request.get()),VLC_SUCCESS);QVERIFY(state.done.tryAcquire(1,10000));QCOMPARE(state.result,VLC_SUCCESS);QCOMPARE(state.calls,1);
        const auto trace=bytes(qEnvironmentVariable("VLC_LIBRARY_FIXTURE_LOG")+(network?QStringLiteral("-network.log"):QStringLiteral("-local.log"))).trimmed();
        bool numeric=false;const auto providerPid=trace.split('\n').first().toLongLong(&numeric);
        QVERIFY(numeric&&providerPid>0);QVERIFY(providerPid!=QCoreApplication::applicationPid());
        QCOMPARE(providerCalls(true)>0,network);
    }
    void queueEnrichmentTimeoutKeepsParsedDetails()
    {
        const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("queue-timeout/owned.wav"));QVERIFY(!file.isEmpty());
        closeNative();environment("VLC_LIBRARY_FIXTURE_DELAY_MS","15000");
        QVERIFY(openNative(false,true,true,{},250));makeModel();QVERIFY(fixtureProviders());QVERIFY(append(uri(file)));
        QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,5000);
        QTRY_VERIFY_WITH_TIMEOUT(providerInterrupted(),2000);
        QCOMPARE(queuedTitle(),QStringLiteral("Owned embedded title"));QCOMPARE(queueUris(),QStringList{uri(file)});
        auto *pl=playlist();vlc_playlist_Lock(pl);auto *media=vlc_playlist_item_GetMedia(vlc_playlist_Get(pl,0));
        const auto duration=input_item_GetDuration(media);vlc_mutex_lock(&media->lock);const auto tracks=media->es_vec.size;vlc_mutex_unlock(&media->lock);vlc_playlist_Unlock(pl);
        QCOMPARE(duration,VLC_TICK_FROM_SEC(60));QCOMPARE(tracks,size_t(1));
    }
    void removingQueueItemCancelsLookupAndAllowsNextItem()
    {
        const auto first=copy(QStringLiteral("one.wav"),QStringLiteral("queue-cancel/first.wav")),second=copy(QStringLiteral("two.wav"),QStringLiteral("queue-cancel/second.wav"));QVERIFY(!first.isEmpty()&&!second.isEmpty());
        closeNative();environment("VLC_LIBRARY_FIXTURE_DELAY_MS","15000");
        QVERIFY(openNative(false,true,true,{},10000));makeModel();QVERIFY(fixtureProviders());QVERIFY(append(uri(first)));
        QTRY_VERIFY_WITH_TIMEOUT(providerCalls(true)>0,5000);
        auto *pl=playlist();vlc_playlist_Lock(pl);vlc_playlist_Remove(pl,0,1);vlc_playlist_Unlock(pl);
        QTRY_VERIFY_WITH_TIMEOUT(providerInterrupted(),2000);QVERIFY(queueUris().isEmpty());
        unsetEnvironment("VLC_LIBRARY_FIXTURE_DELAY_MS");QVERIFY(append(uri(second)));
        QTRY_COMPARE_WITH_TIMEOUT(queuedTitle(),QStringLiteral("Owned network metadata fixture"),5000);
        QCOMPARE(queueUris(),QStringList{uri(second)});
    }
    void disablingLibraryPreservesRealRootsPlaylistFavoriteHistoryAndHome()
    {
        const auto file=copy(QStringLiteral("one.wav"),QStringLiteral("preserved-root/owned.wav"));QVERIFY(!file.isEmpty());closeNative();QVERIFY(openNative(true));makeModel();QVERIFY(m_model->setValue(QStringLiteral("useMediaLibrary"),true));closeNative();QVERIFY(openNative(true));makeModel();QVERIFY(library());QVERIFY(!m_model->restartRequired());QVERIFY(fixtureProviders());
        const auto folder=uri(QFileInfo(file).absolutePath()).toUtf8(),mediaUri=uri(file).toUtf8();QCOMPARE(vlc_ml_add_folder(library(),folder.constData()),VLC_SUCCESS);QTRY_COMPARE_WITH_TIMEOUT(vlc_ml_count_media(library(),nullptr),size_t(1),30000);
        auto *media=vlc_ml_get_media_by_mrl(library(),mediaUri.constData());QVERIFY(media);const auto id=media->i_id;vlc_ml_media_release(media);QCOMPARE(vlc_ml_media_set_played(library(),id,true),VLC_SUCCESS);QCOMPARE(vlc_ml_media_update_progress(library(),id,.375),VLC_SUCCESS);QCOMPARE(vlc_ml_media_set_favorite(library(),id,true),VLC_SUCCESS);
        auto *named=vlc_ml_playlist_create(library(),"Owned retained playlist");QVERIFY(named);const auto playlistId=named->i_id;vlc_ml_playlist_release(named);QCOMPARE(vlc_ml_playlist_append(library(),playlistId,&id,1),VLC_SUCCESS);
        const auto ini=QDir(m_case).filePath(QStringLiteral("owned-home.ini"));QSettings settings(ini,QSettings::IniFormat);MLHomeVisitStore home(&settings);QVERIFY(home.record(MLItemId(playlistId,VLC_ML_PARENT_PLAYLIST),123456));const auto homeBytes=bytes(ini);
        QVERIFY(m_model->setValue(QStringLiteral("useMediaLibrary"),false));QVERIFY(m_model->libraryAvailable());QVERIFY(m_model->restartRequired());closeNative();
        QMap<QString,QByteArray> files;QDirIterator iter(QDir(m_userdata).filePath(QStringLiteral("ml")),QDir::Files,QDirIterator::Subdirectories);while(iter.hasNext()){const auto path=iter.next();files.insert(path,bytes(path));}QVERIFY(!files.isEmpty());
        QVERIFY(openNative(false));makeModel();QVERIFY(!library());QVERIFY(!m_model->libraryAvailable());for(auto i=files.cbegin();i!=files.cend();++i)QCOMPARE(bytes(i.key()),i.value());QCOMPARE(bytes(ini),homeBytes);
        QVERIFY(m_model->setValue(QStringLiteral("useMediaLibrary"),true));QVERIFY(!m_model->libraryAvailable());closeNative();QVERIFY(openNative(true));makeModel();QVERIFY(library());
        media=vlc_ml_get_media(library(),id);QVERIFY(media);QCOMPARE(media->i_playcount,uint32_t(1));QCOMPARE(media->f_progress,.375);QVERIFY(media->b_is_favorite);vlc_ml_media_release(media);QCOMPARE(vlc_ml_count_playlist_media(library(),nullptr,playlistId),size_t(1));
        auto *roots=vlc_ml_list_entry_points(library(),nullptr);QVERIFY(roots);QCOMPARE(roots->i_nb_items,size_t(1));vlc_ml_folder_list_release(roots);MLHomeVisitStore reloaded(&settings);QCOMPARE(reloaded.visits().size(),1);QCOMPARE(reloaded.visits().first().count,quint64(1));QCOMPARE(bytes(ini),homeBytes);
    }
};
QTEST_GUILESS_MAIN(TestLibraryPreferences)
#include "test_library_preferences.moc"
