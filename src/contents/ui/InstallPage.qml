// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page

    required property var installer
    property bool prepareOnCompleted: false

    // "confirm" | "progress" | "error" | "cancelled"
    property string phase: "confirm"

    signal declined()
    signal succeeded()

    padding: Kirigami.Units.largeSpacing
    implicitWidth: Math.max(pageContent.implicitWidth, Kirigami.Units.gridUnit * 28) + padding * 2
    implicitHeight: pageContent.implicitHeight + padding * 2

    Component.onCompleted: {
        if (page.prepareOnCompleted) {
            page.installer.prepare()
        }
    }

    Connections {
        target: page.installer
        function onFinished(success) {
            if (success) {
                page.succeeded()
            } else if (page.installer.cancelled) {
                page.phase = "cancelled"
            } else {
                page.phase = "error"
            }
        }
    }

    ColumnLayout {
        id: pageContent
        anchors.fill: parent
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        RowLayout {
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing

            Kirigami.Icon {
                Layout.rightMargin: Kirigami.Units.largeSpacing * 2
                Layout.preferredWidth: Kirigami.Units.iconSizes.large * 2
                Layout.preferredHeight: Kirigami.Units.iconSizes.large * 2
                Layout.alignment: Qt.AlignCenter
                source: page.phase === "error" ? "dialog-error" : page.installer.icon
            }

            ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                Layout.fillWidth: true

                Kirigami.Heading {
                    id: heading
                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                    wrapMode: Text.WordWrap
                    text: {
                        switch (page.phase) {
                        case "progress":
                            return i18nc("@title %1 is an application name", "Installing %1", page.installer.displayName)
                        case "error":
                            return i18nc("@title", "Installation Failed")
                        case "cancelled":
                            return i18nc("@title", "Installation Cancelled")
                        default:
                            return i18nc("@title %1 is an application name", "Install %1?", page.installer.displayName)
                        }
                    }
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                    wrapMode: Text.WordWrap
                    text: {
                        switch (page.phase) {
                        case "progress":
                            return i18nc("@info:progress %1 is an application name", "Downloading and installing %1…", page.installer.displayName)
                        case "error":
                            return page.installer.errorText !== ""
                                   ? page.installer.errorText
                                   : i18nc("@info %1 is an application name", "%1 could not be installed.", page.installer.displayName)
                        case "cancelled":
                            return i18nc("@info %1 is an application name", "The installation of %1 was cancelled.", page.installer.displayName)
                        default:
                            return i18nc("@info %1 is an application name", "This software requires %1, which is not installed.", page.installer.displayName)
                        }
                    }
                }

                QQC2.Label {
                    visible: page.phase === "confirm"
                    Layout.fillWidth: true
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 22
                    wrapMode: Text.WordWrap
                    opacity: 0.75
                    text: page.installer.sizesKnown
                          ? i18nc("@info %1 is a Flatpak application id, %2 a Flatpak remote name, %3 a download size, %4 a size on disk",
                                  "The Flatpak %1 will be downloaded from “%2” (%3 to download, %4 on disk).",
                                  page.installer.appId, page.installer.remoteName,
                                  page.installer.downloadSizeText, page.installer.installedSizeText)
                          : i18nc("@info %1 is a Flatpak application id, %2 a Flatpak remote name",
                                  "The Flatpak %1 will be downloaded from “%2”. Determining its size…",
                                  page.installer.appId, page.installer.remoteName)
                }

                QQC2.ProgressBar {
                    visible: page.phase === "progress"
                    from: 0
                    to: 100
                    value: page.installer.progress
                    Layout.fillWidth: true
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 18
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight
            Layout.fillWidth: true

            QQC2.Button {
                visible: page.phase === "confirm"
                highlighted: true
                icon.name: "download-symbolic"
                text: i18nc("@action:button Start the installation", "Install")
                onClicked: {
                    page.phase = "progress"
                    page.installer.start()
                }
            }

            QQC2.Button {
                visible: page.phase === "confirm"
                icon.name: "dialog-cancel"
                text: i18nc("@action:button Dismiss the installation prompt", "Cancel")
                onClicked: {
                    page.installer.discardPreparation()
                    page.declined()
                }
            }

            QQC2.Button {
                visible: page.phase === "progress"
                icon.name: "dialog-cancel"
                text: i18nc("@action:button Abort the running installation", "Cancel")
                onClicked: page.installer.cancel()
            }

            QQC2.Button {
                visible: page.phase === "error" || page.phase === "cancelled"
                icon.name: "dialog-close"
                text: i18nc("@action:button Dismiss the error", "Close")
                onClicked: {
                    page.installer.discardPreparation()
                    page.declined()
                }
            }
        }
    }
}
