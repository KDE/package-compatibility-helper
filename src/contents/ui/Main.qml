// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.filotimoproject.appcompatibilityhelper

Kirigami.ApplicationWindow {
    id: root

    title: AppCompatibilityHelper.windowTitle

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
                    source: AppCompatibilityHelper.icon
                }

                ColumnLayout {
                    spacing: Kirigami.Units.largeSpacing
                    Layout.fillWidth: true

                    Kirigami.Heading {
                        id: heading
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: AppCompatibilityHelper.heading
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        Layout.maximumWidth: Math.max(Kirigami.Units.gridUnit * 30, heading.implicitWidth)
                        wrapMode: Text.WordWrap
                        text: AppCompatibilityHelper.description
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
                        AppCompatibilityHelper.openWithAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    id: compatibilityToolActionButton
                    visible: AppCompatibilityHelper.hasCompatibilityTool && !AppCompatibilityHelper.hasNativeApp
                    highlighted: !nativeAppActionButton.visible
                    icon.name: AppCompatibilityHelper.compatibilityToolActionIcon
                    text: AppCompatibilityHelper.compatibilityToolActionText
                    onClicked: {
                        AppCompatibilityHelper.compatibilityToolAction()
                        root.close()
                    }
                }

                QQC2.Button {
                    id: nativeAppActionButton
                    visible: AppCompatibilityHelper.hasNativeApp
                    highlighted: true
                    icon.name: AppCompatibilityHelper.nativeAppActionIcon
                    text: AppCompatibilityHelper.nativeAppActionText
                    onClicked: {
                        AppCompatibilityHelper.nativeAppAction()
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
