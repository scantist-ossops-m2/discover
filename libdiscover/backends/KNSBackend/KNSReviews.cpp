/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "KNSReviews.h"
#include "KNSBackend.h"
#include "KNSResource.h"
#include <KLocalizedString>
#include <KNSCore/EngineBase>
#include <KPasswordDialog>
#include <QDebug>
#include <QDesktopServices>
#include <ReviewsBackend/Rating.h>
#include <ReviewsBackend/Review.h>
#include <attica/content.h>
#include <attica/providermanager.h>
#include <resources/AbstractResource.h>

static QVector<ReviewPtr> createReviewList(AbstractResource *resource, const Attica::Comment::List comments, int depth = 0)
{
    QVector<ReviewPtr> reviews;
    for (const Attica::Comment &comment : comments) {
        // TODO: language lookup?
        ReviewPtr r(new Review(resource->name(),
                               resource->packageName(),
                               QStringLiteral("en"),
                               comment.subject(),
                               comment.text(),
                               comment.user(),
                               comment.date(),
                               true,
                               comment.id().toInt(),
                               comment.score() / 10,
                               0,
                               0,
                               0.0,
                               QString()));
        r->addMetadata(QStringLiteral("NumberOfParents"), depth);
        reviews += r;
        if (comment.childCount() > 0) {
            reviews += createReviewList(resource, comment.children(), depth + 1);
        }
    }
    return reviews;
}

KNSReviews::KNSReviews(KNSBackend *backend)
    : AbstractReviewsBackend(backend)
    , m_backend(backend)
{
}

Rating KNSReviews::ratingForApplication(AbstractResource *resource) const
{
    if (resource) {
        return resource->rating();
    }
    return {};
}

ReviewsJob *KNSReviews::fetchReviews(AbstractResource *resource, int page)
{
    const auto job = provider().requestComments(Attica::Comment::ContentComment, resource->packageName(), QStringLiteral("0"), page - 1, 10);
    if (!job) {
        auto r = new ReviewsJob;
        r->deleteLater();
        return r;
    }
    auto reviewJob = new ReviewsJob;
    connect(job, &Attica::BaseJob::finished, this, [job, resource, reviewJob] {
        QVector<ReviewPtr> reviews = createReviewList(resource, job->itemList());

        Q_EMIT reviewJob->reviewsReady(reviews, !reviews.isEmpty());
        reviewJob->deleteLater();
    });
    job->start();
    return reviewJob;
}

void KNSReviews::acquireFetching(bool fetching)
{
    if (fetching) {
        m_fetching++;
    } else {
        m_fetching--;
    }

    if ((!fetching && m_fetching == 0) || (fetching && m_fetching == 1)) {
        Q_EMIT fetchingChanged(m_fetching != 0);
    }
    Q_ASSERT(m_fetching >= 0);
}

bool KNSReviews::isFetching() const
{
    return m_fetching > 0;
}

void KNSReviews::flagReview(Review * /*r*/, const QString & /*reason*/, const QString & /*text*/)
{
    qWarning() << "cannot flag reviews";
}

void KNSReviews::deleteReview(Review * /*r*/)
{
    qWarning() << "cannot delete comments";
}

ReviewsJob *
KNSReviews::sendReview(AbstractResource *resource, const QString &summary, const QString &reviewText, const QString &rating, const QString &userName)
{
    Q_UNUSED(userName);
    provider().voteForContent(resource->packageName(), rating.toUInt() * 20);
    if (!summary.isEmpty()) {
        provider().addNewComment(Attica::Comment::ContentComment, resource->packageName(), QString(), QString(), summary, reviewText);
    }

    auto job = new ReviewsJob;
    job->deleteLater();
    return job;
}

void KNSReviews::submitUsefulness(Review *review, bool useful)
{
    provider().voteForComment(QString::number(review->id()), useful * 5);
}

void KNSReviews::logout()
{
    bool ok = provider().saveCredentials(QString(), QString());
    if (!ok) {
        qWarning() << "couldn't log out";
    }
}

void KNSReviews::registerAndLogin()
{
    QDesktopServices::openUrl(provider().baseUrl());
}

void KNSReviews::login()
{
    const auto dialog = new KPasswordDialog;
    dialog->setPrompt(i18n("Log in information for %1", provider().name()));
    connect(dialog, &KPasswordDialog::gotUsernameAndPassword, this, &KNSReviews::credentialsReceived);
}

void KNSReviews::credentialsReceived(const QString &user, const QString &password)
{
    bool ok = provider().saveCredentials(user, password);
    if (!ok) {
        qWarning() << "couldn't save" << user << "credentials for" << provider().name();
    }
}

bool KNSReviews::hasCredentials() const
{
    return provider().hasCredentials();
}

QString KNSReviews::userName() const
{
    QString user, password;
    provider().loadCredentials(user, password);
    return user;
}

Attica::Provider KNSReviews::provider() const
{
    if (m_backend->engine()->atticaProviders().isEmpty()) {
        return {};
    }
    return *m_backend->engine()->atticaProviders().constFirst();
}

bool KNSReviews::isResourceSupported(AbstractResource *resource) const
{
    return qobject_cast<KNSResource *>(resource);
}

#include "moc_KNSReviews.cpp"
