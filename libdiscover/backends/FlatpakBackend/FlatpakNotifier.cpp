/*
 *   SPDX-FileCopyrightText: 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FlatpakNotifier.h"
#include "libdiscover_backend_flatpak_debug.h"

#include <glib.h>

#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrentRun>

using namespace std::chrono_literals;

static void installationChanged(GFileMonitor *monitor, GFile *child, GFile *other_file, GFileMonitorEvent event_type, gpointer self)
{
    Q_UNUSED(monitor);
    Q_UNUSED(child);
    Q_UNUSED(other_file);
    Q_UNUSED(event_type);

    FlatpakNotifier::Installation *installation = (FlatpakNotifier::Installation *)self;
    if (!installation)
        return;

    FlatpakNotifier *notifier = installation->m_notifier;
    notifier->loadRemoteUpdates(installation);
}

FlatpakNotifier::FlatpakNotifier(QObject *parent)
    : BackendNotifierModule(parent)
    , m_user(this)
    , m_system(this)
    , m_cancellable(g_cancellable_new())
{
    QTimer *dailyCheck = new QTimer(this);
    dailyCheck->setInterval(24h); // refresh at least once every day
    connect(dailyCheck, &QTimer::timeout, this, &FlatpakNotifier::recheckSystemUpdateNeeded);
}

FlatpakNotifier::Installation::Installation(FlatpakNotifier *notifier)
    : m_notifier(notifier)
{
}

FlatpakNotifier::Installation::~Installation()
{
    if (m_monitor)
        g_object_unref(m_monitor);
    if (m_installation)
        g_object_unref(m_installation);
}

FlatpakNotifier::~FlatpakNotifier()
{
    g_object_unref(m_cancellable);
}

void FlatpakNotifier::recheckSystemUpdateNeeded()
{
    setupFlatpakInstallations();

    // Load updates from remote repositories
    loadRemoteUpdates(&m_system);
    loadRemoteUpdates(&m_user);
}

void FlatpakNotifier::onFetchUpdatesFinished(Installation *installation, bool hasUpdates)
{
    if (installation->m_hasUpdates == hasUpdates) {
        return;
    }
    bool hadUpdates = this->hasUpdates();
    installation->m_hasUpdates = hasUpdates;

    if (hadUpdates != this->hasUpdates()) {
        Q_EMIT foundUpdates();
    }
}

void FlatpakNotifier::loadRemoteUpdates(Installation *installation)
{
    auto fw = new QFutureWatcher<bool>(this);
    connect(fw, &QFutureWatcher<bool>::finished, this, [this, installation, fw]() {
        onFetchUpdatesFinished(installation, fw->result());
        fw->deleteLater();
    });
    fw->setFuture(QtConcurrent::run([installation]() -> bool {
        g_autoptr(GCancellable) cancellable = g_cancellable_new();
        g_autoptr(GError) localError = nullptr;
        g_autoptr(GPtrArray) fetchedUpdates = flatpak_installation_list_installed_refs_for_update(installation->m_installation, cancellable, &localError);
        bool hasUpdates = false;

        if (!fetchedUpdates) {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to get list of installed refs for listing updates: " << localError->message;
            return false;
        }
        for (uint i = 0; !hasUpdates && i < fetchedUpdates->len; i++) {
            FlatpakInstalledRef *ref = FLATPAK_INSTALLED_REF(g_ptr_array_index(fetchedUpdates, i));
            const QString refName = QString::fromUtf8(flatpak_ref_get_name(FLATPAK_REF(ref)));
            // FIXME right now I can't think of any other filter than this, in FlatpakBackend updates are matched
            // with apps so .Locale/.Debug subrefs are not shown and updated automatically. Also this will show
            // updates for refs we don't show in Discover if appstream metadata or desktop file for them is not found
            if (refName.endsWith(QLatin1String(".Locale")) || refName.endsWith(QLatin1String(".Debug"))) {
                continue;
            }
            hasUpdates = true;
        }
        return hasUpdates;
    }));
}

bool FlatpakNotifier::hasUpdates()
{
    return m_system.m_hasUpdates || m_user.m_hasUpdates;
}

bool FlatpakNotifier::Installation::ensureInitialized(std::function<FlatpakInstallation *(GError **error)> func, GCancellable *cancellable)
{
    if (!m_installation) {
        g_autoptr(GError) error = nullptr;
        m_installation = func(&error);
        if (m_installation) {
            m_monitor = flatpak_installation_create_monitor(m_installation, cancellable, &error);
            g_signal_connect(m_monitor, "changed", G_CALLBACK(installationChanged), this);
        } else {
            qCWarning(LIBDISCOVER_BACKEND_FLATPAK_LOG) << "Failed to setup flatpak installation: " << error->message;
        }
    }
    return m_installation && m_monitor;
}

void FlatpakNotifier::setupFlatpakInstallations()
{
    m_system.ensureInitialized(
        [this](GError **error) {
            return flatpak_installation_new_system(m_cancellable, error);
        },
        m_cancellable);

    m_user.ensureInitialized(
        [this](GError **error) {
            return flatpak_installation_new_user(m_cancellable, error);
        },
        m_cancellable);
}

#include "moc_FlatpakNotifier.cpp"
