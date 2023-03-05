/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */


import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import org.kde.discover 2.0
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.components 1.0 as Components

ListView {
    id: root

    readonly property alias count: screenshotsModel.count
    property bool showNavigationArrows: true
    property alias resource: screenshotsModel.application
    property var resource
    property int failedCount: 0
    readonly property bool hasFailed: count !== 0 && failedCount === count

    spacing: Kirigami.Units.largeSpacing
    focus: overlay.visible
    orientation: Qt.Horizontal
    cacheBuffer: 10 // keep some screenshots in memory

    Keys.onLeftPressed:  if (leftAction.visible)  leftAction.trigger()
    Keys.onRightPressed: if (rightAction.visible) rightAction.trigger()

    model: ScreenshotsModel {
        id: screenshotsModel
    }

    property real delegateHeight: Kirigami.Units.gridUnit * 4

    delegate: AbstractButton {
        readonly property bool animated: isAnimated
        readonly property url imageSource: source
        readonly property real proportion: (thumbnail.status === Image.Ready && thumbnail.sourceSize.width > 1)
            ? (thumbnail.sourceSize.height / thumbnail.sourceSize.width) : 1

        implicitWidth: root.delegateHeight / proportion
        implicitHeight: root.delegateHeight
        opacity: hovered ? 0.7 : 1

        hoverEnabled: true
        onClicked: {
            root.currentIndex = model.row
            overlay.open()
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
        }

        background: Item {
            BusyIndicator {
                visible: running
                running: thumbnail.status === Image.Loading
                anchors.centerIn: parent
            }
            Kirigami.Icon {
                anchors.fill: parent
                anchors.margins: Kirigami.Units.gridUnit
                visible: thumbnail.status === Image.Error
                source: "emblem-error"
            }
            ConditionalLoader {
                id: thumbnail
                anchors.fill: parent
                readonly property var status: item.status
                readonly property var sourceSize: item.sourceSize
                condition: isAnimated

                componentFalse: Component {
                    Image {
                        source: tempSource
                    }
                }
                componentTrue: Component {
                    AnimatedImage {
                        source: tempSource
                    }
                }

                onStatusChanged: {
                    if (status === Image.Error) {
                        root.failedCount += 1;
                    }
                }
            }
        }
    }

    Components.MaximizeComponent {
        id: overlay
        itemModel: screenshotsModel
        initialIndex: root.currentIndex

        // leader: RowLayout {
        //     Kirigami.Avatar {
        //         id: userAvatar
        //         implicitWidth: Kirigami.Units.iconSizes.medium
        //         implicitHeight: Kirigami.Units.iconSizes.medium
        //
        //         name: model.author.name ?? model.author.displayName
        //         source: model.author.avatarMediaId ? ("image://mxc/" + model.author.avatarMediaId) : ""
        //         color: model.author.color
        //     }
        //     ColumnLayout {
        //         spacing: 0
        //         QQC2.Label {
        //             id: userLabel
        //             text: model.author.name ?? model.author.displayName
        //             color: model.author.color
        //             font.weight: Font.Bold
        //             elide: Text.ElideRight
        //         }
        //         QQC2.Label {
        //             id: dateTimeLabel
        //             text: model.time.toLocaleString(Qt.locale(), Locale.ShortFormat)
        //             color: Kirigami.Theme.disabledTextColor
        //             elide: Text.ElideRight
        //         }
        //     }
        // }
        //
        // onItemRightClicked: {
        //     const contextMenu = fileDelegateContextMenu.createObject(parent, {
        //         author: model.author,
        //         message: model.message,
        //         eventId: model.eventId,
        //         source: model.source,
        //         file: parent,
        //         mimeType: model.mimeType,
        //         progressInfo: model.progressInfo,
        //         plainMessage: model.message,
        //     });
        //     contextMenu.closeFullscreen.connect(root.close)
        //     contextMenu.open();
        // }
        // onSaveItem: {
        //     var dialog = saveAsDialog.createObject(QQC2.ApplicationWindow.overlay)
        //     dialog.open()
        //     dialog.currentFile = dialog.folder + "/" + currentRoom.fileNameToDownload(model.eventId)
        // }
    }
}
