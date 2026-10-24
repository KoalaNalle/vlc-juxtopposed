// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "player/timeline_thumbnails.hpp"
#include <vlc/vlc.h>
#include <vlc_variables.h>
#include <vlc_fs.h>
#include <vlc_network.h>
#include <vlc_player.h>
#include <QElapsedTimer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickImageProvider>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <limits>
#include <cmath>
#include <algorithm>

extern "C" vlc_object_t *vlc_test_media_player_object(libvlc_media_player_t *);
extern "C" vlc_player_t *vlc_test_media_player_core(libvlc_media_player_t *);

class TestTimelineThumbnails : public QObject
{
    Q_OBJECT
    libvlc_instance_t *m_instance = nullptr;
    libvlc_media_player_t *m_player = nullptr;
    QTemporaryDir m_directory;
    SharedInputItem m_input;
    std::unique_ptr<TimelineThumbnails> m_preview;
    std::unique_ptr<QQuickImageProvider> m_provider;
    QUrl m_url;
    QImage frame()
    {
        QSize size;
        return m_provider->requestImage(QUrl(m_preview->imageSource()).path().mid(1), &size, {});
    }
private slots:
    void initTestCase()
    {
        const auto fixture = qEnvironmentVariable("VLC_THUMBNAIL_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set VLC_THUMBNAIL_FIXTURE to the generated red/green/blue 18s MP4");
        QVERIFY(m_directory.isValid());
        const auto path = m_directory.filePath(QString::fromUtf8("source ü '[]#.mp4"));
        QVERIFY(QFile::copy(fixture, path));
        m_url = QUrl::fromLocalFile(path);
        const char *args[] = {"--ignore-config", "--no-media-library", "--no-hw-dec",
            "--aout=adummy", "--vout=vdummy", "--verbose=2", "--no-metadata-network-access",
            "--file-logging", "--logfile=timeline-thumbnails-native.log"};
        m_instance = libvlc_new(ARRAY_SIZE(args), args);
        QVERIFY(m_instance);
        m_player = libvlc_media_player_new(m_instance, nullptr, nullptr);
        QVERIFY(m_player);
        auto *object = vlc_test_media_player_object(m_player);
        QCOMPARE(var_Create(object,"window",VLC_VAR_STRING),VLC_SUCCESS);
        QCOMPARE(var_SetString(object,"window","wdummy,none"),VLC_SUCCESS);
        m_input = SharedInputItem(input_item_New(m_url.toEncoded().constData(), "Owned preview fixture"), false);
        QVERIFY(m_input);
        m_preview = std::make_unique<TimelineThumbnails>(vlc_test_media_player_object(m_player));
        m_provider.reset(m_preview->createImageProvider());
        m_preview->setInput(m_input.get());
    }
    void nativeFramesAtRequestedTimes()
    {
        QVERIFY(!m_preview->request(2)); // Disabled does no decode.
        m_preview->setEnabled(true);
        const auto originalDuration = input_item_GetDuration(m_input.get());
        QVERIFY(m_preview->request(2.75));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
        QVERIFY2(!m_preview->imageSource().isEmpty(), qPrintable(m_preview->error()));
        QCOMPARE(m_preview->requestedSeconds(), 2.0);
        auto image = frame();
        QCOMPARE(image.size(), QSize(252,142));
        auto color = image.pixelColor(126,71);
        QCOMPARE(color.alpha(),255);
        QVERIFY2(color.red()>200 && color.green()<40 && color.blue()<40, qPrintable(color.name()));
        QVERIFY(m_preview->request(8));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
        color = frame().pixelColor(126,71);
        QVERIFY2(color.green()>200 && color.red()<40 && color.blue()<40, qPrintable(color.name()));
        QVERIFY(m_preview->request(14));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
        color = frame().pixelColor(126,71);
        QVERIFY2(color.blue()>200 && color.red()<40 && color.green()<40, qPrintable(color.name()));
        QCOMPARE(input_item_GetDuration(m_input.get()), originalDuration); // Native clone owns its metadata.
    }
    void cacheAndSameSecond()
    {
        QVERIFY(m_preview->request(2.1));
        QVERIFY(!m_preview->loading());
        const auto source = m_preview->imageSource();
        QVERIFY(m_preview->request(2.9));
        QCOMPARE(m_preview->imageSource(), source);
        m_preview->cancel();
        QVERIFY(m_preview->imageSource().isEmpty());
        QVERIFY(m_preview->request(2.2));
        QVERIFY(!m_preview->loading());
        QVERIFY(!frame().isNull());
    }
    void latestRequestAndMediaGeneration()
    {
        m_preview->setInput(m_input.get());
        for (int i=0;i<50;++i) QVERIFY(m_preview->request(i%16));
        QVERIFY(m_preview->request(14));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
        QCOMPARE(m_preview->requestedSeconds(),14.0);
        const auto color=frame().pixelColor(126,71);
        QVERIFY(color.blue()>200 && color.red()<40);
        m_preview->setInput(m_input.get()); // Same URI is a new generation, not a cache hit.
        QVERIFY(m_preview->imageSource().isEmpty());
        QVERIFY(m_preview->request(2));
        QVERIFY(m_preview->loading());
        QTest::qWait(140); // Allow native work to start, then replace/disable it.
        m_preview->setInput(nullptr);
        m_preview->setEnabled(false);
        QTest::qWait(250);
        QVERIFY(m_preview->imageSource().isEmpty());
        QVERIFY(!m_preview->loading());
        m_preview->setInput(m_input.get());
        m_preview->setEnabled(true);
    }
    void viewsCannotCancelEachOthersPreview()
    {
        const auto a=m_preview->registerClient(),b=m_preview->registerClient();
        QVERIFY(a!=b);
        QVERIFY(m_preview->requestFor(a,2));
        QVERIFY(m_preview->requestFor(b,14));
        m_preview->cancelFor(a);
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QVERIFY(!m_preview->imageSource().isEmpty());
        QCOMPARE(m_preview->requestedSeconds(),14.0);
        const auto source=m_preview->imageSource();
        QVERIFY(!m_preview->requestFor({},2));
        QCOMPARE(m_preview->imageSource(),source);
        m_preview->cancelFor(b);
        QVERIFY(m_preview->imageSource().isEmpty());
        QVERIFY(m_preview->requestFor(a,3));QTest::qWait(140);
        QVERIFY(m_preview->requestFor(b,9));m_preview->cancelFor(a);
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QCOMPARE(m_preview->requestedSeconds(),9.0);
        const auto color=frame().pixelColor(126,71);
        QVERIFY(color.green()>200 && color.red()<40 && color.blue()<40);
        m_preview->cancelFor(b);
    }
    void continuousMovementProducesFrames()
    {
        m_preview->setInput(m_input.get());
        m_preview->setEnabled(true);
        int framesWhileMoving = 0;
        QString last;
        bool moving = true;
        const auto connection = connect(m_preview.get(), &TimelineThumbnails::stateChanged, this, [&] {
            const auto image = m_preview->imageSource();
            if (moving && !image.isEmpty() && image != last) {
                last = image;
                ++framesWhileMoving;
            }
        });
        // Every event requests a different second, as on a long movie's bar.
        // An end-of-burst-only assertion would miss starvation during motion.
        for (int i = 0; i < 100; ++i) {
            QVERIFY(m_preview->request((i * 7) % 17));
            QTest::qWait(20);
        }
        moving = false;
        disconnect(connection);
        QVERIFY(m_preview->request(14));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
        QVERIFY2(!m_preview->imageSource().isEmpty(), qPrintable(m_preview->error()));
        QCOMPARE(m_preview->requestedSeconds(), 14.0);
        const auto color = frame().pixelColor(126,71);
        QVERIFY(color.blue()>200 && color.red()<40 && color.green()<40);
        qInfo("Frames published during 100 moving requests: %d", framesWhileMoving);
        QVERIFY2(framesWhileMoving >= 2, "Previews must appear during movement, before the pointer stops");
    }
    void invalidTimeAndUnavailableMedia()
    {
        QVERIFY(!m_preview->request(-1));
        QVERIFY(!m_preview->request(std::numeric_limits<double>::quiet_NaN()));
        QVERIFY(!m_preview->request(std::numeric_limits<double>::infinity()));
        QVERIFY(!m_preview->request(std::numeric_limits<double>::max()));
        SharedInputItem missing(input_item_New(QUrl::fromLocalFile(m_directory.filePath("missing.mp4")).toEncoded().constData(),"Owned missing file"),false);
        m_preview->setInput(missing.get());
        QVERIFY(m_preview->request(2));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QVERIFY(m_preview->imageSource().isEmpty());
        QVERIFY(!m_preview->error().isEmpty());
        m_preview->setInput(m_input.get());
    }
    void independentPrimaryPlayback()
    {
        auto *media=libvlc_media_new_location(m_url.toEncoded().constData());
        QVERIFY(media);
        libvlc_media_player_set_media(m_player,media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player),0);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player),libvlc_Playing,5000);
        QTRY_VERIFY_WITH_TIMEOUT(libvlc_media_player_get_time(m_player)>100000,5000);
        const auto before=libvlc_media_player_get_time(m_player);
        QElapsedTimer wall;wall.start();
        m_preview->setInput(m_input.get());
        QVERIFY(m_preview->request(14));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QVERIFY2(!m_preview->imageSource().isEmpty(),qPrintable(m_preview->error()));
        QCOMPARE(libvlc_media_player_get_state(m_player),libvlc_Playing);
        const auto after=libvlc_media_player_get_time(m_player);
        qInfo("Primary before=%lldus after=%lldus wall=%lldms",(long long)before,(long long)after,(long long)wall.elapsed());
        QVERIFY(after>=before);
        QVERIFY(after-before<=VLC_TICK_FROM_MS(wall.elapsed()+500));
        QVERIFY(after<10000000); // Preview did not seek main to14s. libvlc4 uses microseconds.
        libvlc_media_player_set_pause(m_player,1);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player),libvlc_Paused,5000);
        QTest::qWait(100);
        const auto paused=libvlc_media_player_get_time(m_player);
        QTest::qWait(100);
        QCOMPARE(libvlc_media_player_get_time(m_player),paused); // Stable no-write control first.
        QVERIFY(m_preview->request(8));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QVERIFY2(!m_preview->imageSource().isEmpty(),qPrintable(m_preview->error()));
        QCOMPARE(libvlc_media_player_get_state(m_player),libvlc_Paused);
        QCOMPARE(libvlc_media_player_get_time(m_player),paused);
        libvlc_media_player_stop_async(m_player);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player),libvlc_Stopped,5000);
    }
    void workerShutdown()
    {
        auto pending=std::make_unique<TimelineThumbnails>(vlc_test_media_player_object(m_player));
        pending->setInput(m_input.get());pending->setEnabled(true);QVERIFY(pending->request(14));
        QTest::qWait(140);
        QElapsedTimer timer;timer.start();pending.reset();
        QVERIFY(timer.elapsed()<5000);
        QCoreApplication::processEvents();
    }
    void stalledNetworkCancellationAndShutdown()
    {
        struct Socket {
            int fd = -1;
            ~Socket() { if (fd >= 0) net_Close(fd); }
        } listener, socket, second;
        listener.fd = vlc_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, true);
        QVERIFY(listener.fd >= 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        QCOMPARE(bind(listener.fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
        QCOMPARE(listen(listener.fd, 2), 0);
        socklen_t length = sizeof(address);
        QCOMPARE(getsockname(listener.fd, reinterpret_cast<sockaddr *>(&address), &length), 0);
        const auto uri = QStringLiteral("http://127.0.0.1:%1/owned-stalled.mp4").arg(ntohs(address.sin_port)).toUtf8();
        const auto accept = [&](Socket &connection) {
            if (connection.fd < 0) connection.fd = vlc_accept(listener.fd, nullptr, nullptr, true);
            return connection.fd >= 0;
        };
        QByteArray request;
        const auto receivedRequest = [&](const Socket &connection) {
            if (request.contains("\r\n\r\n")) return true;
            char data[1024]; const auto count = recv(connection.fd, data, sizeof(data), 0);
            if (count > 0) request.append(data, count);
            return request.contains("\r\n\r\n");
        };
        SharedInputItem network(input_item_New(uri.constData(), "Owned stalled preview"), false);
        auto pending = std::make_unique<TimelineThumbnails>(vlc_test_media_player_object(m_player));
        std::unique_ptr<QQuickImageProvider> provider(pending->createImageProvider());
        pending->setEnabled(true);
        pending->setInput(network.get());
        QVERIFY(pending->request(2));
        QTRY_VERIFY_WITH_TIMEOUT(accept(socket), 2000);
        QTRY_VERIFY_WITH_TIMEOUT(receivedRequest(socket), 1000);
        QVERIFY(request.startsWith("GET /owned-stalled.mp4"));
        QVERIFY(pending->loading()); // Native HTTP request reached us; deliberately send no response.
        QElapsedTimer timer; timer.start();
        pending->setInput(m_input.get());
        QVERIFY(pending->request(14));
        QTRY_VERIFY_WITH_TIMEOUT(!pending->loading(), 3000);
        QVERIFY2(!pending->imageSource().isEmpty(), qPrintable(pending->error()));
        QVERIFY(timer.elapsed() < 3000);
        QSize size;
        const auto image = provider->requestImage(QUrl(pending->imageSource()).path().mid(1), &size, {});
        QVERIFY(!image.isNull());
        const auto color = image.pixelColor(126,71);
        QVERIFY(color.blue()>200 && color.red()<40 && color.green()<40);

        pending->setInput(network.get());
        QVERIFY(pending->request(2));
        QTRY_VERIFY_WITH_TIMEOUT(accept(second), 2000);
        request.clear();
        QTRY_VERIFY_WITH_TIMEOUT(receivedRequest(second), 1000);
        QVERIFY(request.startsWith("GET /owned-stalled.mp4"));
        QVERIFY(pending->loading());
        timer.restart(); pending.reset();
        QVERIFY(timer.elapsed() < 3000);
        // Provider keeps only image data and can be released after its model.
        QVERIFY(provider->requestImage(QStringLiteral("missing"), &size, {}).isNull());
        QCoreApplication::processEvents();
    }
    void geometryMatchesIndependentReference_data()
    {
        QTest::addColumn<QString>("name");
        for (const auto *name : {"landscape", "portrait", "sar-2-to-1", "rotate-90", "rotate-180", "rotate-270"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void geometryMatchesIndependentReference()
    {
        const auto directory=qEnvironmentVariable("VLC_THUMBNAIL_GEOMETRY_DIR");
        if(directory.isEmpty())QSKIP("Set VLC_THUMBNAIL_GEOMETRY_DIR to generated geometry fixtures");
        QFETCH(QString,name);
        const QDir fixtures(directory);
        const auto uri=QUrl::fromLocalFile(fixtures.filePath(name+QStringLiteral(".mp4"))).toEncoded();
        SharedInputItem input(input_item_New(uri.constData(),"Owned geometry preview"),false);
        QVERIFY(input);
        m_preview->setInput(input.get());m_preview->setEnabled(true);
        QVERIFY(m_preview->request(1));
        QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(),6000);
        QVERIFY2(!m_preview->imageSource().isEmpty(),qPrintable(m_preview->error()));
        const auto actual=frame();
        const QImage reference(fixtures.filePath(name+QStringLiteral("-reference.png")));
        QCOMPARE(actual.size(),QSize(252,142));QCOMPARE(reference.size(),actual.size());
        QJsonArray samples;bool quadrants=true;
        for(const auto &point:{QPoint(63,35),QPoint(189,35),QPoint(63,106),QPoint(189,106)}) {
            const auto a=actual.pixelColor(point),b=reference.pixelColor(point);
            samples.append(QJsonObject{{"x",point.x()},{"y",point.y()},{"actual",a.name()},{"reference",b.name()}});
            quadrants=quadrants&&std::abs(a.red()-b.red())<35&&std::abs(a.green()-b.green())<35&&std::abs(a.blue()-b.blue())<35;
        }
        const auto marker=[](const QImage &image) {
            QRect bounds;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
                const auto color=image.pixelColor(x,y);
                // Chroma interpolation at a saturated corner boundary can be
                // pale yellow. The synthetic marker is neutral white.
                const int low=std::min({color.red(),color.green(),color.blue()});
                const int high=std::max({color.red(),color.green(),color.blue()});
                if(low>235&&high-low<12)bounds=bounds.united(QRect(x,y,1,1));
            }
            return bounds;
        };
        const auto a=marker(actual),b=marker(reference);
        const auto output=qEnvironmentVariable("VLC_THUMBNAIL_GEOMETRY_OUTPUT");
        if(!output.isEmpty()) {
            const QDir folder(output);QVERIFY(folder.mkpath(QStringLiteral(".")));
            QVERIFY(actual.save(folder.filePath(name+QStringLiteral(".png"))));
            QFile report(folder.filePath(name+QStringLiteral(".json")));QVERIFY(report.open(QIODevice::WriteOnly));
            report.write(QJsonDocument(QJsonObject{{"name",name},{"samples",samples},
                {"actualMarker",QJsonArray{a.x(),a.y(),a.width(),a.height()}},
                {"referenceMarker",QJsonArray{b.x(),b.y(),b.width(),b.height()}}}).toJson());
        }
        QVERIFY2(quadrants,qPrintable(name+QStringLiteral(" corner colors match independent oriented reference")));
        QVERIFY2(!a.isEmpty()&&!b.isEmpty(),qPrintable(name+QStringLiteral(" white circle exists")));
        QVERIFY2(std::abs(double(a.width())/a.height()-1.)<.12,qPrintable(name+QStringLiteral(" circle remains round")));
        QVERIFY2(std::abs(a.width()-b.width())<=4&&std::abs(a.height()-b.height())<=4
            &&std::abs(a.center().x()-b.center().x())<=3&&std::abs(a.center().y()-b.center().y())<=3,
            qPrintable(name+QStringLiteral(" centered crop matches independent reference")));
        m_preview->setInput(m_input.get());
    }
    void previewIgnoresPlaybackStartupPause_data()
    {
        QTest::addColumn<bool>("startupPaused");
        QTest::newRow("startup-playing") << false;
        QTest::newRow("startup-paused") << true;
    }
    void previewIgnoresPlaybackStartupPause()
    {
        const auto directory=qEnvironmentVariable("VLC_THUMBNAIL_PLAYBACK_DIR");
        if(directory.isEmpty())QSKIP("Set VLC_THUMBNAIL_PLAYBACK_DIR to generated long video/audio fixtures");
        QFETCH(bool,startupPaused);
        const QDir fixtures(directory);
        const auto path=fixtures.filePath(QStringLiteral("motion-1080p50-silent.mp4"));
        const QImage reference(fixtures.filePath(QStringLiteral("reference-40.png")));
        QCOMPARE(reference.size(),QSize(252,142));
        const QByteArray log=QByteArray("--logfile=startup-")+(startupPaused?"paused":"playing")+"-native.log";
        const char *args[]={"--ignore-config","--no-media-library","--no-hw-dec",
            "--aout=adummy","--vout=vdummy","--verbose=2","--no-metadata-network-access",
            "--file-logging",log.constData(),startupPaused?"--start-paused":"--no-start-paused"};
        std::unique_ptr<libvlc_instance_t,decltype(&libvlc_release)> instance(libvlc_new(ARRAY_SIZE(args),args),libvlc_release);
        QVERIFY(instance);
        std::unique_ptr<libvlc_media_player_t,decltype(&libvlc_media_player_release)> player(
            libvlc_media_player_new(instance.get(),nullptr,nullptr),libvlc_media_player_release);
        QVERIFY(player);
        auto *object=vlc_test_media_player_object(player.get());
        QCOMPARE(var_Create(object,"window",VLC_VAR_STRING),VLC_SUCCESS);
        QCOMPARE(var_SetString(object,"window","wdummy,none"),VLC_SUCCESS);
        QCOMPARE(var_InheritBool(object,"start-paused"),startupPaused);
        const auto uri=QUrl::fromLocalFile(path).toEncoded();
        std::unique_ptr<libvlc_media_t,decltype(&libvlc_media_release)> media(libvlc_media_new_location(uri.constData()),libvlc_media_release);
        QVERIFY2(media,libvlc_errmsg());
        libvlc_media_player_set_media(player.get(),media.get());
        auto *core=vlc_test_media_player_core(player.get());
        vlc_player_Lock(core);
        vlc_player_SetStartPaused(core,startupPaused);
        const int started=vlc_player_Start(core);
        vlc_player_Unlock(core);
        QCOMPARE(started,VLC_SUCCESS);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()),startupPaused?libvlc_Paused:libvlc_Playing,6000);
        if(!startupPaused) {
            QTRY_VERIFY_WITH_TIMEOUT(libvlc_media_player_get_time(player.get())>100000,6000);
            libvlc_media_player_set_pause(player.get(),1);
            QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()),libvlc_Paused,5000);
        }
        QTest::qWait(200);
        const auto before=libvlc_media_player_get_time(player.get());
        SharedInputItem input(input_item_New(uri.constData(),"Owned long video/audio preview"),false);
        QVERIFY(input);
        auto preview=std::make_unique<TimelineThumbnails>(object);
        std::unique_ptr<QQuickImageProvider> provider(preview->createImageProvider());
        preview->setInput(input.get());preview->setEnabled(true);
        QElapsedTimer elapsed;elapsed.start();
        QVERIFY(preview->request(40.5));
        QTRY_VERIFY_WITH_TIMEOUT(!preview->loading(),6000);
        QVERIFY2(!preview->imageSource().isEmpty(),qPrintable(preview->error()));
        QCOMPARE(preview->requestedSeconds(),40.);
        QSize size;
        const auto actual=provider->requestImage(QUrl(preview->imageSource()).path().mid(1),&size,{});
        QCOMPARE(actual.size(),reference.size());
        for(const auto &point:{QPoint(20,20),QPoint(232,20),QPoint(20,120),QPoint(232,120)}) {
            const auto a=actual.pixelColor(point),b=reference.pixelColor(point);
            QVERIFY2(std::abs(a.red()-b.red())<35&&std::abs(a.green()-b.green())<35&&std::abs(a.blue()-b.blue())<35,
                     qPrintable(QStringLiteral("40s reference mismatch at %1,%2").arg(point.x()).arg(point.y())));
        }
        QCOMPARE(libvlc_media_player_get_state(player.get()),libvlc_Paused);
        QCOMPARE(libvlc_media_player_get_time(player.get()),before);
        const auto readyMs=elapsed.elapsed();elapsed.restart();preview.reset();
        QVERIFY(elapsed.elapsed()<2000);
        const auto output=qEnvironmentVariable("VLC_THUMBNAIL_STARTUP_OUTPUT");
        if(!output.isEmpty()) {
            const QDir folder(output);QVERIFY(folder.mkpath(QStringLiteral(".")));
            const auto name=QString::fromLatin1(QTest::currentDataTag());
            QVERIFY(actual.save(folder.filePath(name+QStringLiteral(".png"))));
            QFile report(folder.filePath(name+QStringLiteral(".json")));QVERIFY(report.open(QIODevice::WriteOnly));
            report.write(QJsonDocument(QJsonObject{{"startupPaused",startupPaused},{"requestedSecond",40},{"readyMs",readyMs},{"primaryPausedTimeUs",before},{"primaryTimeUnchanged",true},{"referencePixelsMatch",true}}).toJson());
        }
        libvlc_media_player_set_pause(player.get(),0);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()),libvlc_Playing,5000);
        QTRY_VERIFY_WITH_TIMEOUT(libvlc_media_player_get_time(player.get())>before+250000,5000);
        libvlc_media_player_stop_async(player.get());
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(player.get()),libvlc_Stopped,5000);
    }
    void codecContainersKeepPrimaryPaused_data()
    {
        QTest::addColumn<QString>("name");
        for (const auto *name : {"h264.mp4", "hevc-multitrack.mkv", "vp9.webm",
                                 "mpeg4.avi", "mpeg2.mpg", "h264.ts", "audio-only.flac"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void codecContainersKeepPrimaryPaused()
    {
        const auto directory = qEnvironmentVariable("VLC_THUMBNAIL_FORMAT_DIR");
        if (directory.isEmpty()) QSKIP("Set VLC_THUMBNAIL_FORMAT_DIR to generated codec/container fixtures");
        QFETCH(QString, name);
        const QDir fixtures(directory);
        const auto uri = QUrl::fromLocalFile(fixtures.filePath(name)).toEncoded();
        SharedInputItem input(input_item_New(uri.constData(), "Synthetic format preview"), false);
        QVERIFY(input);

        // Keep the independent primary on the original MP4. Extracting another
        // container must neither select tracks nor seek/resume this player.
        auto *media = libvlc_media_new_location(m_url.toEncoded().constData());
        QVERIFY(media);
        libvlc_media_player_set_media(m_player, media);
        libvlc_media_release(media);
        QCOMPARE(libvlc_media_player_play(m_player), 0);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Playing, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(libvlc_media_player_get_time(m_player) > 100000, 5000);
        libvlc_media_player_set_pause(m_player, 1);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Paused, 5000);
        QTest::qWait(200);
        const auto before = libvlc_media_player_get_time(m_player);
        m_preview->setInput(input.get());
        m_preview->setEnabled(true);
        QJsonArray reports;
        for (const auto second : {2, 8, 14}) {
            QElapsedTimer timer;
            timer.start();
            QVERIFY(m_preview->request(second));
            QTRY_VERIFY_WITH_TIMEOUT(!m_preview->loading(), 6000);
            QCOMPARE(libvlc_media_player_get_state(m_player), libvlc_Paused);
            QCOMPARE(libvlc_media_player_get_time(m_player), before);
            if (name == QStringLiteral("audio-only.flac")) {
                QVERIFY(m_preview->imageSource().isEmpty());
                QVERIFY(!m_preview->error().isEmpty());
                reports.append(QJsonObject{{"second", second}, {"noVideoFallback", true}, {"readyMs", timer.elapsed()}});
                break;
            }
            QVERIFY2(!m_preview->imageSource().isEmpty(), qPrintable(m_preview->error()));
            const auto actual = frame();
            const QImage reference(fixtures.filePath(name + QStringLiteral("-%1.png").arg(second)));
            QCOMPARE(actual.size(), QSize(252, 142));
            QCOMPARE(reference.size(), actual.size());
            for (const auto &point : {QPoint(20,20), QPoint(232,20), QPoint(126,71), QPoint(20,120), QPoint(232,120)}) {
                const auto a = actual.pixelColor(point), b = reference.pixelColor(point);
                QVERIFY2(std::abs(a.red()-b.red()) < 35 && std::abs(a.green()-b.green()) < 35 && std::abs(a.blue()-b.blue()) < 35,
                         qPrintable(QStringLiteral("%1 second %2 differs from FFmpeg reference").arg(name).arg(second)));
            }
            reports.append(QJsonObject{{"second", second}, {"referencePixelsMatch", true}, {"readyMs", timer.elapsed()}});
            const auto output = qEnvironmentVariable("VLC_THUMBNAIL_FORMAT_OUTPUT");
            if (!output.isEmpty()) {
                const QDir folder(output); QVERIFY(folder.mkpath(QStringLiteral(".")));
                QVERIFY(actual.save(folder.filePath(name + QStringLiteral("-%1.png").arg(second))));
            }
        }
        m_preview->setInput(m_input.get());
        libvlc_media_player_set_pause(m_player, 0);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Playing, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(libvlc_media_player_get_time(m_player) > before + 250000, 5000);
        libvlc_media_player_stop_async(m_player);
        QTRY_COMPARE_WITH_TIMEOUT(libvlc_media_player_get_state(m_player), libvlc_Stopped, 5000);
        const auto output = qEnvironmentVariable("VLC_THUMBNAIL_FORMAT_OUTPUT");
        if (!output.isEmpty()) {
            const QDir folder(output); QVERIFY(folder.mkpath(QStringLiteral(".")));
            QFile report(folder.filePath(name + QStringLiteral(".json"))); QVERIFY(report.open(QIODevice::WriteOnly));
            report.write(QJsonDocument(QJsonObject{{"name", name}, {"requests", reports}, {"primaryTimeUnchanged", true},
                {"primaryResumedAndStopped", true}}).toJson());
        }
    }
    void cleanupTestCase()
    {
        m_preview.reset();m_provider.reset();m_input.reset();
        if(m_player)libvlc_media_player_release(m_player);
        if(m_instance)libvlc_release(m_instance);
    }
};
QTEST_GUILESS_MAIN(TestTimelineThumbnails)
#include "test_timeline_thumbnails.moc"
