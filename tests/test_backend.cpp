#include "../src/kstorebackend.h"
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KStoreBackend backend;

    QObject::connect(&backend, &KStoreBackend::appsChanged, [&]() {
        QVariantList apps = backend.apps();
        if (apps.isEmpty()) return;

        qDebug() << "Apps fetched:" << apps.size();
        assert(apps.size() > 0);

        // The backend hardcodes "Android App" as category for now
        for (const auto &appVar : apps) {
            QVariantMap map = appVar.toMap();
            assert(!map["name"].toString().isEmpty());
            assert(!map["packageId"].toString().isEmpty());
            qDebug() << "Verified App:" << map["name"].toString();
        }

        qDebug() << "Test Passed: Backend correctly handles app metadata.";
        app.exit(0);
    });

    backend.fetchApps("streaming");

    QTimer::singleShot(10000, &app, [&]() {
        qDebug() << "Test Timed Out";
        app.exit(1);
    });

    return app.exec();
}
