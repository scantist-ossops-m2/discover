//
// Created by fernie on 10/8/24.
//

#include "SystemdSysupdateResource.h"

#include "../../../../../build/discover/notifier/Login1ManagerInterface.h"

#include <AppStreamQt/developer.h>
#include <sysupdate1.h>

SystemdSysupdateResource::SystemdSysupdateResource(const AppStream::Component component, AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_component(component)
{
}

QString SystemdSysupdateResource::packageName() const
{
    if (m_component.packageNames().isEmpty()) {
        return QString();
    }

    return m_component.packageNames().first();
}

QString SystemdSysupdateResource::name() const
{
    return m_component.name();
}

QString SystemdSysupdateResource::comment()
{
    return m_component.summary();
}

QVariant SystemdSysupdateResource::icon() const
{
    return QStringLiteral("system-upgrade");
}

bool SystemdSysupdateResource::canExecute() const
{
    // TODO: Should we only allow this if newer
    return true;
}

void SystemdSysupdateResource::invokeApplication() const
{
}

AbstractResource::State SystemdSysupdateResource::state()
{
    return Upgradeable;
}

bool SystemdSysupdateResource::hasCategory(const QString &category) const
{
    return m_component.hasCategory(category);
}

AbstractResource::Type SystemdSysupdateResource::type() const
{
    return System;
}

quint64 SystemdSysupdateResource::size()
{
    // FIXME: Query Target for size
    return 6;
}

QJsonArray SystemdSysupdateResource::licenses()
{
    return QJsonArray({m_component.projectLicense(), m_component.metadataLicense()});
}

QString SystemdSysupdateResource::installedVersion() const
{
    return QStringLiteral("debug_202410070621");
}

QString SystemdSysupdateResource::availableVersion() const
{
    return QStringLiteral("202410100008");
}

QString SystemdSysupdateResource::longDescription()
{
    return m_component.description();
}

QString SystemdSysupdateResource::origin() const
{
    return m_component.origin();
}

QString SystemdSysupdateResource::section()
{
    return QStringLiteral();
}

QString SystemdSysupdateResource::author() const
{
    return m_component.developer().name();
}

QList<PackageState> SystemdSysupdateResource::addonsInformation()
{
    return {};
}

QString SystemdSysupdateResource::sourceIcon() const
{
    return QStringLiteral();
}

QDate SystemdSysupdateResource::releaseDate() const
{
    const auto releases = m_component.releasesPlain();
    if (!releases.isEmpty()) {
        auto release = releases.indexSafe(0);
        if (release) {
            return release->timestamp().date();
        }
    }

    return {};
}

void SystemdSysupdateResource::fetchChangelog()
{
}