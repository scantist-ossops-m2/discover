/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ScreenshotsModel.h"
#include "libdiscover_debug.h"
#include "utils.h"
#include <resources/AbstractResource.h>
// #include <QAbstractItemModelTester>

ScreenshotsModel::ScreenshotsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_resource(nullptr)
{
}

QHash<int, QByteArray> ScreenshotsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(ThumbnailUrl, "tempSource");
    roles.insert(ScreenshotUrl, "source");
    roles.insert(IsAnimatedRole, "isAnimated");
    roles.insert(TypeRole, "type");
    roles.insert(CaptionRole, "caption");
    return roles;
}

void ScreenshotsModel::setResource(AbstractResource *res)
{
    if (res == m_resource)
        return;

    if (m_resource) {
        disconnect(m_resource, &AbstractResource::screenshotsFetched, this, &ScreenshotsModel::screenshotsFetched);
    }
    m_resource = res;
    Q_EMIT resourceChanged(res);

    beginResetModel();
    m_screenshots.clear();
    endResetModel();

    if (res) {
        connect(m_resource, &AbstractResource::screenshotsFetched, this, &ScreenshotsModel::screenshotsFetched);
        res->fetchScreenshots();
    } else
        qCWarning(LIBDISCOVER_LOG) << "empty resource!";
}

AbstractResource *ScreenshotsModel::resource() const
{
    return m_resource;
}

void ScreenshotsModel::screenshotsFetched(const Screenshots &screenshots)
{
    if (screenshots.isEmpty())
        return;

    beginInsertRows(QModelIndex(), m_screenshots.size(), m_screenshots.size() + screenshots.size() - 1);
    m_screenshots += screenshots;
    endInsertRows();
    Q_EMIT countChanged();
}

QVariant ScreenshotsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.parent().isValid())
        return QVariant();

    switch (role) {
    case ThumbnailUrl:
        return m_screenshots[index.row()].thumbnail;
    case ScreenshotUrl:
        return m_screenshots[index.row()].screenshot;
    case IsAnimatedRole:
        return m_screenshots[index.row()].isAnimated;
    case TypeRole:
        // 0 is image from AlbumModelItem.Type
        return 0;
    case CaptionRole:
        return m_screenshots[index.row()].screenshot.fileName();
    }

    return QVariant();
}

int ScreenshotsModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_screenshots.count() : 0;
}

QUrl ScreenshotsModel::screenshotAt(int row) const
{
    return m_screenshots[row].screenshot;
}

int ScreenshotsModel::count() const
{
    return m_screenshots.count();
}

void ScreenshotsModel::remove(const QUrl &url)
{
    int idxRemove = kIndexOf(m_screenshots, [url](const Screenshot &s) {
        return s.thumbnail == url || s.screenshot == url;
    });
    if (idxRemove >= 0) {
        beginRemoveRows({}, idxRemove, idxRemove);
        m_screenshots.removeAt(idxRemove);
        endRemoveRows();
        Q_EMIT countChanged();

        qDebug() << "screenshot removed" << url;
    }
}

#include "moc_ScreenshotsModel.cpp"
