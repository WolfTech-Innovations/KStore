#ifndef KSTOREBACKEND_H
#define KSTOREBACKEND_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QStringList>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class KStoreBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QVariantMap installationProgress READ installationProgress NOTIFY installationProgressChanged)
    Q_PROPERTY(QStringList installedApps READ installedApps NOTIFY installedAppsChanged)

public:
    explicit KStoreBackend(QObject *parent = nullptr);

    QVariantList apps() const;
    bool loading() const { return m_loading; }
    QVariantMap installationProgress() const { return m_installationProgress; }
    QStringList installedApps() const { return m_installedApps; }

    Q_INVOKABLE void fetchApps(const QString &query = "streaming");
    Q_INVOKABLE void installApp(const QString &packageId, const QString &appName, const QString &iconUrl);
    Q_INVOKABLE void openApp(const QString &packageId);
    Q_INVOKABLE void updateInstalledApps();
    Q_INVOKABLE void hideWaydroidDesktops();

signals:
    void appsChanged();
    void loadingChanged();
    void installationStarted(const QString &packageId);
    void installationFinished(const QString &packageId, bool success, const QString &message);
    void downloadProgress(const QString &packageId, qint64 bytesReceived, qint64 bytesTotal);
    void installationProgressChanged();
    void installedAppsChanged();

private slots:
    void onAppDetailsFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onIconDownloadFinished();

private:
    QVariantList m_apps;
    bool m_loading = false;
    QVariantMap m_installationProgress;
    QStringList m_installedApps;
    QNetworkAccessManager *m_network;

    void setLoading(bool loading);
    void downloadAPK(const QString &packageId, const QString &url);
    void downloadIcon(const QString &packageId, const QString &url);
    void createDesktopFile(const QString &packageId, const QString &name, const QString &iconPath);

    bool m_waydroidLaunched = false;
    QString waydroidContainerIp() const;
    void ensureWaydroidStarted();
    void tryAdbInstall(const QString &packageId, const QString &apkPath, int attempt);
    void setAppProgress(const QString &packageId, double progress, const QString &status);
};

#endif // KSTOREBACKEND_H
