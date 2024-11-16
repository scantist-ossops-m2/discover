/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <attica/provider.h>

class KNSBackend;
class QUrl;
namespace Attica
{
class BaseJob;
}

class KNSReviews : public AbstractReviewsBackend
{
    Q_OBJECT
public:
    explicit KNSReviews(KNSBackend *backend);

    ReviewsJob *fetchReviews(AbstractResource *resource, int page = 1) override;
    bool isFetching() const override;
    void flagReview(Review *review, const QString &reason, const QString &text) override;
    void deleteReview(Review *review) override;
    void submitUsefulness(Review *review, bool useful) override;
    void logout() override;
    void registerAndLogin() override;
    void login() override;
    Rating ratingForApplication(AbstractResource *resource) const override;
    bool hasCredentials() const override;

    bool isResourceSupported(AbstractResource *resource) const override;

protected:
    ReviewsJob *
    sendReview(AbstractResource *resource, const QString &summary, const QString &reviewText, const QString &rating, const QString &userName) override;
    QString userName() const override;

private Q_SLOTS:
    void credentialsReceived(const QString &user, const QString &password);

private:
    Attica::Provider provider() const;
    void acquireFetching(bool f);

    KNSBackend *const m_backend;
    int m_fetching = 0;
};
