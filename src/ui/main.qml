import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 1280
    height: 720
    title: "KStore - WolfTech Innovations"

    globalDrawer: Kirigami.GlobalDrawer {
        title: "KStore"
        titleIcon: "view-app-grid"

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: Label {
                text: "WolfTech Innovations"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 20
            }
        }

        actions: [
            Kirigami.Action {
                text: "Streaming"
                icon.name: "video-display"
                onTriggered: {
                    backend.fetchApps("streaming")
                    pageStack.replace(appsPage)
                }
            },
            Kirigami.Action {
                text: "Remote Compatible"
                icon.name: "input-mouse"
                onTriggered: {
                    backend.fetchApps("remote control")
                    pageStack.replace(appsPage)
                }
            },
            Kirigami.Action {
                text: "About"
                icon.name: "help-about"
                onTriggered: pageStack.push(aboutPage)
            }
        ]
    }

    Component {
        id: appsPage
        Kirigami.ScrollablePage {
            title: "App Store"

            Kirigami.CardsGridView {
                model: backend.apps
                delegate: Kirigami.AbstractCard {
                    id: card
                    implicitWidth: 280
                    implicitHeight: 350

                    contentItem: Item {
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.largeSpacing
                            spacing: Kirigami.Units.mediumSpacing

                            Image {
                                Layout.preferredWidth: 128
                                Layout.preferredHeight: 128
                                Layout.alignment: Qt.AlignHCenter
                                source: modelData.icon || "application-x-executable"
                                fillMode: Image.PreserveAspectFit
                            }

                            Label {
                                text: modelData.name
                                font.bold: true
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                font.pixelSize: 18
                            }

                            Label {
                                text: modelData.packageId
                                font.pixelSize: 12
                                opacity: 0.6
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideMiddle
                            }

                            Button {
                                text: "Install"
                                Layout.alignment: Qt.AlignHCenter
                                Layout.fillWidth: true

                                onClicked: {
                                    backend.installApp(modelData.packageId)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: aboutPage
        Kirigami.AboutPage {
            title: "About KStore"
            aboutData: {
                "displayName": "KStore",
                "productName": "KStore",
                "version": "1.0.0",
                "description": "A premium TV app store for Plasma Bigscreen.\nFetch and install Android apps via Waydroid seamlessly.",
                "copyrightStatement": "Copyright (c) 2026 WolfTech Innovations",
                "authors": [
                    { "name": "WolfTech Innovations" }
                ],
                "licenses": [
                    { "name": "MIT License", "text": "MIT" }
                ]
            }
        }
    }

    pageStack.initialPage: appsPage

    footer: Kirigami.ActionToolBar {
        actions: [
            Kirigami.Action {
                text: "Refresh"
                icon.name: "view-refresh"
                onTriggered: backend.fetchApps()
            }
        ]
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        visible: backend.apps.length === 0 && !backend.loading
        text: "No apps found"
        explanation: "Try searching for something else or check your connection."
    }

    ProgressBar {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        indeterminate: true
        visible: backend.loading
    }

    Connections {
        target: backend
        function onInstallationStarted(packageId) {
            root.showPassiveNotification("Installation started for " + packageId)
        }
        function onInstallationFinished(packageId, success, message) {
            root.showPassiveNotification(message)
        }
    }

    Component.onCompleted: backend.fetchApps("streaming")
}
