#ifndef HEALTHCHECKWIDGET_H
#define HEALTHCHECKWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>
#include "healthcheck.h"

class HealthCheckWidget : public QWidget {
    Q_OBJECT

public:
    explicit HealthCheckWidget(HealthChecker *checker, QWidget *parent = nullptr)
        : QWidget(parent), m_checker(checker) {
        setupUI();
    }

    void runCheck() {
        QLayoutItem *item;
        while ((item = m_resultsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        m_issues = m_checker->runCheck();

        int issuesFound = 0;
        for (const auto &issue : m_issues) {
            if (issue.detected) issuesFound++;
            addIssueCard(issue);
        }

        auto *summary = new QLabel();
        summary->setWordWrap(true);
        if (issuesFound == 0) {
            summary->setText("All checks passed. Configuration looks healthy.");
            summary->setStyleSheet(
                "color: #a6e3a1; font-size: 14px; padding: 12px; "
                "background-color: #1e1e2e; border: 1px solid #a6e3a1; border-radius: 8px;");
        } else {
            summary->setText(QString("Found %1 issue(s) that may affect system operation.").arg(issuesFound));
            summary->setStyleSheet(
                "color: #f9e2af; font-size: 14px; padding: 12px; "
                "background-color: #1e1e2e; border: 1px solid #f9e2af; border-radius: 8px;");
        }
        m_resultsLayout->addWidget(summary);
    }

signals:
    void issueFixed();

private:
    HealthChecker *m_checker;
    QVBoxLayout *m_resultsLayout;
    QList<HealthIssue> m_issues;

    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        auto *headerLayout = new QHBoxLayout();
        auto *titleLabel = new QLabel("System Health Check");
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4;");
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        auto *checkBtn = new QPushButton("Run Check");
        checkBtn->setMinimumHeight(36);
        checkBtn->setStyleSheet(
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }");
        connect(checkBtn, &QPushButton::clicked, this, &HealthCheckWidget::runCheck);
        headerLayout->addWidget(checkBtn);

        layout->addLayout(headerLayout);

        auto *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

        auto *resultsWidget = new QWidget();
        m_resultsLayout = new QVBoxLayout(resultsWidget);
        m_resultsLayout->setContentsMargins(0, 10, 0, 0);
        m_resultsLayout->setSpacing(8);
        m_resultsLayout->addStretch();
        scrollArea->setWidget(resultsWidget);
        layout->addWidget(scrollArea);
    }

    void addIssueCard(const HealthIssue &issue) {
        auto *card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);

        QString borderColor = issue.detected ? "#45475a" : "#313244";
        QString statusColor = issue.detected ? "#f38ba8" : "#a6e3a1";
        QString statusIcon = issue.detected ? "X" : "OK";

        card->setStyleSheet(QString(
            "QFrame { background-color: #1e1e2e; border: 1px solid %1; "
            "border-radius: 8px; padding: 12px; }").arg(borderColor));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 8, 12, 8);
        cardLayout->setSpacing(4);

        auto *titleRow = new QHBoxLayout();
        auto *iconLabel = new QLabel(statusIcon);
        iconLabel->setFixedWidth(30);
        iconLabel->setStyleSheet(QString("font-weight: bold; color: %1; font-size: 12px;").arg(statusColor));
        titleRow->addWidget(iconLabel);

        auto *titleLabel = new QLabel(issue.title);
        titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 13px; color: %1;").arg(statusColor));
        titleRow->addWidget(titleLabel);

        auto *categoryLabel = new QLabel(issue.category);
        categoryLabel->setStyleSheet(
            "color: #6c7086; font-size: 11px; padding: 2px 6px; "
            "background-color: #313244; border-radius: 4px;");
        titleRow->addWidget(categoryLabel);
        titleRow->addStretch();

        cardLayout->addLayout(titleRow);

        auto *descLabel = new QLabel(issue.description);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet("color: #a6adc8; font-size: 12px;");
        cardLayout->addWidget(descLabel);

        if (issue.detected && issue.autoFixable && !issue.fixCommand.isEmpty()) {
            auto *fixRow = new QHBoxLayout();
            fixRow->addStretch();

            auto *fixBtn = new QPushButton(QString("Fix: %1").arg(issue.fixDescription));
            fixBtn->setMinimumHeight(30);
            fixBtn->setStyleSheet(
                "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 4px; "
                "padding: 6px 12px; font-weight: bold; font-size: 11px; border: none; }"
                "QPushButton:hover { background-color: #b4f9b8; }");

            QString issueId = issue.id;
            connect(fixBtn, &QPushButton::clicked, this, [this, issueId, card]() {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    card, "Confirm Fix",
                    "Apply this fix? You may be prompted for your password.",
                    QMessageBox::Yes | QMessageBox::No);
                if (reply == QMessageBox::Yes) {
                    if (m_checker->fixIssue(issueId)) {
                        QMessageBox::information(card, "Fixed", "Fix applied successfully.");
                        emit issueFixed();
                        runCheck();
                    } else {
                        QMessageBox::warning(card, "Failed", "Fix failed. Check terminal for details.");
                    }
                }
            });

            fixRow->addWidget(fixBtn);
            cardLayout->addLayout(fixRow);
        }

        m_resultsLayout->insertWidget(m_resultsLayout->count() - 1, card);
    }
};

#endif // HEALTHCHECKWIDGET_H
