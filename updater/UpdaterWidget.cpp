/***************************************************************************
 *   Copyright © 2010 Jonathan Thomas <echidnaman@kubuntu.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "UpdaterWidget.h"

#include <KLocale>

#include "../libmuon/DetailsWidget.h"
#include <LibQApt/Backend>

UpdaterWidget::UpdaterWidget(QWidget *parent)
    : PackageWidget(parent)
{
    setPackagesType(PackageWidget::MarkedPackages);

    QLabel *headerLabel = new QLabel(this);
    headerLabel->setTextFormat(Qt::RichText);
    headerLabel->setText(i18n("<b>Update Packages</b>"));
    setHeaderWidget(headerLabel);
}

void UpdaterWidget::setBackend(QApt::Backend *backend)
{
    backend->markPackagesForDistUpgrade();
    PackageWidget::setBackend(backend);
}

void UpdaterWidget::setPackages()
{
    m_detailsWidget->hide();
    PackageWidget::setPackages();
}

UpdaterWidget::~UpdaterWidget()
{
}

#include "UpdaterWidget.moc"
