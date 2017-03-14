/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhelpsearchresultwidget.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QPointer>
#include <QtCore/QStringList>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLayout>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QTreeWidgetItem>

QT_BEGIN_NAMESPACE

class QResultWidget : public QTextBrowser
{
    Q_OBJECT

public:
    QResultWidget(QWidget *parent = 0)
        : QTextBrowser(parent)
    {
        connect(this, &QTextBrowser::anchorClicked,
                this, &QResultWidget::requestShowLink);
        setContextMenuPolicy(Qt::NoContextMenu);
    }

    void showResultPage(const QVector<QHelpSearchResult> results, bool isIndexing)
    {
        QString htmlFile = QString(QLatin1String("<html><head><title>%1"
            "</title></head><body>")).arg(tr("Search Results"));

        int count = results.count();
        if (count != 0) {
            if (isIndexing)
                htmlFile += QString(QLatin1String("<div style=\"text-align:left;"
                    " font-weight:bold; color:red\">"
                    "%1&nbsp;<span style=\"font-weight:normal; color:black\">"
                    "%2</span></div></div><br>")).arg(tr("Note:"))
                    .arg(tr("The search results may not be complete since the "
                            "documentation is still being indexed."));

            for (const QHelpSearchResult &result : results) {
                htmlFile += QString(QLatin1String("<div style=\"text-align:left;"
                    " font-weight:bold\"><a href=\"%1\">%2</a>"
                    "<div style=\"color:green; font-weight:normal;"
                    " margin:5px\">%3</div></div><p></p>"))
                        .arg(result.url().toString(), result.title(),
                             result.snippet());
            }
        } else {
            htmlFile += QLatin1String("<div align=\"center\"><br><br><h2>")
                    + tr("Your search did not match any documents.")
                    + QLatin1String("</h2><div>");
            if (isIndexing)
                htmlFile += QLatin1String("<div align=\"center\"><h3>")
                    + tr("(The reason for this might be that the documentation "
                         "is still being indexed.)")
                    + QLatin1String("</h3><div>");
        }

        htmlFile += QLatin1String("</body></html>");

        setHtml(htmlFile);
    }

signals:
    void requestShowLink(const QUrl &url);

private slots:
    void setSource(const QUrl & /* name */) override {}
};


class QHelpSearchResultWidgetPrivate : public QObject
{
    Q_OBJECT

private slots:
    void setResults(int hitsCount)
    {
        if (!searchEngine.isNull()) {
            showFirstResultPage();
            updateNextButtonState(((hitsCount > 20) ? true : false));
        }
    }

    void showNextResultPage()
    {
        if (!searchEngine.isNull()
            && resultLastToShow < searchEngine->searchResultCount()) {
            resultLastToShow += 20;
            resultFirstToShow += 20;

            resultTextBrowser->showResultPage(searchEngine->searchResults(resultFirstToShow,
                resultLastToShow), isIndexing);
            if (resultLastToShow >= searchEngine->searchResultCount())
                updateNextButtonState(false);
        }
        updateHitRange();
    }

    void showLastResultPage()
    {
        if (!searchEngine.isNull()) {
            resultLastToShow = searchEngine->searchResultCount();
            resultFirstToShow = resultLastToShow - (resultLastToShow % 20);

            if (resultFirstToShow == resultLastToShow)
                resultFirstToShow -= 20;

            resultTextBrowser->showResultPage(searchEngine->searchResults(resultFirstToShow,
                resultLastToShow), isIndexing);
            updateNextButtonState(false);
        }
        updateHitRange();
    }

    void showFirstResultPage()
    {
        if (!searchEngine.isNull()) {
            resultLastToShow = 20;
            resultFirstToShow = 0;

            resultTextBrowser->showResultPage(searchEngine->searchResults(resultFirstToShow,
                resultLastToShow), isIndexing);
            updatePrevButtonState(false);
        }
        updateHitRange();
    }

    void showPreviousResultPage()
    {
        if (!searchEngine.isNull()) {
            int count = resultLastToShow % 20;
            if (count == 0 || resultLastToShow != searchEngine->searchResultCount())
                count = 20;

            resultLastToShow -= count;
            resultFirstToShow = resultLastToShow -20;

            resultTextBrowser->showResultPage(searchEngine->searchResults(resultFirstToShow,
                resultLastToShow), isIndexing);
            if (resultFirstToShow == 0)
                updatePrevButtonState(false);
        }
        updateHitRange();
    }

    void updatePrevButtonState(bool state = true)
    {
        firstResultPage->setEnabled(state);
        previousResultPage->setEnabled(state);
    }

    void updateNextButtonState(bool state = true)
    {
        nextResultPage->setEnabled(state);
        lastResultPage->setEnabled(state);
    }

    void indexingStarted()
    {
        isIndexing = true;
    }

    void indexingFinished()
    {
        isIndexing = false;
    }

private:
    QHelpSearchResultWidgetPrivate(QHelpSearchEngine *engine)
        : QObject()
        , searchEngine(engine)
        , isIndexing(false)
    {
        resultTextBrowser = 0;

        resultLastToShow = 20;
        resultFirstToShow = 0;

        firstResultPage = 0;
        previousResultPage = 0;
        hitsLabel = 0;
        nextResultPage = 0;
        lastResultPage = 0;

        connect(searchEngine.data(), &QHelpSearchEngine::indexingStarted,
                this, &QHelpSearchResultWidgetPrivate::indexingStarted);
        connect(searchEngine.data(), &QHelpSearchEngine::indexingFinished,
                this, &QHelpSearchResultWidgetPrivate::indexingFinished);
    }

    ~QHelpSearchResultWidgetPrivate()
    {
        delete searchEngine;
    }

    QToolButton* setupToolButton(const QString &iconPath)
    {
        QToolButton *button = new QToolButton();
        button->setEnabled(false);
        button->setAutoRaise(true);
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(12, 12));
        button->setMaximumSize(QSize(16, 16));

        return button;
    }

    void updateHitRange()
    {
        int last = 0;
        int first = 0;
        int count = 0;

        if (!searchEngine.isNull()) {
            count = searchEngine->searchResultCount();
            if (count > 0) {
                first = resultFirstToShow + 1;
                last = resultLastToShow > count ? count : resultLastToShow;
            }
        }
        hitsLabel->setText(QHelpSearchResultWidget::tr("%1 - %2 of %n Hits", 0, count).arg(first).arg(last));
    }

private:
    friend class QHelpSearchResultWidget;

    QPointer<QHelpSearchEngine> searchEngine;

    QResultWidget *resultTextBrowser;

    int resultLastToShow;
    int resultFirstToShow;
    bool isIndexing;

    QToolButton *firstResultPage;
    QToolButton *previousResultPage;
    QLabel *hitsLabel;
    QToolButton *nextResultPage;
    QToolButton *lastResultPage;
};

#include "qhelpsearchresultwidget.moc"


/*!
    \class QHelpSearchResultWidget
    \since 4.4
    \inmodule QtHelp
    \brief The QHelpSearchResultWidget class provides a text browser to display
    search results.
*/

/*!
    \fn void QHelpSearchResultWidget::requestShowLink(const QUrl &link)

    This signal is emitted when a item is activated and its associated
    \a link should be shown.
*/

QHelpSearchResultWidget::QHelpSearchResultWidget(QHelpSearchEngine *engine)
    : QWidget(0)
    , d(new QHelpSearchResultWidgetPrivate(engine))
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);

    QHBoxLayout *hBoxLayout = new QHBoxLayout();
#ifndef Q_OS_MAC
    hBoxLayout->setMargin(0);
    hBoxLayout->setSpacing(0);
#endif
    hBoxLayout->addWidget(d->firstResultPage = d->setupToolButton(
        QString::fromUtf8(":/qt-project.org/assistant/images/3leftarrow.png")));

    hBoxLayout->addWidget(d->previousResultPage = d->setupToolButton(
        QString::fromUtf8(":/qt-project.org/assistant/images/1leftarrow.png")));

    d->hitsLabel = new QLabel(tr("0 - 0 of 0 Hits"), this);
    d->hitsLabel->setEnabled(false);
    hBoxLayout->addWidget(d->hitsLabel);
    d->hitsLabel->setAlignment(Qt::AlignCenter);
    d->hitsLabel->setMinimumSize(QSize(150, d->hitsLabel->height()));

    hBoxLayout->addWidget(d->nextResultPage = d->setupToolButton(
        QString::fromUtf8(":/qt-project.org/assistant/images/1rightarrow.png")));

    hBoxLayout->addWidget(d->lastResultPage = d->setupToolButton(
        QString::fromUtf8(":/qt-project.org/assistant/images/3rightarrow.png")));

    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hBoxLayout->addItem(spacer);

    vLayout->addLayout(hBoxLayout);

    d->resultTextBrowser = new QResultWidget(this);
    vLayout->addWidget(d->resultTextBrowser);

    connect(d->resultTextBrowser, &QResultWidget::requestShowLink,
            this, &QHelpSearchResultWidget::requestShowLink);

    connect(d->nextResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::showNextResultPage);
    connect(d->lastResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::showLastResultPage);
    connect(d->firstResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::showFirstResultPage);
    connect(d->previousResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::showPreviousResultPage);

    connect(d->firstResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::updateNextButtonState);
    connect(d->previousResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::updateNextButtonState);
    connect(d->nextResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::updatePrevButtonState);
    connect(d->lastResultPage, &QAbstractButton::clicked,
            d, &QHelpSearchResultWidgetPrivate::updatePrevButtonState);

    connect(engine, &QHelpSearchEngine::searchingFinished,
            d, &QHelpSearchResultWidgetPrivate::setResults);
}

/*! \reimp
*/
void QHelpSearchResultWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        d->setResults(d->searchEngine->searchResultCount());
}

/*!
    Destroys the search result widget.
*/
QHelpSearchResultWidget::~QHelpSearchResultWidget()
{
    delete d;
}

/*!
    Returns a reference of the URL that the item at \a point owns, or an
    empty URL if no item exists at that point.
*/
QUrl QHelpSearchResultWidget::linkAt(const QPoint &point)
{
    QUrl url;
    if (d->resultTextBrowser)
        url = d->resultTextBrowser->anchorAt(point);
    return url;
}

QT_END_NAMESPACE
