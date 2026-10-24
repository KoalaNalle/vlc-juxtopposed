// SPDX-License-Identifier: GPL-2.0-or-later
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "plugin_cache_rescan.hpp"
#include <vlc_common.h>
#include <vlc_modules.h>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QLockFile>
#include <QProcessEnvironment>
#include <QSaveFile>
#include <QTemporaryDir>
#include <atomic>
#include <functional>
#ifdef _WIN32
# include <qt_windows.h>
#endif

namespace {
constexpr qint64 MaximumCacheBytes = 64 * 1024 * 1024;
constexpr qint64 MaximumPluginBytes = 16ll * 1024 * 1024 * 1024;
constexpr qsizetype MaximumFiles = 4096;
using Inventory = QMap<QString, QPair<qint64, qint64>>;
using Interrupted = std::function<bool()>;

bool inventory(const QString &root, Inventory &result, const Interrupted &interrupted)
{
    qint64 bytes = 0;
    QDirIterator it(root, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (interrupted()) return false;
        it.next();
        const auto info = it.fileInfo();
        if (info.isSymLink() || info.isJunction()) return false;
        if (info.isDir()) { if (!info.isReadable()) return false; continue; }
        if (!info.isFile()) return false;
        if (info.suffix().compare(QStringLiteral("dll"), Qt::CaseInsensitive)) continue;
        if (info.size() <= 0 || info.size() > MaximumPluginBytes - bytes
            || result.size() >= MaximumFiles) return false;
        bytes += info.size();
        result.insert(QDir(root).relativeFilePath(info.absoluteFilePath()),
                      {info.size(), info.lastModified().toMSecsSinceEpoch()});
    }
    return !result.isEmpty();
}

bool cacheState(const QString &root, QByteArray &digest, bool &exists)
{
    QFile file(QDir(root).filePath(QStringLiteral("plugins.dat")));
    const QFileInfo info(file);
    exists = info.exists();
    if (!exists) { digest.clear(); return true; }
    if (!info.isFile() || info.isSymLink() || info.size() > MaximumCacheBytes
        || !file.open(QIODevice::ReadOnly)) return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file) || file.error() != QFileDevice::NoError) return false;
    digest = hash.result();
    return true;
}

}

struct PluginCacheRescan::Job {
    std::atomic_bool cancelled{false};
    std::unique_ptr<QTemporaryDir> storage;
    std::unique_ptr<QLockFile> lock;
    QString root, cacheDirectory;
    Inventory files;
    QByteArray oldDigest, candidate;
    bool oldExists = false;
};

PluginCacheRescan::PluginCacheRescan(vlc_object_t *parent, QObject *objectParent)
    : PluginCacheRescan(parent, {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("vlc-cache-gen.exe")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("plugins"))}, Limits{}, objectParent)
{}

PluginCacheRescan::PluginCacheRescan(vlc_object_t *parent, Layout layout, Limits limits, QObject *objectParent)
    : QObject(objectParent), m_parent(parent), m_layout(std::move(layout)), m_limits(limits)
{
    m_limits.stageMs = qBound(1, m_limits.stageMs, 30000);
    m_limits.processMs = qBound(1, m_limits.processMs, 30000);
    m_pool.setMaxThreadCount(1);
    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        m_timedOut = true;
        if (m_job) m_job->cancelled = true;
        m_process.kill();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_job && error == QProcess::FailedToStart)
            finish(false, tr("The plugin scanner could not start. The previous cache is unchanged."));
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        m_deadline.stop();
        if (!m_job) return;
        if (m_job->cancelled || status != QProcess::NormalExit || code != 0) {
            finish(false, m_timedOut
                ? tr("The plugin scan timed out. The previous cache is unchanged.")
                : tr("The plugin scan did not finish. The previous cache is unchanged."));
            return;
        }
        verify(m_job);
    });
#ifdef _WIN32
    m_process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
}

PluginCacheRescan::~PluginCacheRescan()
{
    cancel();
    m_deadline.stop();
    disconnect(&m_process, nullptr, this, nullptr);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(3000);
    }
    m_pool.waitForDone();
}

bool PluginCacheRescan::available() const
{
    const QFileInfo program(m_layout.generator), plugins(m_layout.plugins);
    return m_parent && program.isAbsolute() && program.isFile() && program.isReadable()
        && plugins.isAbsolute() && plugins.isDir() && plugins.isReadable()
        && !plugins.isSymLink() && !plugins.isJunction();
}

bool PluginCacheRescan::start()
{
    if (busy()) return false;
    m_error.clear(); emit lastErrorChanged();
    if (!available()) {
        m_error = tr("The installed plugin scanner or plugin folder is unavailable.");
        emit lastErrorChanged(); return false;
    }
    const auto job = m_job = std::make_shared<Job>();
    job->root = QFileInfo(m_layout.plugins).canonicalFilePath();
    if (job->root.isEmpty() || !QDir::isAbsolutePath(job->root)) {
        m_job.reset();
        m_error = tr("The plugin folder is no longer available.");
        emit lastErrorChanged(); return false;
    }
    job->lock = std::make_unique<QLockFile>(
        QDir(job->root).filePath(QStringLiteral("plugins.dat.rescan.lock")));
    if (!job->lock->tryLock(0)) {
        m_job.reset();
        m_error = tr("The plugin cache is unavailable or another rescan is running.");
        emit lastErrorChanged(); return false;
    }
    m_timedOut = false;
    emit busyChanged();
    m_pool.start([this, job] {
        QElapsedTimer timer; timer.start();
        const auto interrupted = [&] { return job->cancelled.load() || timer.elapsed() >= m_limits.stageMs; };
        job->storage = std::make_unique<QTemporaryDir>(
            QDir(QFileInfo(job->root).absolutePath()).filePath(QStringLiteral(".vlc-plugin-cache-XXXXXX")));
        bool ok = job->storage->isValid()
            && cacheState(job->root, job->oldDigest, job->oldExists)
            && inventory(job->root, job->files, interrupted);
        if (ok) {
            job->cacheDirectory = QDir(job->storage->path()).filePath(QStringLiteral("cache"));
            ok = QDir().mkpath(job->cacheDirectory);
        }
        ok = ok && !interrupted();
        QMetaObject::invokeMethod(this, [this, job, ok] {
            if (m_job != job) return;
            if (!ok || job->cancelled)
                finish(false, tr("The plugin scan could not be prepared. The previous cache is unchanged."));
            else launch(job);
        }, Qt::QueuedConnection);
    });
    return true;
}

void PluginCacheRescan::launch(const std::shared_ptr<Job> &job)
{
    auto env = QProcessEnvironment::systemEnvironment();
    for (const auto &key : env.keys()) if (key.startsWith(QStringLiteral("VLC_"))) env.remove(key);
    for (const auto &name : {QStringLiteral("userdata"), QStringLiteral("temp"),
                            QStringLiteral("appdata"), QStringLiteral("localappdata")})
        QDir(job->storage->path()).mkdir(name);
    env.insert(QStringLiteral("VLC_USERDATA_PATH"), job->storage->filePath(QStringLiteral("userdata")));
    for (const auto &key : {QStringLiteral("TEMP"), QStringLiteral("TMP")})
        env.insert(key, job->storage->filePath(QStringLiteral("temp")));
    env.insert(QStringLiteral("APPDATA"), job->storage->filePath(QStringLiteral("appdata")));
    env.insert(QStringLiteral("LOCALAPPDATA"), job->storage->filePath(QStringLiteral("localappdata")));
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(job->storage->path());
    m_process.setStandardOutputFile(QProcess::nullDevice());
    m_process.setStandardErrorFile(QProcess::nullDevice());
    m_deadline.start(m_limits.processMs);
    m_process.start(m_layout.generator, {QStringLiteral("--output"), job->cacheDirectory, job->root});
}

void PluginCacheRescan::verify(const std::shared_ptr<Job> &job)
{
    m_pool.start([this, job] {
        QElapsedTimer timer; timer.start();
        const auto interrupted = [&] { return job->cancelled.load() || timer.elapsed() >= m_limits.stageMs; };
        Inventory current;
        QByteArray digest;
        bool exists = false;
        QFile candidate(QDir(job->cacheDirectory).filePath(QStringLiteral("plugins.dat")));
        bool ok = !interrupted() && candidate.size() > 0 && candidate.size() <= MaximumCacheBytes
            && candidate.open(QIODevice::ReadOnly);
        if (ok) {
            job->candidate = candidate.readAll();
            ok = candidate.error() == QFileDevice::NoError && job->candidate.size() == candidate.size();
            candidate.close();
        }
        const auto native = job->cacheDirectory.toUtf8();
        ok = ok && vlc_module_cache_validate(m_parent, native.constData())
            && inventory(job->root, current, interrupted) && current == job->files
            && cacheState(job->root, digest, exists) && exists == job->oldExists
            && digest == job->oldDigest && !interrupted();
        QMetaObject::invokeMethod(this, [this, job, ok] {
            if (m_job != job) return;
            if (!ok || job->cancelled) {
                finish(false, tr("The generated cache could not be verified, or the plugins changed. The previous cache is unchanged."));
                return;
            }
            // Commit only an entirely validated candidate. Never fall back to
            // truncating the current cache if atomic publication is unavailable.
            QByteArray digest;
            bool exists = false;
            if (!cacheState(job->root, digest, exists) || exists != job->oldExists
                || digest != job->oldDigest) {
                finish(false, tr("The plugin cache changed during the scan. Please rescan again."));
                return;
            }
            QSaveFile target(QDir(job->root).filePath(QStringLiteral("plugins.dat")));
            target.setDirectWriteFallback(false);
            const bool saved = target.open(QIODevice::WriteOnly)
                && target.write(job->candidate) == job->candidate.size() && target.commit();
            finish(saved, saved ? QString{}
                : tr("The plugin cache could not be saved. The previous cache is unchanged."));
        }, Qt::QueuedConnection);
    });
}

void PluginCacheRescan::finish(bool success, const QString &error)
{
    m_deadline.stop();
    auto job = std::move(m_job);
    if (!job) return;
    job->lock->unlock();
    m_error = error;
    // Remove the owned candidate directory without blocking the page.
    if (job) m_pool.start([job] { job->storage.reset(); });
    emit lastErrorChanged();
    emit busyChanged();
    emit finished(success);
}

void PluginCacheRescan::cancel()
{
    if (!m_job) return;
    m_job->cancelled = true;
    if (m_process.state() != QProcess::NotRunning) m_process.kill();
}
