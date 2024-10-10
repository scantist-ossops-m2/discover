#include "SysupdateInternal.h"
#include <QMetaEnum>
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

QDBusArgument &operator<<(QDBusArgument &argument, const Sysupdate::Target &target)
{
    argument.beginStructure();
    argument << QMetaEnum::fromType<Sysupdate::TargetClass>().key(static_cast<int>(target.targetClass)) << target.name << target.objectPath;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Sysupdate::Target &target)
{
    QString targetClass;
    argument.beginStructure();
    argument >> targetClass >> target.name >> target.objectPath;
    target.targetClass = static_cast<Sysupdate::TargetClass>(QMetaEnum::fromType<Sysupdate::TargetClass>().keyToValue(targetClass.toUtf8().constData()));
    argument.endStructure();
    return argument;
}

Q_DECLARE_METATYPE(Sysupdate::Target)
Q_DECLARE_METATYPE(Sysupdate::TargetList)
