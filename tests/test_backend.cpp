#include "../src/kstorebackend.h"
#include <QCoreApplication>
#include <QDebug>
#include <cassert>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KStoreBackend backend;

    QVariantList apps = backend.apps();
    qDebug() << "Number of apps fetched:" << apps.length();

    // Check if we have at least some apps (mock data)
    assert(apps.length() > 0);

    bool foundStreaming = false;
    bool foundRemote = false;

    for (const auto &appVar : apps) {
        QVariantMap map = appVar.toMap();
        QString category = map["category"].toString();
        if (category == "Streaming") foundStreaming = true;
        if (category == "Remote Compatible") foundRemote = true;
        qDebug() << "App:" << map["name"].toString() << "Category:" << category;
    }

    assert(foundStreaming);
    assert(foundRemote);

    qDebug() << "Test Passed: Backend filters and provides app data correctly.";

    return 0;
}
