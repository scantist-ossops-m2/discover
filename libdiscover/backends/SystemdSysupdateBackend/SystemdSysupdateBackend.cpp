/*
 *   SPDX-FileCopyrightText: 2024 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SystemdSysupdateBackend.h"
#include "SystemdSysupdateResource.h"
#include "SystemdSysupdateTransaction.h"
#include "libdiscover_systemdsysupdate_debug.h"

#include <AppStreamQt/metadata.h>
#include <QCoro/QCoroDBusPendingReply>
#include <QCoro/QCoroNetworkReply>
#include <QList>
#include <QNetworkReply>
#include <QtPreprocessorSupport>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>

DISCOVER_BACKEND_PLUGIN(SystemdSysupdateBackend)

#define category LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG

const auto path = QStringLiteral("/org/freedesktop/sysupdate1");

SystemdSysupdateBackend::SystemdSysupdateBackend(QObject *parent)
    : AbstractResourcesBackend(parent)
    , m_updater(new StandardBackendUpdater(this))
    , m_manager(new org::freedesktop::sysupdate1::Manager(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus(), this))
    , m_nam(new QNetworkAccessManager(this))
{
    qDBusRegisterMetaType<Sysupdate::Target>();
    qDBusRegisterMetaType<Sysupdate::TargetList>();

    connect(m_manager, &org::freedesktop::sysupdate1::Manager::JobRemoved, this, &SystemdSysupdateBackend::transactionRemoved);
    connect(m_updater, &StandardBackendUpdater::updatesCountChanged, this, &SystemdSysupdateBackend::updatesCountChanged);
    QTimer::singleShot(0, this, &SystemdSysupdateBackend::checkForUpdates);
}

int SystemdSysupdateBackend::updatesCount() const
{
    return m_updater->updatesCount();
}

bool SystemdSysupdateBackend::isValid() const
{
    auto ping = org::freedesktop::DBus::Peer(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus()).Ping();
    ping.waitForFinished();
    return !ping.isError();
}

ResultsStream *SystemdSysupdateBackend::search(const AbstractResourcesBackend::Filters &filter)
{
    Q_UNUSED(filter);

    // Since we'll only ever have a handful of targets, we can just return all of them
    QVector<StreamResult> results;
    for (const auto &resource : m_resources) {
        results << StreamResult(resource);
    }

    return new ResultsStream(QStringLiteral("systemd-sysupdate"), results);
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
    const auto resource = qobject_cast<SystemdSysupdateResource *>(app);
    if (!resource) {
        qCCritical(category) << "Failed to cast resource to SystemdSysupdateResource";
        return nullptr;
    }

    return resource->update();
}

Transaction *SystemdSysupdateBackend::installApplication(AbstractResource *app, const AddonList &addons)
{
    Q_UNUSED(addons);
    return installApplication(app);
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
    qCDebug(category) << "Updating systemd-sysupdate backend...";
    checkForUpdatesAsync();
}

QCoro::Task<> SystemdSysupdateBackend::checkForUpdatesAsync()
{
    if (isFetching()) {
        qCInfo(category) << "Already fetching updates. Skipping...";
        co_return;
    }

    beginFetch();

    for (auto resource : m_resources) {
        Q_EMIT resourceRemoved(resource);
    }
    m_resources.clear();

    const auto targetsReply = co_await m_manager->ListTargets();
    if (targetsReply.isError()) {
        qCWarning(category) << "Failed to list targets:" << targetsReply.error().message();
        co_return;
    }

    // TODO: Make this parallel once QCoro2 is released
    // https://github.com/qcoro/qcoro/issues/250
    for (const auto &[targetClass, name, objectPath] : targetsReply.value()) {
        qCDebug(category) << "Target:" << name << targetClass << objectPath.path();

        auto target = new org::freedesktop::sysupdate1::Target(SYSUPDATE1_SERVICE, objectPath.path(), QDBusConnection::systemBus(), this);
        const auto appStream = co_await target->GetAppStream();
        if (appStream.isError()) {
            qCCritical(category) << "Failed to get appstream for target (" << name << ") :" << appStream.error().message();
            continue;
        }
        auto appStreamUrls = appStream.value();
        if (appStreamUrls.isEmpty()) {
            qCritical(category) << "No appstream URLs found for target:" << name;
            continue;
        }

        qCDebug(category) << "AppStream:" << appStreamUrls;
        AppStream::Metadata metadata;
        for (auto url : appStreamUrls) {
            const auto reply = co_await m_nam->get(QNetworkRequest(QUrl(url)));
            if (reply->error() != QNetworkReply::NoError) {
                qCWarning(category) << "Failed to fetch appstream:" << reply->errorString();
                continue;
            }

            // if the first non-whitespace character is not a '<', it's probably a YAML file
            auto data = QString::fromUtf8(reply->readAll()).trimmed();
            auto format = data.startsWith(QLatin1Char('<')) ? AppStream::Metadata::FormatKindXml : AppStream::Metadata::FormatKindYaml;

            metadata.parse(data, format);
        }

        auto components = metadata.components();
        if (components.isEmpty()) {
            qCCritical(category) << "No components found in appstream metadata for target:" << name;
            continue;
        }

        if (components.size() > 1) {
            qCWarning(category) << "Multiple components found in appstream metadata for target:" << name << ". Using the first one.";
        }

        auto component = metadata.component();
        qCDebug(category) << "Component:" << component.name() << component.summary() << component.description();

        m_resources << new SystemdSysupdateResource(this,
                                                    component,
                                                    {.installedVersion = co_await target->GetVersion(), .availableVersion = co_await target->CheckNew()},
                                                    target);
    }

    endFetch();
}

QString SystemdSysupdateBackend::displayName() const
{
    return QStringLiteral("Systemd SysUpdate");
}

void SystemdSysupdateBackend::beginFetch()
{
    m_fetchOperationCount++;
    if (m_fetchOperationCount == 1) {
        Q_EMIT fetchingChanged();
    }
}
void SystemdSysupdateBackend::endFetch()
{
    m_fetchOperationCount--;
    if (m_fetchOperationCount == 0) {
        Q_EMIT fetchingChanged();
    }
}

#include "SystemdSysupdateBackend.moc"