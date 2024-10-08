#pragma once

#include <qtmetamacros.h>
#include <resources/AbstractResource.h>

#include "SysupdateInternal.h"

class SystemdSysupdateResource : public AbstractResource
{
    Q_OBJECT

public:
    explicit SystemdSysupdateResource(const Sysupdate::TargetInfo &targetInfo, AbstractResourcesBackend *parent);

    QString appstreamId() const override;
    QList<PackageState> addonsInformation() override;
    QString section() override;
    QString origin() const override;
    QString longDescription() override;
    QString availableVersion() const override;
    QString installedVersion() const override;
    QJsonArray licenses() override;
    quint64 size() override;
    QUrl homepage() override;
    QUrl helpURL() override;
    QUrl bugURL() override;
    QUrl donationURL() override;
    QUrl contributeURL() override;
    bool hasCategory(const QString &category) const override;
    AbstractResource::State state() override;
    QVariant icon() const override;
    QString comment() override;
    QString name() const override;
    QString packageName() const override;
    bool isRemovable() const override;
    AbstractResource::Type type() const override;
    bool canExecute() const override;
    void invokeApplication() const override {};
    void fetchChangelog() override;
    QUrl url() const override;
    QString author() const override;

private:
    const Sysupdate::TargetInfo &m_targetInfo;
};