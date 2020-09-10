/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverBackendsFactory.h"
#include "resources/AbstractResourcesBackend.h"
#include "resources/ResourcesModel.h"
#include "utils.h"
#include "libdiscover_debug.h"
#include <QStandardPaths>
#include <QDir>
#include <QCommandLineParser>
#include <QPluginLoader>
#include <QDirIterator>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

Q_GLOBAL_STATIC(QStringList, s_requestedBackends)

void DiscoverBackendsFactory::setRequestedBackends(const QStringList& backends)
{
    *s_requestedBackends = backends;
}

bool DiscoverBackendsFactory::hasRequestedBackends()
{
    return !s_requestedBackends->isEmpty();
}

DiscoverBackendsFactory::DiscoverBackendsFactory()
{}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::backend(const QString& name) const
{
    if (QDir::isAbsolutePath(name) && QStandardPaths::isTestModeEnabled()) {
        return backendForFile(name, QFileInfo(name).fileName());
    } else {
        return backendForFile(name, name);
    }
}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::backendForFile(const QString& libname, const QString& name) const
{
    QPluginLoader* loader = new QPluginLoader(QLatin1String("discover/") + libname, ResourcesModel::global());

    //     qCDebug(LIBDISCOVER_LOG) << "trying to load plugin:" << loader->fileName();
    AbstractResourcesBackendFactory* f = qobject_cast<AbstractResourcesBackendFactory*>(loader->instance());
    if(!f) {
        qCWarning(LIBDISCOVER_LOG) << "error loading" << libname << loader->errorString() << loader->metaData();
        return {};
    }
    auto instances = f->newInstance(ResourcesModel::global(), name);
    if(instances.isEmpty()) {
        qCWarning(LIBDISCOVER_LOG) << "Couldn't find the backend: " << libname << "among" << allBackendNames(false, true);
        return instances;
    }

    return instances;
}

QStringList DiscoverBackendsFactory::allBackendNames(bool whitelist, bool allowDummy) const
{
    if (whitelist) {
        QStringList whitelistNames = *s_requestedBackends;
        if (!whitelistNames.isEmpty())
            return whitelistNames;
    }

    QStringList pluginNames;
    foreach (const QString &dir, QCoreApplication::libraryPaths()) {
        QDirIterator it(dir + QStringLiteral("/discover"), QDir::Files);
        while (it.hasNext()) {
            it.next();
            if (QLibrary::isLibrary(it.fileName()) && (allowDummy || it.fileName() != QLatin1String("dummy-backend.so"))) {
                pluginNames += it.fileInfo().baseName();
            }
        }
    }

    pluginNames.removeDuplicates(); //will happen when discover is installed twice on the system
    return pluginNames;
}

QVector<AbstractResourcesBackend*> DiscoverBackendsFactory::allBackends() const
{
    QStringList names = allBackendNames();
    auto ret = kTransform<QVector<AbstractResourcesBackend*>>(names, [this](const QString& name) { return backend(name); });
    ret.removeAll(nullptr);

    if(ret.isEmpty())
        qCWarning(LIBDISCOVER_LOG) << "Didn't find any Discover backend!";
    return ret;
}

int DiscoverBackendsFactory::backendsCount() const
{
    return allBackendNames().count();
}

void DiscoverBackendsFactory::setupCommandLine(QCommandLineParser* parser)
{
    parser->addOption(QCommandLineOption(QStringLiteral("backends"), i18n("List all the backends we'll want to have loaded, separated by comma ','."), QStringLiteral("names")));
}

void DiscoverBackendsFactory::processCommandLine(QCommandLineParser* parser, bool test)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringList backends = test ? QStringList{ QStringLiteral("dummy-backend") } : parser->value(QStringLiteral("backends")).split(QLatin1Char(','), QString::SkipEmptyParts);
#else
    QStringList backends = test ? QStringList{ QStringLiteral("dummy-backend") } : parser->value(QStringLiteral("backends")).split(QLatin1Char(','), Qt::SkipEmptyParts);
#endif
    for(auto &backend: backends) {
        if (!backend.endsWith(QLatin1String("-backend")))
            backend.append(QLatin1String("-backend"));
    }
    *s_requestedBackends = backends;
}
