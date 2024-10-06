#pragma once

#include <qdbusextratypes.h>
#include <qtypes.h>

namespace Sysupdate
{

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

struct Target {
    TargetClass targetClass;
    QString name;
    QDBusObjectPath objectPath;
};

typedef QList<Sysupdate::Job> JobList;
typedef QList<Sysupdate::Target> TargetList;

}