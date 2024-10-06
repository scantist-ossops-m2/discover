#include "SysupdateInternal.h"
#include <qdbusargument.h>
#include <qdbusmetatype.h>

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Job &job)
{
    argument.beginStructure();
    argument << job.id << static_cast<int>(job.type) << job.progressPercent << job.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Job &job)
{
    int type;
    argument.beginStructure();
    argument >> job.id >> type >> job.progressPercent >> job.objectPath;
    job.type = static_cast<Sysupdate::JobType>(type);
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(Sysupdate::Job)
Q_DECLARE_METATYPE(Sysupdate::JobList)
