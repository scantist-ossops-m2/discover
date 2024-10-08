/*
 *   SPDX-FileCopyrightText: 2024 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateBackend.h"
#include "Category/Category.h"
#include "resources/AbstractResource.h"
#include <qcollator.h>
#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qlist.h>
#include <qobject.h>
#include <resources/AbstractResourcesBackend.h>

DISCOVER_BACKEND_PLUGIN(SystemdSysupdateBackend)

SystemdSysupdateBackend::SystemdSysupdateBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
{
}

int SystemdSysupdateBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool SystemdSysupdateBackend::isValid() const
{
    // FIXME: check if the dbus interface exists
    return true;
}

ResultsStream *SystemdSysupdateBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    // Skip the search if we're looking into a Category, but not the "Operating System" category
    if (filter.category && filter.category->untranslatedName() != QLatin1String("Operating System")) {
        return new ResultsStream(QStringLiteral("rpm-ostree-empty"), {});
    }

    // Trim whitespace from beginning and end of the string entered in the search field.
    QString keyword = filter.search.trimmed();

    QVector<StreamResult> res;
    for (AbstractResource *r : m_updater) {
        // Skip if the state does not match the filter
        if (r->state() < filter.state) {
            continue;
        }
        // Skip if the search field is not empty and neither the name, description or version matches
        if (!keyword.isEmpty()) {
            if (!r->name().contains(keyword) && !r->longDescription().contains(keyword) && !r->installedVersion().contains(keyword)) {
                continue;
            }
        }
        // Add the ressources to the search filter
        res << r;
    }
    return new ResultsStream(QStringLiteral("systemd-sysupdate"), res);
}

#include "SystemdSysupdateBackend.moc"