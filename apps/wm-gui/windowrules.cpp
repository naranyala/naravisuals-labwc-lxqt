#include "windowrules.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QProcess>
#include <QRegularExpression>

WindowRulesTab::WindowRulesTab(QWidget *parent) : QWidget(parent) {
    rcPath = QDir::homePath() + "/.config/labwc/rc.xml";

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *header = new QLabel("Per-app window rules — match by identifier/title and set properties");
    header->setStyleSheet("font-size: 14px; color: #a6adc8;");
    layout->addWidget(header);

    table = new QTableWidget();
    table->setColumnCount(9);
    table->setHorizontalHeaderLabels({"Identifier", "Title", "Skip Taskbar", "Skip Switcher", "Decorate", "Type", "Monitor", "Workspace", "Match Once"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    for (int i = 2; i < 9; ++i)
        table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    table->setStyleSheet(
        "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
        "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 4px; border: 1px solid #313244; font-weight: bold; }");
    layout->addWidget(table);

    auto *btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    auto *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setMinimumHeight(34);
    connect(refreshBtn, &QPushButton::clicked, this, &WindowRulesTab::loadRules);
    btnRow->addWidget(refreshBtn);

    auto *addBtn = new QPushButton("Add Rule");
    addBtn->setMinimumHeight(34);
    addBtn->setStyleSheet("QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 8px; font-weight: bold; border: none; }");
    connect(addBtn, &QPushButton::clicked, this, &WindowRulesTab::addRule);
    btnRow->addWidget(addBtn);

    auto *editBtn = new QPushButton("Edit");
    editBtn->setMinimumHeight(34);
    connect(editBtn, &QPushButton::clicked, this, &WindowRulesTab::editRule);
    btnRow->addWidget(editBtn);

    auto *removeBtn = new QPushButton("Remove");
    removeBtn->setMinimumHeight(34);
    removeBtn->setStyleSheet("QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 8px; font-weight: bold; border: none; }");
    connect(removeBtn, &QPushButton::clicked, this, &WindowRulesTab::removeRule);
    btnRow->addWidget(removeBtn);

    btnRow->addStretch();

    auto *saveBtn = new QPushButton("Save && Reconfigure");
    saveBtn->setMinimumHeight(34);
    saveBtn->setStyleSheet("QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 8px; font-weight: bold; border: none; }");
    connect(saveBtn, &QPushButton::clicked, this, &WindowRulesTab::saveRules);
    btnRow->addWidget(saveBtn);

    layout->addLayout(btnRow);
    loadRules();
}

void WindowRulesTab::loadRules() {
    rules.clear();
    QFile f(rcPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusMessage("Cannot open " + rcPath);
        return;
    }
    QString content = f.readAll();
    f.close();

    QRegularExpression wrRe(QStringLiteral(R"re(<windowRules>(.*?)</windowRules>)re"), QRegularExpression::DotMatchesEverythingOption);
    auto wrMatch = wrRe.match(content);
    if (!wrMatch.hasMatch()) {
        emit statusMessage("No <windowRules> section found");
        populateTable();
        return;
    }
    QString wrSection = wrMatch.captured(1);

    QRegularExpression ruleRe(QStringLiteral(R"re(<windowRule(?:\s+identifier="([^"]*)")?(?:\s+title="([^"]*)")?(?:\s+matchOnce="([^"]*)")?\s*>(.*?)</windowRule>)re"), QRegularExpression::DotMatchesEverythingOption);
    auto it = ruleRe.globalMatch(wrSection);
    while (it.hasNext()) {
        auto m = it.next();
        WindowRule rule;
        rule.identifier = m.captured(1);
        rule.title = m.captured(2);
        rule.matchOnce = m.captured(3);
        QString body = m.captured(4);

        auto extract = [&](const QString &tag) -> QString {
            QRegularExpression re(QString("<%1>([^<]*)</%1>").arg(tag));
            auto mm = re.match(body);
            return mm.hasMatch() ? mm.captured(1).trimmed() : "";
        };
        rule.skipTaskbar = extract("skipTaskbar");
        rule.skipWindowSwitcher = extract("skipWindowSwitcher");
        rule.decorate = extract("decorate");
        rule.type = extract("type");
        rule.monitor = extract("monitor");
        rule.workspace = extract("workspace");
        rules.append(rule);
    }
    populateTable();
    emit statusMessage(QString("Loaded %1 window rules").arg(rules.size()));
}

void WindowRulesTab::populateTable() {
    table->setRowCount(0);
    for (const auto &r : rules) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(r.identifier.isEmpty() ? "*" : r.identifier));
        table->setItem(row, 1, new QTableWidgetItem(r.title));
        table->setItem(row, 2, new QTableWidgetItem(r.skipTaskbar));
        table->setItem(row, 3, new QTableWidgetItem(r.skipWindowSwitcher));
        table->setItem(row, 4, new QTableWidgetItem(r.decorate));
        table->setItem(row, 5, new QTableWidgetItem(r.type));
        table->setItem(row, 6, new QTableWidgetItem(r.monitor));
        table->setItem(row, 7, new QTableWidgetItem(r.workspace));
        table->setItem(row, 8, new QTableWidgetItem(r.matchOnce));
    }
}

void WindowRulesTab::addRule() {
    WindowRule r;
    if (!editRuleDialog(r, true)) return;
    rules.append(r);
    populateTable();
    emit statusMessage("Rule added");
}

void WindowRulesTab::editRule() {
    int row = table->currentRow();
    if (row < 0 || row >= rules.size()) { emit statusMessage("Select a rule first"); return; }
    if (editRuleDialog(rules[row], false))
        populateTable();
}

bool WindowRulesTab::editRuleDialog(WindowRule &r, bool isNew) {
    bool ok;
    QString id = QInputDialog::getText(this, isNew ? "New Rule" : "Edit Rule",
        "Window identifier (e.g. firefox, *):", QLineEdit::Normal, r.identifier, &ok);
    if (!ok) return false;
    r.identifier = id;

    QString title = QInputDialog::getText(this, "Title Filter",
        "Title pattern (leave empty for all):", QLineEdit::Normal, r.title, &ok);
    if (!ok) return false;
    r.title = title;

    QStringList choices = {"", "yes", "no"};
    auto pick = [&](const QString &label, const QString &current) -> QString {
        QStringList items = choices;
        int idx = items.indexOf(current);
        if (idx < 0) idx = 0;
        bool ok2;
        QString choice = QInputDialog::getItem(this, label, "Value:", items, idx, false, &ok2);
        return ok2 ? choice : current;
    };

    r.skipTaskbar = pick("Skip Taskbar", r.skipTaskbar);
    r.skipWindowSwitcher = pick("Skip Window Switcher", r.skipWindowSwitcher);
    r.decorate = pick("Decorate", r.decorate);

    QStringList typeChoices = {"", "desktop", "dock", "dialog", "splash", "toolbar", "menu"};
    int typeIdx = typeChoices.indexOf(r.type);
    if (typeIdx < 0) typeIdx = 0;
    QString type = QInputDialog::getItem(this, "Window Type", "Type:", typeChoices, typeIdx, false, &ok);
    if (!ok) return false;
    r.type = type;

    QString monitor = QInputDialog::getText(this, "Monitor",
        "Monitor (e.g. DP-1, leave empty for any):", QLineEdit::Normal, r.monitor, &ok);
    if (!ok) return false;
    r.monitor = monitor;

    QString workspace = QInputDialog::getText(this, "Workspace",
        "Workspace (e.g., 1, leave empty for current):", QLineEdit::Normal, r.workspace, &ok);
    if (!ok) return false;
    r.workspace = workspace;

    QStringList onceChoices = {"", "yes", "no"};
    int onceIdx = onceChoices.indexOf(r.matchOnce);
    if (onceIdx < 0) onceIdx = 0;
    QString matchOnce = QInputDialog::getItem(this, "Match Once", "Match Once:", onceChoices, onceIdx, false, &ok);
    if (!ok) return false;
    r.matchOnce = matchOnce;

    return true;
}

void WindowRulesTab::removeRule() {
    int row = table->currentRow();
    if (row < 0 || row >= rules.size()) { emit statusMessage("Select a rule first"); return; }
    auto reply = QMessageBox::question(this, "Confirm", "Remove rule for '" + rules[row].identifier + "'?",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        rules.removeAt(row);
        populateTable();
        emit statusMessage("Rule removed");
    }
}

void WindowRulesTab::saveRules() {
    QFile f(rcPath);
    if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) {
        emit statusMessage("Cannot write " + rcPath);
        return;
    }
    QString content = f.readAll();

    QString wrSection;
    wrSection += "  <windowRules>\n";
    for (const auto &r : rules) {
        wrSection += "    <windowRule";
        if (!r.identifier.isEmpty() && r.identifier != "*")
            wrSection += QString(" identifier=\"%1\"").arg(r.identifier);
        if (!r.title.isEmpty())
            wrSection += QString(" title=\"%1\"").arg(r.title);
        if (!r.matchOnce.isEmpty())
            wrSection += QString(" matchOnce=\"%1\"").arg(r.matchOnce);
        wrSection += ">\n";
        auto addTag = [&](const QString &tag, const QString &val) {
            if (!val.isEmpty()) wrSection += QString("      <%1>%2</%1>\n").arg(tag, val);
        };
        addTag("skipTaskbar", r.skipTaskbar);
        addTag("skipWindowSwitcher", r.skipWindowSwitcher);
        addTag("decorate", r.decorate);
        addTag("type", r.type);
        addTag("monitor", r.monitor);
        addTag("workspace", r.workspace);
        wrSection += "    </windowRule>\n";
    }
    wrSection += "  </windowRules>";

    QRegularExpression wrRe(QStringLiteral(R"re(<windowRules>.*?</windowRules>)re"), QRegularExpression::DotMatchesEverythingOption);
    content.replace(wrRe, wrSection);

    f.resize(0);
    QTextStream out(&f);
    out << content;
    f.close();

    QProcess::startDetached("labwc", {"-r"});
    emit statusMessage("Window rules saved, labwc reconfigured");
}
