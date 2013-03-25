/***************************************************************************
 *   Copyright © 2011 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "UpdateModel.h"

// Qt includes
#include <QtGui/QFont>
#include <QApplication>

// KDE includes
#include <KGlobal>
#include <KIconLoader>
#include <KLocale>
#include <KDebug>

// Own includes
#include "UpdateItem.h"
#include <resources/AbstractResource.h>
#include <resources/ResourcesUpdatesModel.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/AbstractBackendUpdater.h>

#define ICON_SIZE KIconLoader::SizeSmallMedium


UpdateModel::UpdateModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_updates(nullptr)
{
    m_rootItem = new UpdateItem();
}

UpdateModel::~UpdateModel()
{
    delete m_rootItem;
}

static bool isMarked(AbstractResource* res)
{
    return res->backend()->backendUpdater()->toUpdate().contains(res);
}

QVariant UpdateModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    UpdateItem *item = static_cast<UpdateItem*>(index.internalPointer());
    int column = index.column();

    switch (role) {
    case Qt::DisplayRole:
        switch (column) {
        case NameColumn:
            return item->name();
        case VersionColumn:
            return item->version();
        case SizeColumn:
            return KGlobal::locale()->formatByteSize(item->size());
        }
        break;
    case Qt::DecorationRole:
        if (column == NameColumn) {
            return item->icon().pixmap(ICON_SIZE, ICON_SIZE);
        }
        break;
    case Qt::FontRole: {
        QFont font;
        if ((item->type() == UpdateItem::ItemType::CategoryItem) && column == SizeColumn) {
            font.setBold(true);
            return font;
        }
        return font;
    }
    case Qt::CheckStateRole:
        if (column == NameColumn) {
            if(item->type() == UpdateItem::ItemType::CategoryItem) {
                int checkedCount = 0;
                foreach(UpdateItem* child, item->children()) {
                    checkedCount += isMarked(child->app());
                }
                return checkedCount==0 ? Qt::Unchecked : 
                       checkedCount==item->childCount() ? Qt::Checked : Qt::PartiallyChecked;
            } else {
                return isMarked(item->app()) ? Qt::Checked : Qt::Unchecked;
            }
        }
        break;
    default:
        break;
    }

    return QVariant();
}

void UpdateModel::checkResources(const QList<AbstractResource*>& resource, bool checked)
{
    if (resource.size() > 1) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
    if(checked)
        m_updates->addResources(resource);
    else
        m_updates->removeResources(resource);
    QApplication::restoreOverrideCursor();
}

QVariant UpdateModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
        case NameColumn:
            return i18nc("@label Column label", "Updates");
        case VersionColumn:
            return i18nc("@label Column label", "Version");
        case SizeColumn:
            return i18nc("@label Column label", "Download Size");
        }
    }

    return QVariant();
}

Qt::ItemFlags UpdateModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex UpdateModel::index(int row, int column, const QModelIndex &index) const
{
    // Bounds checks
    if (!m_rootItem || row < 0 || column < 0 || column > 3 ||
        (index.isValid() && index.column() != 0)) {
        return QModelIndex();
    }

    if (UpdateItem *parent = itemFromIndex(index)) {
        if (UpdateItem *childItem = parent->child(row))
            return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex UpdateModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    UpdateItem *childItem = itemFromIndex(index);
    UpdateItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int UpdateModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return 0;

    UpdateItem *parentItem = itemFromIndex(parent);

    return parentItem->childCount();
}

int UpdateModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

void UpdateModel::clear()
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = new UpdateItem();
    endResetModel();
}

bool UpdateModel::removeItem(const QModelIndex &index)
{
    QModelIndexList indexes;
    if (rowCount(index) > 0) {
        indexes = collectItems(index);
    }
    indexes.append(index);

    foreach (const QModelIndex &itemToRemove, indexes) {
        if (!removeRow(itemToRemove.row(), itemToRemove.parent()))
            return false;
    }
    return true;
}

bool UpdateModel::removeRows(int position, int rows, const QModelIndex &index)
{
    if (!m_rootItem)
        return false;

    UpdateItem *item = index.isValid() ? itemFromIndex(index)
                                       : m_rootItem;
    beginRemoveRows(index, position, position + rows - 1);
    item->removeChildren(position, rows);
    endRemoveRows();

    return true;
}

QModelIndexList UpdateModel::collectItems(const QModelIndex &parent) const
{
    QModelIndexList list;
    for (int i = rowCount(parent) - 1; i >= 0 ; --i) {
        const QModelIndex &next = index(i, 0, parent);
        list += collectItems(next);
        list.append(next);
    }
    return list;
}

UpdateItem* UpdateModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
         return static_cast<UpdateItem*>(index.internalPointer());
    return m_rootItem;
}

void UpdateModel::addItem(UpdateItem *item)
{
    beginInsertRows(QModelIndex(), m_rootItem->childCount(), m_rootItem->childCount());
    m_rootItem->appendChild(item);
    endInsertRows();
}

bool UpdateModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        UpdateItem *item = static_cast<UpdateItem*>(idx.internalPointer());
        bool newValue = value.toBool();
        UpdateItem::ItemType type = item->type();

        QList<AbstractResource *> apps;
        if (type == UpdateItem::ItemType::CategoryItem) {
            // Collect items to (un)check
            foreach (UpdateItem *child, item->children()) {
                apps << child->app();
            }
        } else if (type == UpdateItem::ItemType::ApplicationItem) {
            apps << item->app();
        }

        checkResources(apps, newValue);
        dataChanged(idx, idx);
        if (type == UpdateItem::ItemType::ApplicationItem) {
            QModelIndex parentIndex = idx.parent();
            emit dataChanged(parentIndex, parentIndex);
        } else {
            emit dataChanged(index(0,0, idx), index(item->childCount()-1, 0, idx));
        }

        return true;
    }

    return false;
}

void UpdateModel::packageChanged()
{
    // We don't know what changed or not, so say everything changed
    emit dataChanged(index(0, 0), QModelIndex());
}

void UpdateModel::addResources(const QList< AbstractResource* >& resources)
{
    UpdateItem *securityItem = new UpdateItem(i18nc("@item:inlistbox", "Important Security Updates"),
                                              KIcon("security-medium"));

    UpdateItem *appItem = new UpdateItem(i18nc("@item:inlistbox", "Application Updates"),
                                          KIcon("applications-other"));

    UpdateItem *systemItem = new UpdateItem(i18nc("@item:inlistbox", "System Updates"),
                                             KIcon("applications-system"));

    foreach(AbstractResource* res, resources) {
        UpdateItem *updateItem = new UpdateItem(res);
        if (res->isFromSecureOrigin()) {
            securityItem->appendChild(updateItem);
        } else if(!res->isTechnical()) {
            appItem->appendChild(updateItem);
        } else {
            systemItem->appendChild(updateItem);
        }
    }

    // Add populated items to the model
    if (securityItem->childCount()) {
        securityItem->sort();
        addItem(securityItem);
    } else {
        delete securityItem;
    }

    if (appItem->childCount()) {
        appItem->sort();
        addItem(appItem);
    } else {
        delete appItem;
    }

    if (systemItem->childCount()) {
        systemItem->sort();
        addItem(systemItem);
    } else {
        delete systemItem;
    }
}

void UpdateModel::setBackend(ResourcesUpdatesModel* updates)
{
    m_updates = updates;
}
