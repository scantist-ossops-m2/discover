#pragma once

#include <qdbusargument.h>
#include <qdbusextratypes.h>
#include <qtypes.h>

namespace Sysupdate
{
Q_NAMESPACE

enum class JobType {
    JOB_LIST,
    JOB_DESCRIBE,
    JOB_CHECK_NEW,
    JOB_UPDATE,
    JOB_VACUUM,
};

struct Job {
    quint64 id;
    JobType type;
    quint32 progressPercent;
    QDBusObjectPath objectPath;
};

enum class TargetClass {
    TARGET_APPSTREAM,
    TARGET_SYSUPDATE,
};
Q_ENUM_NS(TargetClass)

struct Target {
    QString targetClass;
    QString name;
    QDBusObjectPath objectPath;
};

typedef QList<Job> JobList;
typedef QList<Target> TargetList;
}

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Target &target);
const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Target &target);
