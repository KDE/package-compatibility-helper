// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.packagecompatibilityhelper

Kirigami.ApplicationWindow {
    id: root

    readonly property real fixedWidth: Math.max(mainPage.implicitWidth, installPageMetrics.item?.implicitWidth ?? 0)
    readonly property real fixedHeight: Math.max(mainPage.implicitHeight, installPageMetrics.item?.implicitHeight ?? 0) + headerSeparator.implicitHeight

    title: PackageCompatibilityHelper.windowTitle
    flags: Qt.Dialog | Qt.WindowStaysOnTopHint
    controlsVisible: false
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.defaultColumnWidth: fixedWidth

    minimumWidth: fixedWidth
    maximumWidth: fixedWidth
    width: fixedWidth
    minimumHeight: fixedHeight
    maximumHeight: fixedHeight
    height: fixedHeight

    header: Kirigami.Separator {
        id: headerSeparator
        Layout.fillWidth: true
    }

    Component {
        id: installPageComponent

        InstallPage {
            installer: PackageCompatibilityHelper.compatibilityToolInstaller
            onDeclined: {
                if (PackageCompatibilityHelper.installOnly) {
                    installer.decline()
                } else {
                    root.pageStack.pop()
                }
            }
            onSucceeded: {
                if (PackageCompatibilityHelper.installOnly) {
                    installer.completeSuccess()
                } else {
                    PackageCompatibilityHelper.compatibilityToolInstallFinished()
                    root.close()
                }
            }
        }
    }

    Component {
        id: secondaryInstallPageComponent

        InstallPage {
            installer: PackageCompatibilityHelper.secondaryCompatibilityToolInstaller
            onDeclined: root.pageStack.pop()
            onSucceeded: {
                PackageCompatibilityHelper.secondaryCompatibilityToolInstallFinished()
                root.close()
            }
        }
    }

    Component {
        id: installPageMetricsComponent

        InstallPage {
            installer: PackageCompatibilityHelper.compatibilityToolInstaller
        }
    }

    // Measures the dimensions of the install page without triggering it.
    Loader {
        id: installPageMetrics
        visible: false
        sourceComponent: installPageMetricsComponent
    }

    pageStack.initialPage: Kirigami.Page {
        id: mainPage
        padding: Kirigami.Units.largeSpacing
        implicitWidth: pageContent.implicitWidth + padding * 2
        implicitHeight: pageContent.implicitHeight + padding * 2

        ColumnLayout {
            id: pageContent
            anchors.fill: parent
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            RowLayout {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.margins: Kirigami.Units.largeSpacing
                Layout.fillWidth: true

                Kirigami.Icon {
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

            Item {
                Layout.fillHeight: true
            }

            RowLayout {
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
                        if (PackageCompatibilityHelper.compatibilityToolInstalled) {
                            PackageCompatibilityHelper.launchCompatibilityTool()
                            root.close()
                        } else {
                            root.pageStack.push(installPageComponent, { prepareOnCompleted: true })
                        }
                    }
                }

                QQC2.Button {
                    id: secondaryCompatibilityToolActionButton
                    visible: PackageCompatibilityHelper.hasSecondaryCompatibilityTool && !PackageCompatibilityHelper.hasNativeApp
                    icon.name: PackageCompatibilityHelper.secondaryCompatibilityToolActionIcon
                    text: PackageCompatibilityHelper.secondaryCompatibilityToolActionText
                    onClicked: {
                        if (PackageCompatibilityHelper.secondaryCompatibilityToolInstalled) {
                            PackageCompatibilityHelper.launchSecondaryCompatibilityTool()
                            root.close()
                        } else {
                            root.pageStack.push(secondaryInstallPageComponent, { prepareOnCompleted: true })
                        }
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

    Component.onCompleted: {
        if (PackageCompatibilityHelper.installOnly) {
            pageStack.replace(installPageComponent, { prepareOnCompleted: true })
        }
    }
}
