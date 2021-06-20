/***************************************************************************
 *   Copyright © 2020 Alexey Min <alexey.min@gmail.com>                    *
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

#include "AlpineApkResource.h"
#include "alpineapk_backend_logging.h"  // generated by ECM

#include <AppStreamQt/icon.h>
#include <AppStreamQt/pool.h>
#include <AppStreamQt/release.h>

#include <QFileInfo>
#include <QIcon>
#include <QProcess>

// libdiscover
#include "appstream/AppStreamUtils.h"
#include "config-paths.h"
#include "Transaction/AddonList.h"

AlpineApkResource::AlpineApkResource(const QtApk::Package &apkPkg,
                                     AppStream::Component &component,
                                     AbstractResource::Type typ,
                                     AbstractResourcesBackend *parent)
    : AbstractResource(parent)
    , m_state(AbstractResource::State::None)
    , m_type(typ)
    , m_pkg(apkPkg)
    , m_appsC(component)
{
}

QList<PackageState> AlpineApkResource::addonsInformation()
{
    return m_addons;
}

QString AlpineApkResource::availableVersion() const
{
    return m_availableVersion;
}

QStringList AlpineApkResource::categories()
{
    if (hasAppStreamData()) {
        return m_appsC.categories();
    }
    return { m_category };
}

QString AlpineApkResource::comment()
{
    if (hasAppStreamData()) {
        return m_appsC.summary();
    }
    return m_pkg.description;
}

int AlpineApkResource::size()
{
    return static_cast<int>(m_pkg.size);
}

QUrl AlpineApkResource::homepage()
{
    if (hasAppStreamData()) {
        return m_appsC.url(AppStream::Component::UrlKindHomepage);
    }
    return QUrl::fromUserInput(m_pkg.url);
}

QUrl AlpineApkResource::helpURL()
{
    if (hasAppStreamData()) {
        return m_appsC.url(AppStream::Component::UrlKindHelp);
    }
    return QUrl();
}

QUrl AlpineApkResource::bugURL()
{
    if (hasAppStreamData()) {
        return m_appsC.url(AppStream::Component::UrlKindBugtracker);
    }
    return QUrl();
}

QUrl AlpineApkResource::donationURL()
{
    if (hasAppStreamData()) {
        return m_appsC.url(AppStream::Component::UrlKindDonation);
    }
    return QUrl();
}

///xdg-compatible icon name to represent the resource, url or QIcon
QVariant AlpineApkResource::icon() const
{
    if (hasAppStreamData()) {
        const QList<AppStream::Icon> icns = m_appsC.icons();
        if (icns.size() == 0) {
            return QStringLiteral("package-x-generic");
        }
        QIcon ico;
        const AppStream::Icon &appIco = icns.first();

        switch (appIco.kind()) {
        case AppStream::Icon::KindStock:
            // we can create icons of this type directly from theme
            ico = QIcon::fromTheme(appIco.name());
            break;
        case AppStream::Icon::KindLocal:
        case AppStream::Icon::KindCached: {
            // try from predefined standard Alpine path
            const QString appstreamIconsPath = QLatin1String("/usr/share/app-info/icons/");
            const QString path = appstreamIconsPath + appIco.url().path();
            if (QFileInfo::exists(path)) {
                ico.addFile(path, appIco.size());
            } else {
                const QString altPath = appstreamIconsPath +
                        QStringLiteral("%1x%2/").arg(appIco.size().width()).arg(appIco.size().height()) +
                        appIco.url().path();
                if (QFileInfo::exists(altPath)) {
                    ico.addFile(altPath, appIco.size());
                }
            }
        } break;
        default: break;
        }

        // return icon only if we successfully loaded it
        if (!ico.isNull()) {
            return QVariant::fromValue<QIcon>(ico);
        }

        // try to load from icon theme by package name, this is better
        //   than nothing and works surprisingly well for many packages
        ico = QIcon::fromTheme(m_pkg.name);
        if (!ico.isNull()) {
            return QVariant::fromValue<QIcon>(ico);
        }
    }
    return QStringLiteral("package-x-generic");
}

QString AlpineApkResource::installedVersion() const
{
    return m_pkg.version;
}

QJsonArray AlpineApkResource::licenses()
{
    return {
        QJsonObject {
            { QStringLiteral("name"), m_pkg.license },
            { QStringLiteral("url"), QStringLiteral("https://spdx.org/license-list") },
        }
    };
}

QString AlpineApkResource::longDescription()
{
    if (hasAppStreamData()) {
        return m_appsC.description();
    }
    return m_pkg.description;
}

QString AlpineApkResource::name() const
{
    if (hasAppStreamData()) {
        return m_appsC.name();
    }
    return m_pkg.name;
}

QString AlpineApkResource::origin() const
{
    return m_originSoruce;
}

QString AlpineApkResource::packageName() const
{
    return m_pkg.name;
}

QString AlpineApkResource::section()
{
    return m_sectionName;
}

AbstractResource::State AlpineApkResource::state()
{
    return m_state;
}

void AlpineApkResource::fetchChangelog()
{
    if (hasAppStreamData()) {
        emit changelogFetched(AppStreamUtils::changelogToHtml(m_appsC));
    }
}

void AlpineApkResource::fetchScreenshots()
{
    if (hasAppStreamData()) {
        /*const*/ QPair<QList<QUrl>, QList<QUrl>> sc = AppStreamUtils::fetchScreenshots(m_appsC);
        if (sc.first.size() > 0) {
            const QUrl &url = sc.first.first();
            if (url.isRelative()) {
                // This is a hack to fix broken Alpine Linux appstream
                // metadata: somehow generated thumbnails URLs are relative
                // and always fail to load (not present in metadata at all)
                // We work around this by using full url, which is veyr not
                // good - they're fetched by http://...
                // It would be great if thumbnails were hosted locally.
                if (sc.second.size() > 0) {
                    sc.first = sc.second;
                }
            }
        }
        Q_EMIT screenshotsFetched(sc.first, sc.second);
    }
}

QString AlpineApkResource::appstreamId() const
{
    if (hasAppStreamData()) {
        return m_appsC.id();
    }
    return QString();
}

void AlpineApkResource::setState(AbstractResource::State state)
{
    m_state = state;
    emit stateChanged();
}

void AlpineApkResource::setCategoryName(const QString &categoryName)
{
    m_category = categoryName;
}

void AlpineApkResource::setOriginSource(const QString &originSource)
{
    m_originSoruce = originSource;
}

void AlpineApkResource::setSection(const QString &sectionName)
{
    m_sectionName = sectionName;
}

void AlpineApkResource::setAddons(const AddonList &addons)
{
    const QStringList addonsToInstall = addons.addonsToInstall();
    for (const QString &toInstall : addonsToInstall) {
        setAddonInstalled(toInstall, true);
    }
    const QStringList addonsToRemove = addons.addonsToRemove();
    for (const QString &toRemove : addonsToRemove) {
        setAddonInstalled(toRemove, false);
    }
}

void AlpineApkResource::setAddonInstalled(const QString &addon, bool installed)
{
    for(PackageState &elem : m_addons) {
        if(elem.name() == addon) {
            elem.setInstalled(installed);
        }
    }
}

void AlpineApkResource::setAvailableVersion(const QString &av)
{
    m_availableVersion = av;
}

bool AlpineApkResource::hasAppStreamData() const
{
    return !m_appsC.id().isEmpty();
}

void AlpineApkResource::setAppStreamData(const AppStream::Component &component)
{
    m_appsC = component;
}

bool AlpineApkResource::canExecute() const
{
    if (hasAppStreamData()) {
        return (m_appsC.kind() == AppStream::Component::KindDesktopApp &&
                (m_state == AbstractResource::Installed || m_state == AbstractResource::Upgradeable));
    }
    return false;
}

void AlpineApkResource::invokeApplication() const
{
    const QString desktopFile = QLatin1String("/usr/share/applications/") + appstreamId();
    if (QFile::exists(desktopFile)) {
        QProcess::startDetached(QStringLiteral("kstart5"), {QStringLiteral("--service"), desktopFile});
    }
}

QUrl AlpineApkResource::url() const
{
    if (hasAppStreamData()) {
        return QUrl(QStringLiteral("appstream://") + appstreamId());
    }
    return QUrl(QLatin1String("apk://") + packageName());
}

QString AlpineApkResource::author() const
{
    if (hasAppStreamData()) {
        return m_appsC.developerName();
    }
    return m_pkg.maintainer;
}

QString AlpineApkResource::sourceIcon() const
{
    return QStringLiteral("alpine-linux-logo-icon");
}

QDate AlpineApkResource::releaseDate() const
{
    if (hasAppStreamData()) {
        if (!m_appsC.releases().isEmpty()) {
            auto release = m_appsC.releases().constFirst();
            return release.timestamp().date();
        }
    }
    // just build date is fine, too
    return m_pkg.buildTime.date();
}
