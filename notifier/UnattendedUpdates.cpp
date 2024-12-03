/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UnattendedUpdates.h"
#include "DiscoverNotifier.h"
#include "updatessettings.h"
#include <KIdleTime>
#include <QDateTime>
#include <QDebug>
#include <QProcess>
#include <chrono>

UnattendedUpdates::UnattendedUpdates(DiscoverNotifier *parent)
    : QObject(parent)
{
    connect(parent, &DiscoverNotifier::stateChanged, this, &UnattendedUpdates::checkNewState);
    connect(KIdleTime::instance(), QOverload<int, int>::of(&KIdleTime::timeoutReached), this, &UnattendedUpdates::triggerUpdate);

    checkNewState();
}

void UnattendedUpdates::checkNewState()
{
    using namespace std::chrono_literals;
    DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());

    // Only allow offline updating every 3h. It should keep some peace to our users, especially on rolling distros
    const QDateTime updateableTime = notifier->settings()->lastUnattendedTrigger().addSecs(std::chrono::seconds(3h).count());
    if (updateableTime > QDateTime::currentDateTimeUtc()) {
        qDebug() << "skipping update, already updated on" << notifier->settings()->lastUnattendedTrigger().toString();
        return;
    }

    const bool doTrigger = notifier->hasUpdates() && !notifier->isBusy();
    if (doTrigger && !m_idleTimeoutId.has_value()) {
        qDebug() << "waiting for an idle moment";
        // If the system is untouched for 1 minute, trigger the unattended update.
        // The trick here is that we want to trigger the update, but ideally not when the user is using the computer.
        // The idle timeout cannot be too long or it won't ever update, but it also shouldn't be none because then
        // we definitely trigger the update while the user is using the computer. So we arrived at 1 minute.
        // Should this not work out, a more expansive solution needs inventing (e.g. cgroup constraining of resource use).
        m_idleTimeoutId.emplace(1min);
    } else if (!doTrigger && m_idleTimeoutId.has_value()) {
        qDebug() << "nothing to do";
        m_idleTimeoutId.reset();
    }
}

void UnattendedUpdates::triggerUpdate(int timeoutId)
{
    if (!m_idleTimeoutId.has_value() || timeoutId != m_idleTimeoutId->m_id) {
        return;
    }

    m_idleTimeoutId.reset();

    DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());
    if (!notifier->hasUpdates() || notifier->isBusy()) {
        return;
    }

    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [](QProcess::ProcessError error) {
        qWarning() << "Error running plasma-discover-update" << error;
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Finished running plasma-discover-update" << exitCode << exitStatus;
        DiscoverNotifier *notifier = static_cast<DiscoverNotifier *>(parent());
        process->deleteLater();
        notifier->settings()->setLastUnattendedTrigger(QDateTime::currentDateTimeUtc());
        notifier->settings()->save();
        notifier->setBusy(false);
    });

    notifier->setBusy(true);
    process->start(QStringLiteral("plasma-discover-update"), {QStringLiteral("--offline")});
    qInfo() << "started unattended update" << QDateTime::currentDateTimeUtc();
}

UnattendedUpdates::IdleHandle::IdleHandle(const std::chrono::milliseconds &idleTimeout)
    : m_id(KIdleTime::instance()->addIdleTimeout(int(idleTimeout.count())))
{
}

UnattendedUpdates::IdleHandle::~IdleHandle()
{
    KIdleTime::instance()->removeIdleTimeout(m_id);
}
