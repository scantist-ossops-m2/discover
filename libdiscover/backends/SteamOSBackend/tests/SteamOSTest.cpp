/***************************************************************************
 *   SPDX-FileCopyrightText: 2024 Jeremy Whiting <jpwhiting@kde.org>       *
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 ***************************************************************************/

#include <ApplicationAddonsModel.h>
#include <Category/CategoryModel.h>
#include <ReviewsBackend/ReviewsModel.h>
#include <Transaction/TransactionModel.h>
#include <resources/AbstractBackendUpdater.h>
#include <resources/DiscoverAction.h>
#include <resources/ResourcesModel.h>
#include <resources/ResourcesProxyModel.h>
#include <resources/SourcesModel.h>

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include "libdiscover_steamos_debug.h"

class SteamOSTest : public QObject
{
    Q_OBJECT
public:
    AbstractResourcesBackend *backendByName(ResourcesModel *m, const QString &name)
    {
        const QVector<AbstractResourcesBackend *> backends = m->backends();
        qDebug() << "backends count: " << m->backends().count();
        for (AbstractResourcesBackend *backend : backends) {
            if (QLatin1String(backend->metaObject()->className()) == name) {
                return backend;
            }
        }
        return nullptr;
    }

    SteamOSTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String("/discover-steamos-test")).removeRecursively();

        QStandardPaths::setTestModeEnabled(true);
        qputenv("STEAMOS_TEST_MODE", "ON");
        m_model = new ResourcesModel(QStringLiteral("steamos-backend"), this);
        m_appBackend = backendByName(m_model, QStringLiteral("SteamOSBackend"));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_appBackend);
        while (m_appBackend->isFetching()) {
            QSignalSpy spy(m_appBackend, &AbstractResourcesBackend::fetchingChanged);
            QVERIFY(spy.wait());
        }
    }

private:
    Transaction::Status waitTransaction(Transaction *t)
    {
        int lastProgress = -1;
        connect(t, &Transaction::progressChanged, this, [t, &lastProgress] {
            Q_ASSERT(lastProgress <= t->progress());
            lastProgress = t->progress();
        });

        TransactionModel::global()->addTransaction(t);
        QSignalSpy spyInstalled(TransactionModel::global(), &TransactionModel::transactionRemoved);
        QSignalSpy destructionSpy(t, &QObject::destroyed);

        Transaction::Status ret = t->status();
        connect(TransactionModel::global(), &TransactionModel::transactionRemoved, t, [t, &ret](Transaction *trans) {
            if (trans == t) {
                ret = trans->status();
            }
        });
        connect(t, &Transaction::statusChanged, t, [t] {
            qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "status" << t->status();
        });
        while (t && spyInstalled.count() == 0) {
            qCDebug(LIBDISCOVER_BACKEND_STEAMOS_LOG) << "waiting, currently" << ret << t->progress() << spyInstalled.count() << destructionSpy.count();
            spyInstalled.wait(1000);
        }
        Q_ASSERT(destructionSpy.count() || destructionSpy.wait());
        return ret;
    }

    QVector<AbstractResource *> getResources(ResultsStream *stream, bool canBeEmpty = true)
    {
        Q_ASSERT(stream);
        QSignalSpy spyResources(stream, &ResultsStream::destroyed);
        QVector<AbstractResource *> resources;
        connect(stream, &ResultsStream::resourcesFound, this, [&resources](const QVector<StreamResult> &res) {
            for (auto result : res) {
                resources += result.resource;
            }
        });
        Q_ASSERT(spyResources.wait(100000));
        Q_ASSERT(!resources.isEmpty() || canBeEmpty);
        return resources;
    }

    QVector<AbstractResource *> getAllResources(AbstractResourcesBackend *backend)
    {
        AbstractResourcesBackend::Filters f;
        if (CategoryModel::global()->rootCategories().isEmpty())
            CategoryModel::global()->populateCategories();
        f.category = CategoryModel::global()->rootCategories().constFirst();
        return getResources(backend->search(f), true);
    }

    ResourcesModel *m_model;
    AbstractResourcesBackend *m_appBackend;
};

QTEST_GUILESS_MAIN(SteamOSTest)

#include "SteamOSTest.moc"
