#include "../src/kstorebackend.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KStoreBackend backend;

    QObject::connect(&backend, &KStoreBackend::appsChanged, [&]() {
        QVariantList apps = backend.apps();
        if (apps.isEmpty()) return;

        qDebug() << "Success! Fetched" << apps.size() << "apps.";
        for (const auto &appVar : apps) {
            QVariantMap map = appVar.toMap();
            qDebug() << "App:" << map["name"].toString() << "(" << map["packageId"].toString() << ")";
        }

        if (apps.size() > 0) {
            qDebug() << "Functional Test Passed.";
            app.exit(0);
        }
    });

    QObject::connect(&backend, &KStoreBackend::loadingChanged, [&]() {
        if (!backend.loading() && backend.apps().isEmpty()) {
            qDebug() << "Failed to fetch apps.";
            app.exit(1);
        }
    });

    qDebug() << "Starting functional test (Network request to Play Store)...";
    backend.fetchApps("streaming");

    // Timeout after 30 seconds
    QTimer::singleShot(30000, &app, [&]() {
        qDebug() << "Test timed out.";
        app.exit(1);
    });

    return app.exec();
}
