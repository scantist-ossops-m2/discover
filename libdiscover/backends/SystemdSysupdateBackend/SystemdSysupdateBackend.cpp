/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateBackend.h"
#include <resources/AbstractResourcesBackend.h>

DISCOVER_BACKEND_PLUGIN(SystemdSysupdateBackend)

SystemdSysupdateBackend::SystemdSysupdateBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
{
    m_updater->addResources({});
}

int SystemdSysupdateBackend::updatesCount() const
{
    return 0;
}

#include "SystemdSysupdateBackend.moc"