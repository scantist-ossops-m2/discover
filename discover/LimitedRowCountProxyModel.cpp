/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "LimitedRowCountProxyModel.h"

#include <QTimer>

using namespace std::chrono_literals;

class LimitedRowCountProxyModel::LimitedRowCountProxyModelPrivate
{
public:
    int m_page = 0;
    int m_pageSize = 1;
    bool m_connected = false;
    QTimer m_invalidateTimer;
};

LimitedRowCountProxyModel::LimitedRowCountProxyModel(QObject *object)
    : QSortFilterProxyModel(object)
    , d(new LimitedRowCountProxyModelPrivate)
{
    // Compress events to avoid excessive filter runs
    d->m_invalidateTimer.setInterval(250ms);
    d->m_invalidateTimer.setSingleShot(true);
    connect(&d->m_invalidateTimer, &QTimer::timeout, this, &LimitedRowCountProxyModel::invalidateRowsFilter);

    connect(this, &QSortFilterProxyModel::sourceModelChanged, this, [this] {
        if (!sourceModel()) {
            return;
        }

        // Only support running once to not have to implement disconnecting.
        Q_ASSERT(!d->m_connected);
        if (d->m_connected) {
            return;
        }
        d->m_connected = true;

        connect(sourceModel(), &QAbstractItemModel::rowsInserted, &d->m_invalidateTimer, QOverload<>::of(&QTimer::start));
        connect(sourceModel(), &QAbstractItemModel::rowsRemoved, &d->m_invalidateTimer, QOverload<>::of(&QTimer::start));
        connect(sourceModel(), &QAbstractItemModel::modelReset, &d->m_invalidateTimer, QOverload<>::of(&QTimer::start));
        invalidateRowsFilter();
    });
}

LimitedRowCountProxyModel::~LimitedRowCountProxyModel() = default;

int LimitedRowCountProxyModel::pageSize() const
{
    return d->m_pageSize;
}

void LimitedRowCountProxyModel::setPageSize(int count)
{
    if (count == d->m_pageSize) {
        return;
    }

    d->m_pageSize = count;
    Q_EMIT pageSizeChanged();

    invalidateRowsFilter();
}

bool LimitedRowCountProxyModel::filterAcceptsRow(int source_row, [[maybe_unused]] const QModelIndex &source_parent) const
{
    return source_row >= 0 && source_row < d->m_pageSize;
}
