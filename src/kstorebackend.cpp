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
#include <QJsonDocument>
#include <QJsonObject>

KStoreBackend::KStoreBackend(QObject *parent)
    : QObject(parent), m_network(new QNetworkAccessManager(this))
{
    updateInstalledApps();
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

void KStoreBackend::setAppProgress(const QString &packageId, double progress, const QString &status)
{
    QVariantMap data;
    data["progress"] = progress;
    data["status"] = status;
    data["installing"] = (progress < 1.0 && progress >= 0);
    m_installationProgress[packageId] = data;
    emit installationProgressChanged();
}

void KStoreBackend::updateInstalledApps()
{
    m_installedApps.clear();
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir dir(appPath);
    QStringList files = dir.entryList({"kstore-*.desktop"}, QDir::Files);
    for (const QString &file : files) {
        // file is "kstore-com.package.id.desktop"
        // pkg should be "com.package.id"
        QString pkg = file.mid(7); // skip "kstore-"
        if (pkg.endsWith(".desktop")) {
            pkg.chop(8); // remove ".desktop"
        }
        m_installedApps << pkg;
    }
    emit installedAppsChanged();
}

void KStoreBackend::openApp(const QString &packageId)
{
    QProcess::startDetached("waydroid", {"app", "launch", packageId});
}

void KStoreBackend::createDesktopFile(const QString &packageId, const QString &name, const QString &iconPath)
{
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(appPath);
    QString filePath = appPath + "/kstore-" + packageId + ".desktop";

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=" << name << "\n";
        out << "Exec=waydroid app launch " << packageId << "\n";
        out << "Icon=" << iconPath << "\n";
        out << "Categories=TV;Video;Streaming;\n";
        out << "X-KDE-Wayland-Interfaces=org.kde.kwin.wayland_interface\n";
        file.close();
    }
    updateInstalledApps();
}

void KStoreBackend::hideWaydroidDesktops()
{
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir dir(appPath);
    QStringList files = dir.entryList({"waydroid.*.desktop"}, QDir::Files);
    for (const QString &filename : files) {
        QString fullPath = appPath + "/" + filename;
        QFile file(fullPath);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString content = file.readAll();
            if (!content.contains("NoDisplay=true")) {
                if (!content.endsWith("\n")) content += "\n";
                content += "NoDisplay=true\n";
                file.resize(0);
                file.write(content.toUtf8());
            }
            file.close();
        }
    }
}

// ---------------------------------------------------------------------------
// Waydroid helpers
// ---------------------------------------------------------------------------

QString KStoreBackend::waydroidContainerIp() const
{
    QFile leases("/var/lib/misc/dnsmasq.waydroid0.leases");
    if (!leases.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&leases);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QStringList parts = line.split(' ', Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            QString ip = parts[2];
            if (ip.startsWith("192.168.240."))
                return ip;
        }
    }
    return {};
}

void KStoreBackend::ensureWaydroidStarted()
{
    if (m_waydroidLaunched) return;
    m_waydroidLaunched = true;
    QProcess::startDetached("waydroid", {"session", "start"});
}

// ---------------------------------------------------------------------------
// Public install entry-point
// ---------------------------------------------------------------------------

void KStoreBackend::installApp(const QString &packageId, const QString &appName, const QString &iconUrl)
{
    setAppProgress(packageId, 0.0, "Starting download...");
    emit installationStarted(packageId);

    // Save metadata for later
    m_installationProgress[packageId + "_name"] = appName;
    m_installationProgress[packageId + "_iconUrl"] = iconUrl;

    QString downloadUrl = QString("https://d.apkpure.com/b/APK/%1?version=latest").arg(packageId);
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
    connect(reply, &QNetworkReply::finished, this, &KStoreBackend::onDownloadFinished);
    connect(reply, &QNetworkReply::downloadProgress, this, &KStoreBackend::onDownloadProgress);
}

void KStoreBackend::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply) {
        QString pkg = reply->property("packageId").toString();
        double p = (bytesTotal > 0) ? (double)bytesReceived / bytesTotal : 0;
        setAppProgress(pkg, p * 0.5, QString("Downloading (%1%)").arg(int(p * 100)));
        emit downloadProgress(pkg, bytesReceived, bytesTotal);
    }
}

void KStoreBackend::onDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    QString packageId = reply->property("packageId").toString();

    if (reply->error() != QNetworkReply::NoError) {
        setAppProgress(packageId, -1.0, "Download failed");
        emit installationFinished(packageId, false, "Download failed: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QString tempPath = QDir::tempPath() + "/" + packageId + ".apk";
    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        setAppProgress(packageId, -1.0, "Storage error");
        emit installationFinished(packageId, false, "Could not write APK");
        reply->deleteLater();
        return;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();

    setAppProgress(packageId, 0.5, "Preparing Waydroid...");

    // Start icon download
    downloadIcon(packageId, m_installationProgress[packageId + "_iconUrl"].toString());

    ensureWaydroidStarted();
    tryAdbInstall(packageId, tempPath, 0);
}

void KStoreBackend::downloadIcon(const QString &packageId, const QString &url)
{
    if (url.isEmpty()) return;
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = m_network->get(request);
    reply->setProperty("packageId", packageId);
    connect(reply, &QNetworkReply::finished, this, &KStoreBackend::onIconDownloadFinished);
}

void KStoreBackend::onIconDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    QString packageId = reply->property("packageId").toString();
    if (reply->error() == QNetworkReply::NoError) {
        QString iconDir = QDir::homePath() + "/.local/share/icons";
        QDir().mkpath(iconDir);
        QString iconPath = iconDir + "/" + packageId + ".webp";
        QFile file(iconPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(reply->readAll());
            file.close();
            m_installationProgress[packageId + "_iconPath"] = iconPath;
        }
    }
    reply->deleteLater();
}

// ---------------------------------------------------------------------------
// ADB install with retry
// ---------------------------------------------------------------------------

static constexpr int kMaxRetries = 10;

void KStoreBackend::tryAdbInstall(const QString &packageId, const QString &apkPath, int attempt)
{
    if (attempt >= kMaxRetries) {
        QFile::remove(apkPath);
        setAppProgress(packageId, -1.0, "Waydroid timeout");
        emit installationFinished(packageId, false, "Waydroid timeout");
        return;
    }

    QString ip = waydroidContainerIp();
    if (ip.isEmpty()) {
        setAppProgress(packageId, 0.6, "Waiting for Waydroid...");
        QTimer::singleShot(15000, this, [this, packageId, apkPath, attempt]() {
            tryAdbInstall(packageId, apkPath, attempt + 1);
        });
        return;
    }

    QString target = ip + ":5555";
    auto *connectProc = new QProcess(this);
    connect(connectProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, packageId, apkPath, attempt, target, connectProc] (int, QProcess::ExitStatus) {
        QString connectOut = connectProc->readAllStandardOutput().trimmed();
        connectProc->deleteLater();
        bool connected = connectOut.contains("connected to") || connectOut.contains("already connected");

        if (!connected) {
            QTimer::singleShot(10000, this, [this, packageId, apkPath, attempt]() {
                tryAdbInstall(packageId, apkPath, attempt + 1);
            });
            return;
        }

        setAppProgress(packageId, 0.8, "Installing APK...");
        auto *installProc = new QProcess(this);
        connect(installProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, packageId, apkPath, attempt, installProc] (int exitCode, QProcess::ExitStatus exitStatus) {
            QString installOut = installProc->readAllStandardOutput().trimmed() + installProc->readAllStandardError().trimmed();
            installProc->deleteLater();
            bool success = (exitStatus == QProcess::NormalExit && exitCode == 0 && installOut.contains("Success"));

            if (success) {
                QFile::remove(apkPath);
                createDesktopFile(packageId, m_installationProgress[packageId + "_name"].toString(), m_installationProgress[packageId + "_iconPath"].toString());
                hideWaydroidDesktops();
                setAppProgress(packageId, 1.0, "Installed");
                emit installationFinished(packageId, true, "Installed " + packageId);
            } else if (installOut.contains("INSTALL_FAILED")) {
                QFile::remove(apkPath);
                setAppProgress(packageId, -1.0, "Install failed");
                emit installationFinished(packageId, false, "ADB install failed");
            } else {
                QTimer::singleShot(10000, this, [this, packageId, apkPath, attempt]() {
                    tryAdbInstall(packageId, apkPath, attempt + 1);
                });
            }
        });
        installProc->start("adb", {"-s", target, "install", "-r", apkPath});
    });
    connectProc->start("adb", {"connect", target});
}
