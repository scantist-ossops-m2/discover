//
// Created by fernie on 10/11/24.
//

#include "SystemdSysupdateTransaction.h"
#include "SystemdSysupdateBackend.h"
#include "libdiscover_systemdsysupdate_debug.h"

#include <QCoro/QCoroDBusPendingReply>
#include <QCoro/QCoroTask>

#define category LIBDISCOVER_BACKEND_SYSTEMDSYSUPDATE_LOG

const auto PROGRESS_PROPERTY_NAME = QStringLiteral("Progress");
const QString SYSUPDATE_JOB_INTERFACE_NAME = QLatin1String(org::freedesktop::sysupdate1::Job::staticInterfaceName());

SystemdSysupdateTransaction::SystemdSysupdateTransaction(AbstractResource *resource, qulonglong id, const QString &path)
    : Transaction(resource, resource, InstallRole)
    , m_job(new org::freedesktop::sysupdate1::Job(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus(), this))
    , m_properties(new org::freedesktop::DBus::Properties(SYSUPDATE1_SERVICE, path, QDBusConnection::systemBus(), this))
{
    connect(m_properties,
            &org::freedesktop::DBus::Properties::PropertiesChanged,
            this,
            [this](const QString &interface, const QVariantMap &changed, const QStringList &invalidated) -> QCoro::Task<> {
                if (interface != SYSUPDATE_JOB_INTERFACE_NAME) {
                    co_return;
                }

                qCDebug(category) << "Properties changed:" << changed << "Invalidated:" << invalidated;

                if (changed.contains(PROGRESS_PROPERTY_NAME)) {
                    setProgress(changed.value(PROGRESS_PROPERTY_NAME).toUInt());
                }

                if (invalidated.contains(PROGRESS_PROPERTY_NAME)) {
                    const auto reply = co_await m_properties->Get(SYSUPDATE_JOB_INTERFACE_NAME, PROGRESS_PROPERTY_NAME);
                    if (reply.isError()) {
                        qCCritical(category) << "Failed to get progress:" << reply.error().message();
                        co_return;
                    }

                    setProgress(reply.argumentAt(0).toUInt());
                }
            });
    connect(qobject_cast<SystemdSysupdateBackend *>(resource->backend()),
            &SystemdSysupdateBackend::transactionRemoved,
            [id, resource, this](qulonglong jobId, const QDBusObjectPath &jobPath, int status) {
                if (id != jobId) {
                    return;
                }

                qCInfo(category) << "Job" << jobPath.path() << "for target" << resource->name() << "finished with status" << status;
                setStatus(status == 0 ? DoneStatus : DoneWithErrorStatus);
            });
}

void SystemdSysupdateTransaction::cancel()
{
    m_job->Cancel();
}
SystemdSysupdateTransaction::~SystemdSysupdateTransaction()
{
    qCDebug(category) << "Destroying SystemdSysupdateTransaction" << m_job->path();
}