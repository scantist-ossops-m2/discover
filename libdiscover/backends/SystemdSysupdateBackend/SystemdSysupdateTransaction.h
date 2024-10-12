//
// Created by fernie on 10/11/24.
//

#pragma once

#include <Transaction/Transaction.h>
#include <sysupdate1.h>

const auto SYSUPDATE1_SERVICE = QStringLiteral("org.freedesktop.sysupdate1");

class SystemdSysupdateTransaction : public Transaction
{
public:
    SystemdSysupdateTransaction(AbstractResource *resource, qulonglong id, const QString &path);

    void cancel() override;
    ~SystemdSysupdateTransaction() override;

private:
    org::freedesktop::sysupdate1::Job *m_job;
    org::freedesktop::DBus::Properties *m_properties;
};
