//
// Created by fernie on 10/8/24.
//

#include "SystemdSysupdateResource.h"

#include <AppStreamQt/developer.h>
#include <sysupdate1.h>

SystemdSysupdateResource::SystemdSysupdateResource(AbstractResourcesBackend *parent,
                                                   const AppStream::Component &component,
                                                   const Sysupdate::TargetInfo &targetInfo)

    : AbstractResource(parent)
    , m_component(component)
    , m_targetInfo(targetInfo)
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
    // It doesn't make sense to have a "Launch" button for the OS
    return false;
}

bool SystemdSysupdateResource::isRemovable() const
{
    // Doesn't make sense to remove the OS either
    return false;
}

void SystemdSysupdateResource::invokeApplication() const
{
}

AbstractResource::State SystemdSysupdateResource::state()
{
    return availableVersion() != installedVersion() ? Upgradeable : Installed;
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
    // Will need to be updated once sysupdate implements size querying
    // https://github.com/systemd/systemd/issues/34710
    return 0;
}

QJsonArray SystemdSysupdateResource::licenses()
{
    return QJsonArray({m_component.projectLicense(), m_component.metadataLicense()});
}

QString SystemdSysupdateResource::installedVersion() const
{
    return m_targetInfo.installedVersion;
}

QString SystemdSysupdateResource::availableVersion() const
{
    return m_targetInfo.availableVersion;
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
    auto releaseList = m_component.loadReleases(true).value_or(m_component.releasesPlain());
    auto targetRelease = availableVersion();
    for (auto release : releaseList.entries()) {
        if (release.version() == targetRelease) {
            Q_EMIT changelogFetched(release.description());
            break;
        }
    }
}