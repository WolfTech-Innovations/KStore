#ifndef KSTOREBACKEND_H
#define KSTOREBACKEND_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariantMap>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

class KStoreBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit KStoreBackend(QObject *parent = nullptr);

    QVariantList apps() const;
    bool loading() const { return m_loading; }

    Q_INVOKABLE void fetchApps(const QString &query = "streaming");
    Q_INVOKABLE void installApp(const QString &packageId);

signals:
    void appsChanged();
    void loadingChanged();
    void installationStarted(const QString &packageId);
    void installationFinished(const QString &packageId, bool success, const QString &message);
    void downloadProgress(const QString &packageId, qint64 bytesReceived, qint64 bytesTotal);

private slots:
    void onAppDetailsFinished();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    QVariantList m_apps;
    bool m_loading = false;
    QNetworkAccessManager *m_network;
    void setLoading(bool loading);
    void downloadAPK(const QString &packageId, const QString &url);
};

#endif // KSTOREBACKEND_H
