#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <KAboutData>
#include <KLocalizedString>
#include "kstorebackend.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KAboutData aboutData(
        QStringLiteral("kstore"),
        i18n("KStore"),
        QStringLiteral("1.0.0"),
        i18n("A simple TV app store for Plasma Bigscreen."),
        KAboutLicense::Custom,
        i18n("(c) 2026 WolfTech Innovations")
    );
    aboutData.setLicenseText(i18n("MIT License"));
    aboutData.addAuthor(i18n("WolfTech Innovations"));
    KAboutData::setApplicationData(aboutData);

    KStoreBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);

    const QUrl url(QStringLiteral("qrc:/ui/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
