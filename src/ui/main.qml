import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 1280
    height: 720
    title: "KStore - WolfTech Innovations"

    readonly property color bgBase:      "#0d0d0f"
    readonly property color bgSurface:   "#18181c"
    readonly property color bgCard:      "#1e1e24"
    readonly property color bgCardHover: "#26262e"
    readonly property color accent:      "#ffffff"
    readonly property color accentDim:   "#44ffffff"
    readonly property color textPrimary: "#f0f0f0"
    readonly property color textMuted:   "#808090"
    readonly property color divider:     "#2a2a34"

    function decodeHtml(str) {
        if (!str) return "";
        return str
            .replace(/&amp;/g,  "&")
            .replace(/&lt;/g,   "<")
            .replace(/&gt;/g,   ">")
            .replace(/&quot;/g, "\"")
            .replace(/&#39;/g,  "'")
    }

    background: Rectangle { color: root.bgBase }

    globalDrawer: Kirigami.GlobalDrawer {
        title: "KStore"
        titleIcon: "applications-all"
        modal: false
        width: Kirigami.Units.gridUnit * 14
        background: Rectangle { color: root.bgSurface }

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

        Kirigami.Page {
            background: Rectangle { color: root.bgBase }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - Kirigami.Units.gridUnit * 4
                visible: backend.apps.length === 0 && !backend.loading
                icon.name: "applications-all"
                text: "No apps found"
                explanation: "Try a different category or check your connection."
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                visible: backend.loading
                text: "Fetching Apps..."
                explanation: "Please wait while we update the catalog."
                Kirigami.Action {
                    icon.name: "view-refresh"
                    text: "Stop"
                    onTriggered: backend.loading = false
                }
            }

            GridView {
                id: appsGrid
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit
                model: backend.apps
                visible: !backend.loading && backend.apps.length > 0

                readonly property int minCellW: Kirigami.Units.gridUnit * 18
                readonly property int cols: Math.max(1, Math.floor(width / minCellW))
                cellWidth:  Math.floor(width / cols)
                cellHeight: Kirigami.Units.gridUnit * 22

                clip: true
                focus: true

                delegate: Item {
                    id: delegateRoot
                    width:  appsGrid.cellWidth
                    height: appsGrid.cellHeight

                    readonly property bool isFocused: appsGrid.currentIndex === index
                    readonly property var status: backend.installationProgress[modelData.packageId] || {"installing": false, "progress": 0, "status": ""}
                    readonly property bool isInstalled: backend.installedApps.indexOf(modelData.packageId) !== -1

                    Rectangle {
                        id: cardBody
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.largeSpacing / 2
                        radius: 16
                        color: hoverHandler.hovered || delegateRoot.isFocused ? root.bgCardHover : root.bgCard
                        border.color: delegateRoot.isFocused ? root.accent : "transparent"
                        border.width: 2

                        scale: delegateRoot.isFocused ? 1.02 : 1.0
                        Behavior on scale { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.largeSpacing
                            spacing: Kirigami.Units.smallSpacing

                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 85
                                Layout.preferredHeight: 85
                                radius: 20
                                color: root.bgSurface

                                Image {
                                    id: appIcon
                                    anchors.centerIn: parent
                                    width: 64; height: 64
                                    source: modelData.icon || ""
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            Label {
                                text: root.decodeHtml(modelData.name)
                                font.bold: true
                                font.pixelSize: 18
                                color: root.textPrimary
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            Label {
                                text: delegateRoot.status.installing ? delegateRoot.status.status : (delegateRoot.isInstalled ? "Installed" : "Ready to Install")
                                font.pixelSize: 12
                                color: delegateRoot.isInstalled ? "#4CAF50" : root.textMuted
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                            }

                            ProgressBar {
                                Layout.fillWidth: true
                                visible: delegateRoot.status.installing
                                value: delegateRoot.status.progress
                                from: 0
                                to: 1
                            }

                            Item { Layout.fillHeight: true }

                            Button {
                                Layout.fillWidth: true
                                text: delegateRoot.isInstalled ? "Open" : (delegateRoot.status.installing ? "Installing..." : "Install")
                                enabled: !delegateRoot.status.installing
                                highlighted: delegateRoot.isFocused
                                icon.name: delegateRoot.isInstalled ? "media-playback-start" : "entry-edit"

                                onClicked: {
                                    if (delegateRoot.isInstalled) {
                                        backend.openApp(modelData.packageId)
                                    } else {
                                        backend.installApp(modelData.packageId, modelData.name, modelData.icon)
                                    }
                                }
                            }
                        }
                    }

                    HoverHandler { id: hoverHandler }
                    TapHandler {
                        onTapped: {
                            appsGrid.currentIndex = index
                            appsGrid.forceActiveFocus()
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
                "version": "1.1.0",
                "description": "Premium TV app store by WolfTech Innovations.\nNative experience for Android apps on Plasma Bigscreen.",
                "copyrightStatement": "Copyright (c) 2026 WolfTech Innovations",
                "authors": [{ "name": "WolfTech Innovations" }]
            }
        }
    }

    pageStack.initialPage: appsPage

    ProgressBar {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        indeterminate: true
        visible: backend.loading
    }

    Connections {
        target: backend
        function onInstallationFinished(packageId, success, message) {
            root.showPassiveNotification(message)
        }
    }

    Component.onCompleted: {
        backend.fetchApps("streaming")
        backend.hideWaydroidDesktops()
    }
}
