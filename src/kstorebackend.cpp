#include "kstorebackend.h"
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QTextStream>

KStoreBackend::KStoreBackend(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

QVariantList KStoreBackend::apps() const
{
    return m_apps;
}

void KStoreBackend::setLoading(bool loading)
{
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

void KStoreBackend::fetchApps(const QString &query)
{
    setLoading(true);
    m_apps.clear();
    emit appsChanged();

    QStringList curated;
    if (query.contains("streaming", Qt::CaseInsensitive)) {
        curated << "com.netflix.mediaclient"
                << "com.google.android.youtube"
                << "com.disney.disneyplus"
                << "com.amazon.avod.thirdpartyclient"
                << "com.hulu.plus"
                << "com.plexapp.android"
                << "tv.twitch.android.app";
    } else {
        curated << "org.xbmc.kodi"
                << "org.videolan.vlc"
                << "com.google.android.apps.chromecast.app";
    }

    for (const QString &id : curated) {
        QUrl url("https://play.google.com/store/apps/details?id=" + id);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader,
                          "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                          "(KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *detailsReply = m_network->get(request);
        detailsReply->setProperty("packageId", id);
        connect(detailsReply, &QNetworkReply::finished,
                this, &KStoreBackend::onAppDetailsFinished);
    }
}

void KStoreBackend::onAppDetailsFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString packageId = reply->property("packageId").toString();
    if (reply->error() == QNetworkReply::NoError) {
        QString html = reply->readAll();

        QString name;
        QRegularExpression nameRe("itemprop=\"name\">([^<]*)");
        QRegularExpressionMatch nameMatch = nameRe.match(html);
        if (nameMatch.hasMatch()) {
            name = nameMatch.captured(1);
        } else {
            QRegularExpression nameReFallback("<h1[^>]*><span>([^<]*)</span></h1>");
            QRegularExpressionMatch nameMatchFallback = nameReFallback.match(html);
            if (nameMatchFallback.hasMatch()) name = nameMatchFallback.captured(1);
        }

        QString icon;
        QRegularExpression iconRe("https://play-lh.googleusercontent.com/[^ \"^<]+");
        QRegularExpressionMatch iconMatch = iconRe.match(html);
        if (iconMatch.hasMatch()) icon = iconMatch.captured(0);

        if (name.isEmpty()) name = packageId;

        QVariantMap app;
        app["name"]      = name;
        app["packageId"] = packageId;
        app["icon"]      = icon;
        app["category"]  = "Android App";
        m_apps.append(app);
        emit appsChanged();
    }
    setLoading(false);
    reply->deleteLater();
}

// ---------------------------------------------------------------------------
// Waydroid helpers
// ---------------------------------------------------------------------------

// Read the container IP from dnsmasq's lease file.
// Format: <timestamp> <mac> <ip> <hostname> <clientid>
// Returns empty string if not found.
QString KStoreBackend::waydroidContainerIp() const
{
    QFile leases("/var/lib/misc/dnsmasq.waydroid0.leases");
    if (!leases.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&leases);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        // Column 2 (0-indexed) is the IP address
        if (parts.size() >= 3) {
            QString ip = parts[2];
            if (ip.startsWith("192.168.240."))   // sanity-check waydroid subnet
                return ip;
        }
    }
    return {};
}

// Start "waydroid session start" detached in the background (once).
void KStoreBackend::ensureWaydroidStarted()
{
    if (m_waydroidLaunched) return;
    m_waydroidLaunched = true;

    qDebug() << "[KStore] Launching waydroid session start in background...";
    QProcess::startDetached("waydroid", {"session", "start"});
}

// ---------------------------------------------------------------------------
// Public install entry-point
// ---------------------------------------------------------------------------

void KStoreBackend::installApp(const QString &packageId)
{
    emit installationStarted(packageId);
    qDebug() << "[KStore] Fetching APK for" << packageId;

    QString downloadUrl =
        QString("https://d.apkpure.com/b/APK/%1?version=latest").arg(packageId);
    downloadAPK(packageId, downloadUrl);
}

// ---------------------------------------------------------------------------
// Download
// ---------------------------------------------------------------------------

void KStoreBackend::downloadAPK(const QString &packageId, const QString &url)
{
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                      "(KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    reply->setProperty("packageId", packageId);
    connect(reply, &QNetworkReply::finished,
            this, &KStoreBackend::onDownloadFinished);
    connect(reply, &QNetworkReply::downloadProgress,
            this, &KStoreBackend::onDownloadProgress);
}

void KStoreBackend::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply)
        emit downloadProgress(reply->property("packageId").toString(),
                              bytesReceived, bytesTotal);
}

void KStoreBackend::onDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString packageId = reply->property("packageId").toString();

    if (reply->error() != QNetworkReply::NoError) {
        emit installationFinished(packageId, false,
                                  "Download failed: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // Write APK to /tmp
    QString tempPath = QDir::tempPath() + "/" + packageId + ".apk";
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit installationFinished(packageId, false,
                                  "Could not write APK to " + tempPath);
        reply->deleteLater();
        return;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();

    qDebug() << "[KStore] APK saved to" << tempPath
             << "- handing off to ADB installer";

    // Kick waydroid if it isn't running yet, then start the install loop.
    ensureWaydroidStarted();
    tryAdbInstall(packageId, tempPath, 0);
}

// ---------------------------------------------------------------------------
// ADB install with retry
// ---------------------------------------------------------------------------

// Max attempts before we give up (30 s × 10 = 5 min)
static constexpr int kMaxRetries = 10;

void KStoreBackend::tryAdbInstall(const QString &packageId,
                                  const QString &apkPath,
                                  int attempt)
{
    if (attempt >= kMaxRetries) {
        QFile::remove(apkPath);
        emit installationFinished(packageId, false,
                                  "Waydroid did not become ready after "
                                  + QString::number(kMaxRetries) + " attempts.");
        return;
    }

    QString ip = waydroidContainerIp();
    if (ip.isEmpty()) {
        qDebug() << "[KStore] Waydroid container IP not found yet, retry"
                 << attempt + 1 << "in 30 s...";
        QTimer::singleShot(30000, this, [this, packageId, apkPath, attempt]() {
            tryAdbInstall(packageId, apkPath, attempt + 1);
        });
        return;
    }

    QString target = ip + ":5555";
    qDebug() << "[KStore] Connecting ADB to" << target;

    // Step 1: adb connect <ip>:5555
    auto *connectProc = new QProcess(this);
    connect(connectProc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, packageId, apkPath, attempt, target, connectProc]
            (int exitCode, QProcess::ExitStatus) {

        QString connectOut = connectProc->readAllStandardOutput().trimmed();
        connectProc->deleteLater();

        qDebug() << "[KStore] adb connect output:" << connectOut;

        // adb connect returns 0 even on "already connected / refused", so
        // we inspect stdout instead.
        bool connected = connectOut.contains("connected to")
                      || connectOut.contains("already connected");

        if (!connected) {
            qDebug() << "[KStore] ADB connect failed, retry" << attempt + 1
                     << "in 30 s...";
            QTimer::singleShot(30000, this, [this, packageId, apkPath, attempt]() {
                tryAdbInstall(packageId, apkPath, attempt + 1);
            });
            return;
        }

        // Step 2: adb -s <ip>:5555 install -r <apk>
        qDebug() << "[KStore] ADB connected, installing" << apkPath;
        auto *installProc = new QProcess(this);
        connect(installProc,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, packageId, apkPath, attempt, target, installProc]
                (int exitCode, QProcess::ExitStatus exitStatus) {

            QString installOut = installProc->readAllStandardOutput().trimmed()
                               + installProc->readAllStandardError().trimmed();
            installProc->deleteLater();

            qDebug() << "[KStore] adb install output:" << installOut;

            bool success = (exitStatus == QProcess::NormalExit && exitCode == 0
                            && installOut.contains("Success"));

            if (success) {
                QFile::remove(apkPath);
                emit installationFinished(packageId, true,
                                          "Installed " + packageId);
            } else if (installOut.contains("INSTALL_FAILED")
                       || exitStatus != QProcess::NormalExit) {
                // Hard failure – don't retry
                QFile::remove(apkPath);
                emit installationFinished(packageId, false,
                                          "ADB install failed: " + installOut);
            } else {
                // Waydroid may still be booting; retry
                qDebug() << "[KStore] Install not successful yet, retry"
                         << attempt + 1 << "in 30 s...";
                QTimer::singleShot(30000, this,
                    [this, packageId, apkPath, attempt]() {
                        tryAdbInstall(packageId, apkPath, attempt + 1);
                    });
            }
        });

        installProc->start("adb",
            {"-s", target, "install", "-r", apkPath});

        if (!installProc->waitForStarted(3000)) {
            installProc->deleteLater();
            QFile::remove(apkPath);
            emit installationFinished(packageId, false,
                                      "adb not found or failed to start.");
        }
    });

    connectProc->start("adb", {"connect", target});
    if (!connectProc->waitForStarted(3000)) {
        connectProc->deleteLater();
        QFile::remove(apkPath);
        emit installationFinished(packageId, false,
                                  "adb not found or failed to start.");
    }
}#include "kstorebackend.h"
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QProcess>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

KStoreBackend::KStoreBackend(QObject *parent) : QObject(parent), m_network(new QNetworkAccessManager(this))
{
}

QVariantList KStoreBackend::apps() const
{
    return m_apps;
}

void KStoreBackend::setLoading(bool loading)
{
    if (m_loading != loading) {
        m_loading = loading;
        emit loadingChanged();
    }
}

void KStoreBackend::fetchApps(const QString &query)
{
    setLoading(true);
    m_apps.clear();
    emit appsChanged();

    QStringList curated;
    if (query.contains("streaming", Qt::CaseInsensitive)) {
        curated << "com.netflix.mediaclient"
                << "com.google.android.youtube"
                << "com.disney.disneyplus"
                << "com.amazon.avod.thirdpartyclient"
                << "com.hulu.plus"
                << "com.plexapp.android"
                << "tv.twitch.android.app";
    } else {
        curated << "org.xbmc.kodi"
                << "org.videolan.vlc"
                << "com.google.android.apps.chromecast.app";
    }

    for (const QString &id : curated) {
        QUrl url("https://play.google.com/store/apps/details?id=" + id);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
        // In Qt6, FollowRedirectsAttribute is gone. Use RedirectPolicyAttribute.
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *detailsReply = m_network->get(request);
        detailsReply->setProperty("packageId", id);
        connect(detailsReply, &QNetworkReply::finished, this, &KStoreBackend::onAppDetailsFinished);
    }
}

void KStoreBackend::onAppDetailsFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString packageId = reply->property("packageId").toString();
    if (reply->error() == QNetworkReply::NoError) {
        QString html = reply->readAll();

        QString name;
        QRegularExpression nameRe("itemprop=\"name\">([^<]*)");
        QRegularExpressionMatch nameMatch = nameRe.match(html);
        if (nameMatch.hasMatch()) {
            name = nameMatch.captured(1);
        } else {
            QRegularExpression nameReFallback("<h1[^>]*><span>([^<]*)</span></h1>");
            QRegularExpressionMatch nameMatchFallback = nameReFallback.match(html);
            if (nameMatchFallback.hasMatch()) name = nameMatchFallback.captured(1);
        }

        QString icon;
        QRegularExpression iconRe("https://play-lh.googleusercontent.com/[^ \"^<]+");
        QRegularExpressionMatch iconMatch = iconRe.match(html);
        if (iconMatch.hasMatch()) icon = iconMatch.captured(0);

        if (name.isEmpty()) name = packageId;

        QVariantMap app;
        app["name"] = name;
        app["packageId"] = packageId;
        app["icon"] = icon;
        app["category"] = "Android App";
        m_apps.append(app);
        emit appsChanged();
    }
    setLoading(false);
    reply->deleteLater();
}

void KStoreBackend::installApp(const QString &packageId)
{
    emit installationStarted(packageId);
    qDebug() << "Fetching APK for" << packageId;

    QString downloadUrl = QString("https://d.apkpure.com/b/APK/%1?version=latest").arg(packageId);
    downloadAPK(packageId, downloadUrl);
}

void KStoreBackend::downloadAPK(const QString &packageId, const QString &url)
{
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    // Explicitly use RedirectPolicyAttribute for Qt6
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    reply->setProperty("packageId", packageId);
    connect(reply, &QNetworkReply::finished, this, &KStoreBackend::onDownloadFinished);
    connect(reply, &QNetworkReply::downloadProgress, this, &KStoreBackend::onDownloadProgress);
}

void KStoreBackend::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        emit downloadProgress(reply->property("packageId").toString(), bytesReceived, bytesTotal);
    }
}

void KStoreBackend::onDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString packageId = reply->property("packageId").toString();
    if (reply->error() == QNetworkReply::NoError) {
        QString tempPath = QDir::tempPath() + "/" + packageId + ".apk";
        QFile file(tempPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();

            qDebug() << "Downloaded APK to" << tempPath << ". Installing via Waydroid...";

            QProcess *process = new QProcess(this);
            connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    [this, packageId, process, tempPath](int exitCode, QProcess::ExitStatus exitStatus) {
                bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
                emit installationFinished(packageId, success, success ? "Installed " + packageId : "Waydroid failed to install " + packageId);
                QFile::remove(tempPath);
                process->deleteLater();
            });

            process->start("waydroid", QStringList() << "app" << "install" << tempPath);

            if (!process->waitForStarted(500)) {
                emit installationFinished(packageId, false, "Waydroid not found or failed to start.");
                QFile::remove(tempPath);
            }
        }
    } else {
        emit installationFinished(packageId, false, "Download failed: " + reply->errorString());
    }
    reply->deleteLater();
}
