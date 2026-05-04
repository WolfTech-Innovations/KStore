import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root
    width: 1280
    height: 720
    title: "KStore - WolfTech Innovations"

    // ── Palette ────────────────────────────────────────────────────────────
    readonly property color bgBase:      "#0d0d0f"
    readonly property color bgSurface:   "#18181c"
    readonly property color bgCard:      "#1e1e24"
    readonly property color bgCardHover: "#26262e"
    readonly property color accent:      "#ffffff"
    readonly property color accentDim:   "#44ffffff"
    readonly property color textPrimary: "#f0f0f0"
    readonly property color textMuted:   "#808090"
    readonly property color divider:     "#2a2a34"

    // FIX: helper to decode HTML entities from backend strings
    function decodeHtml(str) {
        return str
            .replace(/&amp;/g,  "&")
            .replace(/&lt;/g,   "<")
            .replace(/&gt;/g,   ">")
            .replace(/&quot;/g, "\"")
            .replace(/&#39;/g,  "'")
    }

    background: Rectangle { color: root.bgBase }

    // ── Sidebar ────────────────────────────────────────────────────────────
    globalDrawer: Kirigami.GlobalDrawer {
        title: ""
        titleIcon: ""
        modal: false
        width: Kirigami.Units.gridUnit * 14

        background: Rectangle { color: root.bgSurface }

        header: Item {
            width: parent.width
            height: Kirigami.Units.gridUnit * 5

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 4

                // FIX: use "applications-all" — more universally available in KF6 icon themes
                Kirigami.Icon {
                    source: "applications-all"
                    Layout.alignment: Qt.AlignHCenter
                    implicitWidth: 32
                    implicitHeight: 32
                    color: root.textPrimary
                }

                Label {
                    text: "KStore"
                    font.pixelSize: 20
                    font.bold: true
                    font.letterSpacing: 3
                    color: root.textPrimary
                    Layout.alignment: Qt.AlignHCenter
                }

                Label {
                    text: "WolfTech Innovations"
                    font.pixelSize: 10
                    color: root.textMuted
                    font.letterSpacing: 1
                    Layout.alignment: Qt.AlignHCenter
                }
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

    // ── Apps Page ──────────────────────────────────────────────────────────
    Component {
        id: appsPage

        Kirigami.Page {
            title: ""
            background: Rectangle { color: root.bgBase }

            padding: 0
            topPadding: Kirigami.Units.gridUnit * 1.5

            GridView {
                id: appsGrid
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit

                model: backend.apps

                readonly property int minCellW: Kirigami.Units.gridUnit * 17
                readonly property int cols: Math.max(1, Math.floor(width / minCellW))

                cellWidth:  Math.floor(width / cols)
                // FIX: bumped from 19 to 21 so Install button is never clipped
                cellHeight: Kirigami.Units.gridUnit * 21

                clip: true
                focus: true

                Keys.onLeftPressed:  if (currentIndex > 0) currentIndex--
                Keys.onRightPressed: if (currentIndex < count - 1) currentIndex++
                Keys.onUpPressed:    if (currentIndex >= cols) currentIndex -= cols
                Keys.onDownPressed:  if (currentIndex + cols < count) currentIndex += cols

                delegate: Item {
                    id: delegateRoot
                    width:  appsGrid.cellWidth
                    height: appsGrid.cellHeight

                    readonly property bool isFocused: appsGrid.currentIndex === index

                    // White selection outline
                    Rectangle {
                        anchors.fill: cardBody
                        anchors.margins: -2
                        radius: cardBody.radius + 2
                        color: "transparent"
                        border.color: root.accent
                        border.width: delegateRoot.isFocused || hoverHandler.hovered ? 2 : 0
                        opacity: delegateRoot.isFocused ? 1.0 : 0.5

                        Behavior on opacity      { NumberAnimation { duration: 120 } }
                        Behavior on border.width { NumberAnimation { duration: 80  } }
                    }

                    // Card body
                    Rectangle {
                        id: cardBody
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.largeSpacing / 2
                        radius: 16
                        color: hoverHandler.hovered ? root.bgCardHover : root.bgCard

                        Behavior on color { ColorAnimation { duration: 120 } }

                        // Subtle top highlight
                        Rectangle {
                            width: parent.width * 0.5
                            height: 1
                            anchors.top: parent.top
                            anchors.topMargin: 1
                            anchors.horizontalCenter: parent.horizontalCenter
                            radius: 1
                            color: root.accentDim
                            opacity: delegateRoot.isFocused ? 1.0 : 0.3
                        }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.largeSpacing
                            spacing: Kirigami.Units.smallSpacing

                            // Icon container
                            Rectangle {
                                Layout.alignment: Qt.AlignHCenter
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 80
                                radius: 18
                                color: root.bgSurface

                                Image {
                                    id: appIcon
                                    anchors.centerIn: parent
                                    width: 60
                                    height: 60
                                    source: modelData.icon || ""
                                    fillMode: Image.PreserveAspectFit
                                    visible: status === Image.Ready
                                }

                                Kirigami.Icon {
                                    anchors.centerIn: parent
                                    width: 40
                                    height: 40
                                    source: "application-x-executable"
                                    color: root.textMuted
                                    visible: appIcon.status !== Image.Ready
                                }
                            }

                            // FIX: decode HTML entities in app name
                            Label {
                                text: root.decodeHtml(modelData.name)
                                font.bold: true
                                font.pixelSize: 16
                                color: root.textPrimary
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            // Package ID (no entities expected but decode anyway)
                            Label {
                                text: root.decodeHtml(modelData.packageId)
                                font.pixelSize: 11
                                color: root.textMuted
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideMiddle
                            }

                            Item { Layout.fillHeight: true }

                            // Install button
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 38
                                radius: 10
                                color: installMouse.pressed
                                    ? Qt.rgba(1,1,1,0.15)
                                    : installMouse.containsMouse
                                        ? Qt.rgba(1,1,1,0.10)
                                        : Qt.rgba(1,1,1,0.06)
                                border.color: root.divider
                                border.width: 1

                                Behavior on color { ColorAnimation { duration: 100 } }

                                Label {
                                    anchors.centerIn: parent
                                    text: "Install"
                                    font.pixelSize: 13
                                    font.bold: true
                                    font.letterSpacing: 1
                                    color: root.textPrimary
                                }

                                MouseArea {
                                    id: installMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: backend.installApp(modelData.packageId)
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

            // Empty state
            Column {
                anchors.centerIn: parent
                spacing: Kirigami.Units.largeSpacing
                visible: backend.apps.length === 0 && !backend.loading

                Kirigami.Icon {
                    source: "applications-all"
                    width: 64; height: 64
                    color: root.textMuted
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label {
                    text: "No apps found"
                    font.pixelSize: 20
                    font.bold: true
                    color: root.textPrimary
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label {
                    text: "Try a different category or check your connection."
                    font.pixelSize: 13
                    color: root.textMuted
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // ── About Page ─────────────────────────────────────────────────────────
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

    // ── Footer ─────────────────────────────────────────────────────────────
    footer: Rectangle {
        width: parent.width
        height: 48
        color: root.bgSurface

        Rectangle {
            width: parent.width
            height: 1
            color: root.divider
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Kirigami.Units.largeSpacing
            anchors.rightMargin: Kirigami.Units.largeSpacing

            Label {
                text: "KSTORE"
                font.pixelSize: 11
                font.letterSpacing: 3
                font.bold: true
                color: root.textMuted
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 100
                height: 32
                radius: 8
                color: refreshMouse.pressed
                    ? Qt.rgba(1,1,1,0.12)
                    : refreshMouse.containsMouse
                        ? Qt.rgba(1,1,1,0.07)
                        : "transparent"
                border.color: root.divider
                border.width: 1

                Behavior on color { ColorAnimation { duration: 100 } }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Kirigami.Icon {
                        source: "view-refresh"
                        width: 14; height: 14
                        color: root.textMuted
                    }
                    Label {
                        text: "Refresh"
                        font.pixelSize: 12
                        color: root.textMuted
                    }
                }

                MouseArea {
                    id: refreshMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.fetchApps()
                }
            }
        }
    }

    // ── Loading bar ────────────────────────────────────────────────────────
    ProgressBar {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        indeterminate: true
        visible: backend.loading
    }

    // ── Notifications ──────────────────────────────────────────────────────
    Connections {
        target: backend
        function onInstallationStarted(packageId) {
            root.showPassiveNotification("Installing " + packageId + "…")
        }
        function onInstallationFinished(packageId, success, message) {
            root.showPassiveNotification(message)
        }
    }

    Component.onCompleted: backend.fetchApps("streaming")
}
