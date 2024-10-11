#pragma once

#include <qtmetamacros.h>
#include <resources/AbstractResource.h>

#include "SysupdateInternal.h"

#include <AppStreamQt/component.h>

class SystemdSysupdateResource : public AbstractResource
{
    Q_OBJECT

public:
    SystemdSysupdateResource(AppStream::Component component, AbstractResourcesBackend *parent);
    QString packageName() const override;
    QString name() const override;
    QString comment() override;
    QVariant icon() const override;
    bool canExecute() const override;
    void invokeApplication() const override;
    State state() override;
    bool hasCategory(const QString &category) const override;
    Type type() const override;
    quint64 size() override;
    QJsonArray licenses() override;
    QString installedVersion() const override;
    QString availableVersion() const override;
    QString longDescription() override;
    QString origin() const override;
    QString section() override;
    QString author() const override;
    QList<PackageState> addonsInformation() override;
    QString sourceIcon() const override;
    QDate releaseDate() const override;
    void fetchChangelog() override;

private:
    AppStream::Component m_component;
};