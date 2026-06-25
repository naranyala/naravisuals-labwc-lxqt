#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QString>

struct WindowRule {
    QString identifier;
    QString title;
    QString matchOnce;
    QString skipTaskbar;
    QString skipWindowSwitcher;
    QString decorate;
    QString type;
    QString monitor;
    QString workspace;
};

class WindowRulesTab : public QWidget {
    Q_OBJECT
public:
    explicit WindowRulesTab(QWidget *parent = nullptr);
signals:
    void statusMessage(const QString &msg);
private slots:
    void loadRules();
    void addRule();
    void editRule();
    void removeRule();
    void saveRules();
private:
    void populateTable();
    bool editRuleDialog(WindowRule &r, bool isNew);
    QTableWidget *table;
    QList<WindowRule> rules;
    QString rcPath;
};
