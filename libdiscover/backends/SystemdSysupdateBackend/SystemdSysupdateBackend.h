/*
 *   SPDX-FileCopyrightText: 2022 Lasath Fernando <devel@lasath.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SYSTEMD_SYSUPDATE_BACKEND_H
#define SYSTEMD_SYSUPDATE_BACKEND_H

#include "sysupdate1.h"
#include <resources/AbstractResourcesBackend.h>
#include <resources/StandardBackendUpdater.h>

class SystemdSysupdateBackend : public AbstractResourcesBackend
{
    Q_OBJECT

public:
    explicit SystemdSysupdateBackend(QObject *parent = nullptr);

    int updatesCount() const override;
    AbstractBackendUpdater *backendUpdater() const override;
    AbstractReviewsBackend *reviewsBackend() const override;
    ResultsStream *search(const AbstractResourcesBackend::Filters &search) override;
    bool isValid() const override;
    Transaction *installApplication(AbstractResource *app) override;
    Transaction *installApplication(AbstractResource *app, const AddonList &addons) override;
    Transaction *removeApplication(AbstractResource *app) override;

    bool isFetching() const override;
    void checkForUpdates() override;
    QString displayName() const override;

private:
    StandardBackendUpdater *m_updater;
    int m_fetchOperationCount = 0;
    QPointer<org::freedesktop::sysupdate1::Manager> m_manager;
};

#endif // SYSTEMD_SYSUPDATE_BACKEND_H
