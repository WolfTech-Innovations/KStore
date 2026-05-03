#include "../src/kstorebackend.h"
#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KStoreBackend backend;

    backend.fetchApps("streaming");

    // Test Streaming
    {
        QEventLoop loop;
        QObject::connect(&backend, &KStoreBackend::appsChanged, [&]() {
            if (!backend.apps().isEmpty() && !backend.loading()) loop.quit();
        });
        QTimer::singleShot(20000, &loop, &QEventLoop::quit);
        loop.exec();
    }

    QVariantList apps = backend.apps();
    qDebug() << "Number of streaming apps fetched:" << apps.length();
    assert(apps.length() > 0);

    bool foundStreaming = false;
    for (const auto &appVar : apps) {
        if (appVar.toMap()["category"].toString() == "Streaming") foundStreaming = true;
    }
    assert(foundStreaming);

    // Test Remote Compatible
    backend.fetchApps("remote control");
    {
        QEventLoop loop;
        QObject::connect(&backend, &KStoreBackend::appsChanged, [&]() {
            bool found = false;
            for (const auto &appVar : backend.apps()) {
                if (appVar.toMap()["category"].toString() == "Remote Compatible") {
                        found = true;
                }
            }
            // Only quit if we have EXACTLY the apps we expect for this query
            // (Wait until we see at least one Remote Compatible app)
            if (found && !backend.loading()) loop.quit();
        });
        QTimer::singleShot(20000, &loop, &QEventLoop::quit);
        loop.exec();
    }

    apps = backend.apps();
    qDebug() << "Number of remote apps fetched:" << apps.length();
    assert(apps.length() > 0);

    bool foundRemote = false;
    for (const auto &appVar : apps) {
        qDebug() << "Checking app:" << appVar.toMap()["name"].toString() << "category:" << appVar.toMap()["category"].toString();
        if (appVar.toMap()["category"].toString() == "Remote Compatible") foundRemote = true;
    }
    assert(foundRemote);

    qDebug() << "Test Passed: Backend filters and provides app data correctly.";

    return 0;
}
