// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.packagecompatibilityhelper

Kirigami.ApplicationWindow {
    id: root

    title: PackageCompatibilityHelper.windowTitle

    flags: Qt.Dialog | Qt.WindowStaysOnTopHint

    controlsVisible: false
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    minimumWidth: pageStack.currentItem?.implicitWidth ?? 0
    minimumHeight: pageStack.currentItem?.implicitHeight ?? 0
    width: minimumWidth
    height: minimumHeight
    maximumWidth: width
    maximumHeight: height

    header: Kirigami.Separator {
        Layout.fillWidth: true
    }

    pageStack.initialPage: Kirigami.Page {
        id: mainPage

        padding: Kirigami.Units.largeSpacing

        implicitWidth: pageContent.implicitWidth + padding * 2
        implicitHeight: pageContent.implicitHeight + padding * 2

        ColumnLayout {
            id: pageContent
            spacing: Kirigami.Units.smallSpacing
            Layout.fillWidth: true

            RowLayout {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    id: icon
                    Layout.rightMargin: Kirigami.Units.largeSpacing * 2
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large * 2
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large * 2
                    Layout.alignment: Qt.AlignCenter
                    source: PackageCompatibilityHelper.icon
                }

                ColumnLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Kirigami.Heading {
                        id: heading
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: PackageCompatibilityHelper.heading
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.max(Kirigami.Units.gridUnit * 30, heading.implicitWidth)
                        wrapMode: Text.WordWrap
                        text: PackageCompatibilityHelper.description
                    }
                }
            }

            RowLayout {
                id: actionButtons
                Layout.alignment: Qt.AlignRight
                Layout.fillWidth: true

                QQC2.Button {
                    icon.name: "system-run-symbolic"
                    text: i18n("Open With…")
                    onClicked: {
                        PackageCompatibilityHelper.openWithAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    id: compatibilityToolActionButton
                    visible: PackageCompatibilityHelper.hasCompatibilityTool && !PackageCompatibilityHelper.hasNativeApp
                    highlighted: !nativeAppActionButton.visible
                    icon.name: PackageCompatibilityHelper.compatibilityToolActionIcon
                    text: PackageCompatibilityHelper.compatibilityToolActionText
                    onClicked: {
                        PackageCompatibilityHelper.compatibilityToolAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    id: nativeAppActionButton
                    visible: PackageCompatibilityHelper.hasNativeApp
                    highlighted: true
                    icon.name: PackageCompatibilityHelper.nativeAppActionIcon
                    text: PackageCompatibilityHelper.nativeAppActionText
                    onClicked: {
                        PackageCompatibilityHelper.nativeAppAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    id: documentationActionButton
                    visible: !PackageCompatibilityHelper.hasCompatibilityTool && !PackageCompatibilityHelper.hasNativeApp
                    highlighted: !nativeAppActionButton.visible && !compatibilityToolActionButton.visible
                    icon.name: PackageCompatibilityHelper.documentationActionIcon
                    text: PackageCompatibilityHelper.documentationActionText
                    onClicked: {
                        PackageCompatibilityHelper.documentationAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    icon.name: "dialog-cancel"
                    text: i18n("Cancel")
                    onClicked: root.close()
                }
            }
        }
    }
}
