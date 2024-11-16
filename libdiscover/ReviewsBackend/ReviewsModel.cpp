/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "ReviewsBackend/ReviewsModel.h"
#include "libdiscover_debug.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QtQml/qqmllist.h>
#include <ReviewsBackend/AbstractReviewsBackend.h>
#include <ReviewsBackend/Review.h>
#include <ksharedconfig.h>
#include <resources/AbstractResource.h>
#include <resources/AbstractResourcesBackend.h>
#include <resources/ResourcesModel.h>

using namespace Qt::StringLiterals;

int StarsCount::one() const
{
    return m_one;
}

int StarsCount::two() const
{
    return m_two;
}

int StarsCount::three() const
{
    return m_three;
}

int StarsCount::four() const
{
    return m_four;
}

int StarsCount::five() const
{
    return m_five;
}

void StarsCount::addRating(int rating)
{
    // Ratings are 1-10, but we show only 5 stars
    switch (int(std::ceil(qreal(rating) / 2.0))) {
    case 1:
        ++m_one;
        break;
    case 2:
        ++m_two;
        break;
    case 3:
        ++m_three;
        break;
    case 4:
        ++m_four;
        break;
    case 5:
        ++m_five;
        break;
    default:
        break;
    }
}

void StarsCount::clear()
{
    m_one = 0;
    m_two = 0;
    m_three = 0;
    m_four = 0;
    m_five = 0;
}

ReviewsModel::ReviewsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_lastPage(0)
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup configGroup(config, u"Reviews"_s);
    const QString role = configGroup.readEntry("PreferredSortRole");
    if (QStringList({QStringLiteral("wilsonScore"), QStringLiteral("date"), QStringLiteral("rating")}).contains(role)) {
        m_preferredSortRole = role;
    } else {
        m_preferredSortRole = QStringLiteral("wilsonScore");
    }
}

ReviewsModel::~ReviewsModel() = default;

QHash<int, QByteArray> ReviewsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles.insert(ShouldShow, "shouldShow");
    roles.insert(Reviewer, "reviewer");
    roles.insert(CreationDate, "date");
    roles.insert(UsefulnessTotal, "usefulnessTotal");
    roles.insert(UsefulnessFavorable, "usefulnessFavorable");
    roles.insert(WilsonScore, "wilsonScore");
    roles.insert(UsefulChoice, "usefulChoice");
    roles.insert(Rating, "rating");
    roles.insert(Summary, "summary");
    roles.insert(Depth, "depth");
    roles.insert(PackageVersion, "packageVersion");
    return roles;
}

QVariant ReviewsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    const auto &review = m_reviews.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return review->reviewText();
    case ShouldShow:
        return review->shouldShow();
    case Reviewer:
        return review->reviewer();
    case CreationDate:
        return review->creationDate();
    case UsefulnessTotal:
        return review->usefulnessTotal();
    case UsefulnessFavorable:
        return review->usefulnessFavorable();
    case WilsonScore:
        return review->wilsonScore();
    case UsefulChoice:
        return review->usefulChoice();
    case Rating:
        return review->rating();
    case Summary:
        return review->summary();
    case PackageVersion:
        return review->packageVersion();
    case Depth:
        return review->getMetadata(QStringLiteral("NumberOfParents")).toInt();
    }
    return QVariant();
}

int ReviewsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_reviews.count();
}

QString ReviewsModel::preferredSortRole() const
{
    return m_preferredSortRole;
}

void ReviewsModel::setPreferredSortRole(const QString &sorting)
{
    if (m_preferredSortRole == sorting || !QStringList({QStringLiteral("wilsonScore"), QStringLiteral("date"), QStringLiteral("rating")}).contains(sorting)) {
        return;
    }

    m_preferredSortRole = sorting;
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup configGroup(config, u"Reviews"_s);
    configGroup.writeEntry("PreferredSortRole", sorting);

    Q_EMIT preferredSortRoleChanged();
}

AbstractResource *ReviewsModel::resource() const
{
    return m_app;
}

AbstractReviewsBackend *ReviewsModel::backend() const
{
    return m_backend;
}

void ReviewsModel::setResource(AbstractResource *app)
{
    if (m_app != app) {
        beginResetModel();
        m_starsCount.clear();
        m_reviews.clear();
        m_lastPage = 0;

        if (m_backend) {
            disconnect(m_backend, &AbstractReviewsBackend::errorMessageChanged, this, &ReviewsModel::restartFetching);
            disconnect(m_backend, &AbstractReviewsBackend::fetchingChanged, this, &ReviewsModel::fetchingChanged);
            disconnect(m_app, &AbstractResource::versionsChanged, this, &ReviewsModel::restartFetching);
        }
        m_app = app;
        m_backend = app ? app->backend()->reviewsBackend() : nullptr;
        if (m_backend) {
            connect(m_backend, &AbstractReviewsBackend::errorMessageChanged, this, &ReviewsModel::restartFetching);
            connect(m_backend, &AbstractReviewsBackend::fetchingChanged, this, &ReviewsModel::fetchingChanged);
            connect(m_app, &AbstractResource::versionsChanged, this, &ReviewsModel::restartFetching);

            QMetaObject::invokeMethod(this, &ReviewsModel::restartFetching, Qt::QueuedConnection);
        }
        endResetModel();
        Q_EMIT rowsChanged();
        Q_EMIT resourceChanged();
    }
}

void ReviewsModel::restartFetching()
{
    if (!m_app || !m_backend) {
        return;
    }

    m_canFetchMore = true;
    m_lastPage = 0;
    fetchMore();
    Q_EMIT rowsChanged();
    Q_EMIT fetchingChanged(m_job);
}

void ReviewsModel::fetchMore(const QModelIndex &parent)
{
    if (!m_backend || !m_app || parent.isValid() || !m_canFetchMore) {
        return;
    }

    if (m_job) {
        return;
    }

    m_lastPage++;
    setReviewsJob(m_backend->fetchReviews(m_app, m_lastPage));
    // qCDebug(LIBDISCOVER_LOG) << "fetching reviews... " << m_lastPage;
}

void ReviewsModel::addReviews(const QVector<ReviewPtr> &reviews, bool canFetchMore)
{
    m_canFetchMore = canFetchMore;
    qCDebug(LIBDISCOVER_LOG) << "reviews arrived..." << m_lastPage << reviews.size();

    if (!reviews.isEmpty()) {
        for (ReviewPtr review : reviews) {
            m_starsCount.addRating(review->rating());
        }
        beginInsertRows(QModelIndex(), rowCount(), rowCount() + reviews.size() - 1);
        m_reviews += reviews;
        endInsertRows();
        Q_EMIT rowsChanged();
    }
}

bool ReviewsModel::canFetchMore(const QModelIndex & /*parent*/) const
{
    return m_canFetchMore;
}

void ReviewsModel::markUseful(int row, bool useful)
{
    Review *r = m_reviews[row].data();
    r->setUsefulChoice(useful ? Yes : No);
    // qCDebug(LIBDISCOVER_LOG) << "submitting usefulness" << r->applicationName() << r->id() << useful;
    m_backend->submitUsefulness(r, useful);
    const QModelIndex ind = index(row, 0, QModelIndex());
    Q_EMIT dataChanged(ind, ind, {UsefulnessTotal, UsefulnessFavorable, UsefulChoice});
}

void ReviewsModel::deleteReview(int row)
{
    Review *r = m_reviews[row].data();
    m_backend->deleteReview(r);
}

void ReviewsModel::flagReview(int row, const QString &reason, const QString &text)
{
    Review *r = m_reviews[row].data();
    m_backend->flagReview(r, reason, text);
}

StarsCount ReviewsModel::starsCount() const
{
    return m_starsCount;
}

bool ReviewsModel::isFetching() const
{
    return m_job;
}

void ReviewsModel::setReviewsJob(ReviewsJob *job)
{
    if (job == m_job) {
        return;
    }

    if (m_job) {
        disconnect(m_job, &QObject::destroyed, this, nullptr);
    }
    Q_ASSERT(job);
    connect(job, &ReviewsJob::reviewsReady, this, &ReviewsModel::addReviews);
    connect(job, &QObject::destroyed, this, [this] {
        Q_EMIT fetchingChanged(false);
    });
    m_job = job;
    Q_EMIT fetchingChanged(true);
}

#include "moc_ReviewsModel.cpp"
