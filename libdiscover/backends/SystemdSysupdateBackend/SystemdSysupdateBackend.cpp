/*
 *   SPDX-FileCopyrightText: 2024 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateBackend.h"

#include "resources/AbstractResource.h"
#include <qlist.h>
#include <qtpreprocessorsupport.h>
#include <resources/AbstractResourcesBackend.h>

DISCOVER_BACKEND_PLUGIN(SystemdSysupdateBackend)

const auto service = QStringLiteral("org.freedesktop.sysupdate1");
const auto path = QStringLiteral("/org/freedesktop/sysupdate1");

SystemdSysupdateBackend::SystemdSysupdateBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_manager(new org::freedesktop::sysupdate1::Manager(service, path, QDBusConnection::systemBus(), this))
{
    qDBusRegisterMetaType<Sysupdate::Target>();
    qDBusRegisterMetaType<Sysupdate::TargetList>();
}

int SystemdSysupdateBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool SystemdSysupdateBackend::isValid() const
{
    // Try waking up the service
    org::freedesktop::DBus::Peer(service, path, QDBusConnection::systemBus()).Ping().waitForFinished();

    return m_manager->isValid();
}

ResultsStream *SystemdSysupdateBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    Q_UNUSED(filter);
    return new ResultsStream(QStringLiteral("systemd-sysupdate"), QVector<StreamResult>());
}

AbstractBackendUpdater *SystemdSysupdateBackend::backendUpdater() const
{
    return m_updater;
}

AbstractReviewsBackend *SystemdSysupdateBackend::reviewsBackend() const
{
    return nullptr;
}

Transaction *SystemdSysupdateBackend::installApplication(AbstractResource *app)
{
    Q_UNUSED(app);
    return nullptr;
}

Transaction *SystemdSysupdateBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(app);
    Q_UNUSED(addons);
    return nullptr;
}

Transaction *SystemdSysupdateBackend::removeApplication(AbstractResource *app)
{
    Q_UNUSED(app);
    return nullptr;
}

bool SystemdSysupdateBackend::isFetching() const
{
    return m_fetchOperationCount > 0;
}

void SystemdSysupdateBackend::checkForUpdates()
{
    qDebug() << "Updating systemd-sysupdate backend...";
    auto reply = m_manager->ListTargets();
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        call->deleteLater();

        auto watcher = QDBusPendingReply<Sysupdate::TargetList>(*call);
        if (watcher.isError()) {
            qWarning() << "Failed to list targets:" << watcher.error().message();
            return;
        }

        auto targets = watcher.value();
        for (const auto &target : targets) {
            qDebug() << "Target:" << target.name << QMetaEnum::fromType<Sysupdate::TargetClass>().valueToKey(static_cast<int>(target.targetClass))
                     << target.objectPath.path();
        }
    });
}

QString SystemdSysupdateBackend::displayName() const
{
    return QStringLiteral("Systemd SysUpdate");
}

#include "SystemdSysupdateBackend.moc"