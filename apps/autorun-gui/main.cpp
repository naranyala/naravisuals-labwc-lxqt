#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QSettings>
#include <QProcess>
#include <QRegularExpression>

static const QString STYLE_TABLE =
    "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
    "QTableWidget::item { padding: 6px; }"
    "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
    "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }";

static const QString STYLE_BTN =
    "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
    "QPushButton:hover { background-color: #b4befe; }"
    "QPushButton:pressed { background-color: #74c7ec; }";

static const QString STYLE_DANGER =
    "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
    "QPushButton:hover { background-color: #f5a8c0; }"
    "QPushButton:pressed { background-color: #d16b8a; }";

static const QString STYLE_SUCCESS =
    "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
    "QPushButton:hover { background-color: #b4f9b8; }"
    "QPushButton:pressed { background-color: #89b885; }";

// ─── XDG Autostart ──────────────────────────────────────────────────────────

struct XdgItem {
    QString fileName;
    QString name;
    QString exec;
    bool isHidden;
};

class XdgTab : public QWidget {
    Q_OBJECT
public:
    explicit XdgTab(QWidget *parent = nullptr) : QWidget(parent) {
        autostartDir = QDir::homePath() + "/.config/autostart";
        QDir().mkpath(autostartDir);

        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Applications that start automatically on login (XDG .desktop files)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Name", "Command", "Status"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->setStyleSheet(STYLE_TABLE);
        layout->addWidget(table);

        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &XdgTab::loadItems);
        btnRow->addWidget(refreshBtn);

        auto *toggleBtn = new QPushButton("Toggle Enable/Disable");
        toggleBtn->setMinimumHeight(36);
        toggleBtn->setStyleSheet(STYLE_SUCCESS);
        connect(toggleBtn, &QPushButton::clicked, this, &XdgTab::toggleItem);
        btnRow->addWidget(toggleBtn);

        btnRow->addStretch();

        auto *editBtn = new QPushButton("Edit");
        editBtn->setMinimumHeight(36);
        connect(editBtn, &QPushButton::clicked, this, &XdgTab::editItem);
        btnRow->addWidget(editBtn);

        auto *removeBtn = new QPushButton("Remove");
        removeBtn->setMinimumHeight(36);
        removeBtn->setStyleSheet(STYLE_DANGER);
        connect(removeBtn, &QPushButton::clicked, this, &XdgTab::removeItem);
        btnRow->addWidget(removeBtn);

        auto *addBtn = new QPushButton("Add New");
        addBtn->setMinimumHeight(36);
        addBtn->setStyleSheet(STYLE_BTN);
        connect(addBtn, &QPushButton::clicked, this, &XdgTab::addItemDialog);
        btnRow->addWidget(addBtn);

        layout->addLayout(btnRow);
        loadItems();
    }

    void refresh() { loadItems(); }

signals:
    void statusMessage(const QString &msg);

private slots:
    void loadItems() {
        items.clear();
        QDir dir(autostartDir);
        dir.setNameFilters({"*.desktop"});
        for (const QFileInfo &fi : dir.entryInfoList()) {
            QSettings s(fi.absoluteFilePath(), QSettings::IniFormat);
            s.beginGroup("Desktop Entry");
            items.append({fi.fileName(), s.value("Name", fi.baseName()).toString(),
                          s.value("Exec", "").toString(), s.value("Hidden", false).toBool()});
            s.endGroup();
        }
        populateTable();
        emit statusMessage(QString("Loaded %1 XDG autostart entries").arg(items.size()));
    }

    void populateTable() {
        table->setRowCount(0);
        for (const auto &item : items) {
            int r = table->rowCount();
            table->insertRow(r);
            auto *n = new QTableWidgetItem(item.name);
            n->setForeground(QColor("#f9e2af"));
            table->setItem(r, 0, n);
            auto *e = new QTableWidgetItem(item.exec);
            e->setForeground(QColor("#a6adc8"));
            table->setItem(r, 1, e);
            auto *s = new QTableWidgetItem(item.isHidden ? "Disabled" : "Enabled");
            s->setForeground(item.isHidden ? QColor("#f38ba8") : QColor("#a6e3a1"));
            table->setItem(r, 2, s);
        }
    }

    void toggleItem() {
        int row = table->currentRow();
        if (row < 0 || row >= items.size()) { emit statusMessage("Select an item first"); return; }
        auto &item = items[row];
        QString path = autostartDir + "/" + item.fileName;
        QSettings s(path, QSettings::IniFormat);
        s.beginGroup("Desktop Entry");
        s.setValue("Hidden", !item.isHidden);
        s.endGroup();
        s.sync();
        loadItems();
        emit statusMessage(QString("Toggled '%1'").arg(item.name));
    }

    void editItem() {
        int row = table->currentRow();
        if (row < 0 || row >= items.size()) { emit statusMessage("Select an item first"); return; }
        auto &item = items[row];

        bool ok;
        QString newName = QInputDialog::getText(this, "Edit Name", "Name:", QLineEdit::Normal, item.name, &ok);
        if (!ok) return;
        QString newExec = QInputDialog::getText(this, "Edit Command", "Command:", QLineEdit::Normal, item.exec, &ok);
        if (!ok) return;

        QString path = autostartDir + "/" + item.fileName;
        QSettings s(path, QSettings::IniFormat);
        s.beginGroup("Desktop Entry");
        s.setValue("Name", newName);
        s.setValue("Exec", newExec);
        s.endGroup();
        s.sync();
        loadItems();
        emit statusMessage(QString("Updated '%1'").arg(newName));
    }

    void removeItem() {
        int row = table->currentRow();
        if (row < 0 || row >= items.size()) { emit statusMessage("Select an item first"); return; }
        auto reply = QMessageBox::question(this, "Confirm", "Remove '" + items[row].name + "' from autostart?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QFile::remove(autostartDir + "/" + items[row].fileName);
            loadItems();
        }
    }

    void addItemDialog() {
        QString execPath = QFileDialog::getOpenFileName(this, "Select executable or script", QDir::homePath());
        if (execPath.isEmpty()) return;

        bool ok;
        QString name = QInputDialog::getText(this, "Name", "Friendly name:", QLineEdit::Normal,
                                             QFileInfo(execPath).baseName(), &ok);
        if (!ok || name.isEmpty()) return;

        QString safe = name;
        safe.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
        QString path = autostartDir + "/" + safe + ".desktop";

        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "[Desktop Entry]\nType=Application\nName=" << name << "\nExec=" << execPath
                << "\nHidden=false\nTerminal=false\n";
            f.close();
            QProcess::execute("chmod", {"+x", execPath});
            loadItems();
            emit statusMessage("Added to XDG autostart");
        }
    }

private:
    QTableWidget *table;
    QList<XdgItem> items;
    QString autostartDir;
};

// ─── Labwc Autostart Script ─────────────────────────────────────────────────

struct LabwcEntry {
    QString daemon;
    QString execLine;
    int execIdx;       // line index in the file
    int blockStart;    // line index of `if command -v`
    int blockEnd;      // line index of matching `fi`
    bool enabled;
};

class LabwcTab : public QWidget {
    Q_OBJECT
public:
    explicit LabwcTab(QWidget *parent = nullptr) : QWidget(parent) {
        scriptPath = QDir::homePath() + "/.config/labwc/autostart";

        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Daemons launched by the labwc Wayland compositor autostart script");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Daemon", "Command", "Status"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->setStyleSheet(STYLE_TABLE);
        layout->addWidget(table);

        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &LabwcTab::loadEntries);
        btnRow->addWidget(refreshBtn);

        auto *toggleBtn = new QPushButton("Toggle Enable/Disable");
        toggleBtn->setMinimumHeight(36);
        toggleBtn->setStyleSheet(STYLE_SUCCESS);
        connect(toggleBtn, &QPushButton::clicked, this, &LabwcTab::toggleEntry);
        btnRow->addWidget(toggleBtn);

        btnRow->addStretch();

        auto *addBtn = new QPushButton("Add Daemon Entry");
        addBtn->setMinimumHeight(36);
        addBtn->setStyleSheet(STYLE_BTN);
        connect(addBtn, &QPushButton::clicked, this, &LabwcTab::addEntry);
        btnRow->addWidget(addBtn);

        layout->addLayout(btnRow);

        // Note about the file
        auto *note = new QLabel("Note: Entries are parsed from 'if command -v …' blocks in the script. "
                                "Toggle comments/uncomments the command line. Edit the script directly for complex entries.");
        note->setStyleSheet("font-size: 12px; color: #6c7086;");
        note->setWordWrap(true);
        layout->addWidget(note);

        loadEntries();
    }

    void refresh() { loadEntries(); }

signals:
    void statusMessage(const QString &msg);

private:
    void loadEntries() {
        entries.clear();
        lines.clear();

        QFile f(scriptPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            populateTable();
            emit statusMessage("labwc autostart not found at " + scriptPath);
            return;
        }

        QString raw = f.readAll();
        f.close();
        lines = raw.split('\n');

        static QRegularExpression cmdVRe(R"(^if\s+command\s+-v\s+(\S+))");
        for (int i = 0; i < lines.size(); i++) {
            auto m = cmdVRe.match(lines[i].trimmed());
            if (!m.hasMatch()) continue;

            int depth = 1;
            int execIdx = -1;
            int fiIdx = -1;
            for (int j = i + 1; j < lines.size() && depth > 0; j++) {
                QString t = lines[j].trimmed();
                if (t.startsWith("if ")) depth++;
                else if (t == "fi") { depth--; if (depth == 0) fiIdx = j; }
                else if (execIdx < 0 && depth == 1 && !t.isEmpty() && t != "then" && !t.startsWith("#")) {
                    execIdx = j;
                }
            }

            if (execIdx >= 0) {
                QString exec = lines[execIdx].trimmed();
                bool enabled = !exec.startsWith("#");
                if (!enabled) {
                    // strip leading # and spaces for display
                    exec = exec.remove(QRegularExpression("^#\\s*"));
                }
                // strip trailing & for display
                if (exec.endsWith("&")) exec.chop(1);
                exec = exec.trimmed();
                entries.append({m.captured(1), exec, execIdx, i, fiIdx, enabled});
            }
        }

        populateTable();
        emit statusMessage(QString("Found %1 daemon entries in labwc autostart").arg(entries.size()));
    }

    void populateTable() {
        table->setRowCount(0);
        for (const auto &e : entries) {
            int r = table->rowCount();
            table->insertRow(r);
            auto *d = new QTableWidgetItem(e.daemon);
            d->setForeground(QColor("#89b4fa"));
            table->setItem(r, 0, d);
            auto *c = new QTableWidgetItem(e.execLine);
            c->setForeground(QColor("#a6adc8"));
            table->setItem(r, 1, c);
            auto *s = new QTableWidgetItem(e.enabled ? "Enabled" : "Disabled");
            s->setForeground(e.enabled ? QColor("#a6e3a1") : QColor("#f38ba8"));
            table->setItem(r, 2, s);
        }
    }

    void toggleEntry() {
        int row = table->currentRow();
        if (row < 0 || row >= entries.size()) { emit statusMessage("Select an entry first"); return; }

        QFile f(scriptPath);
        if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) {
            emit statusMessage("Cannot write " + scriptPath);
            return;
        }

        QString all = f.readAll();
        QStringList lns = all.split('\n');
        auto &entry = entries[row];

        if (entry.enabled) {
            // Disable: comment the exec line
            if (entry.execIdx >= 0 && entry.execIdx < lns.size()) {
                lns[entry.execIdx] = "# " + lns[entry.execIdx];
            }
        } else {
            // Enable: uncomment the exec line
            if (entry.execIdx >= 0 && entry.execIdx < lns.size()) {
                QString t = lns[entry.execIdx].trimmed();
                if (t.startsWith("#")) {
                    lns[entry.execIdx] = lns[entry.execIdx].remove(QRegularExpression("^\\s*#\\s*"));
                }
            }
        }

        f.resize(0);
        QTextStream out(&f);
        out << lns.join('\n');
        if (all.endsWith('\n')) out << '\n';
        f.close();

        loadEntries();
        reconfigureLabwc();
        emit statusMessage(QString("Toggled '%1' in labwc autostart").arg(entry.daemon));
    }

    void addEntry() {
        bool ok;
        QString daemon = QInputDialog::getText(this, "Daemon Name", "Program/binary name (e.g. conky):",
                                               QLineEdit::Normal, "", &ok);
        if (!ok || daemon.isEmpty()) return;

        QString args = QInputDialog::getText(this, "Arguments",
                                             "Command line arguments (leave empty if none):",
                                             QLineEdit::Normal, "", &ok);
        if (!ok) return;

        QFile f(scriptPath);
        if (!f.open(QIODevice::Append | QIODevice::Text)) {
            // File doesn't exist yet, create it
            QDir().mkpath(QDir::homePath() + "/.config/labwc");
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                emit statusMessage("Cannot create " + scriptPath);
                return;
            }
        }

        QTextStream out(&f);
        out << "\n# Added by nv-autorun-gui\n";
        out << "if command -v " << daemon << " &>/dev/null; then\n";
        out << "  " << daemon << " " << args << " &\n";
        out << "fi\n";
        f.close();

        loadEntries();
        reconfigureLabwc();
        emit statusMessage(QString("Added '%1' to labwc autostart").arg(daemon));
    }

    void reconfigureLabwc() {
        QProcess::startDetached("labwc", {"-r"});
    }

    QTableWidget *table;
    QList<LabwcEntry> entries;
    QStringList lines;
    QString scriptPath;
};

// ─── Systemd User Services ──────────────────────────────────────────────────

class SystemdTab : public QWidget {
    Q_OBJECT
public:
    explicit SystemdTab(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Systemd user services (start/stop/enable/disable)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Service", "Active", "Enabled"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->setStyleSheet(STYLE_TABLE);
        layout->addWidget(table);

        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &SystemdTab::loadServices);
        btnRow->addWidget(refreshBtn);

        auto *startBtn = new QPushButton("Start");
        startBtn->setMinimumHeight(36);
        startBtn->setStyleSheet(STYLE_SUCCESS);
        connect(startBtn, &QPushButton::clicked, this, &SystemdTab::startService);
        btnRow->addWidget(startBtn);

        auto *stopBtn = new QPushButton("Stop");
        stopBtn->setMinimumHeight(36);
        stopBtn->setStyleSheet(STYLE_DANGER);
        connect(stopBtn, &QPushButton::clicked, this, &SystemdTab::stopService);
        btnRow->addWidget(stopBtn);

        btnRow->addStretch();

        auto *enableBtn = new QPushButton("Enable");
        enableBtn->setMinimumHeight(36);
        connect(enableBtn, &QPushButton::clicked, this, &SystemdTab::enableService);
        btnRow->addWidget(enableBtn);

        auto *disableBtn = new QPushButton("Disable");
        disableBtn->setMinimumHeight(36);
        connect(disableBtn, &QPushButton::clicked, this, &SystemdTab::disableService);
        btnRow->addWidget(disableBtn);

        layout->addLayout(btnRow);
        loadServices();
    }

    void refresh() { loadServices(); }

signals:
    void statusMessage(const QString &msg);

private slots:
    void loadServices() {
        services.clear();

        QProcess p;
        p.start("systemctl", {"--user", "list-units", "--type=service", "--all", "--no-pager", "--no-legend"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 4) continue;
            services.append({parts[0], parts[2], parts[3]});
        }

        // Get enable/disable status
        QProcess ep;
        ep.start("systemctl", {"--user", "list-unit-files", "--type=service", "--no-pager", "--no-legend"});
        ep.waitForFinished(5000);
        QString eout = ep.readAllStandardOutput();
        QMap<QString, QString> enabledMap;
        for (const QString &line : eout.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) enabledMap[parts[0]] = parts[1];
        }

        table->setRowCount(0);
        for (const auto &svc : services) {
            int r = table->rowCount();
            table->insertRow(r);
            table->setItem(r, 0, new QTableWidgetItem(svc.name));
            auto *a = new QTableWidgetItem(svc.activeState);
            if (svc.activeState == "active") a->setForeground(QColor("#a6e3a1"));
            else if (svc.activeState == "failed") a->setForeground(QColor("#f38ba8"));
            table->setItem(r, 1, a);

            QString enabled = enabledMap.value(svc.name, "unknown");
            auto *e = new QTableWidgetItem(enabled);
            if (enabled == "enabled") e->setForeground(QColor("#a6e3a1"));
            else if (enabled == "disabled") e->setForeground(QColor("#f38ba8"));
            table->setItem(r, 2, e);
        }
        emit statusMessage(QString("%1 user services loaded").arg(services.size()));
    }

    QString selectedService() {
        int row = table->currentRow();
        if (row < 0) return {};
        return table->item(row, 0)->text();
    }

    void startService() {
        QString s = selectedService();
        if (s.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "start", s});
        loadServices();
    }

    void stopService() {
        QString s = selectedService();
        if (s.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "stop", s});
        loadServices();
    }

    void enableService() {
        QString s = selectedService();
        if (s.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "enable", s});
        loadServices();
    }

    void disableService() {
        QString s = selectedService();
        if (s.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "disable", s});
        loadServices();
    }

private:
    struct Svc { QString name, activeState, subState; };
    QTableWidget *table;
    QList<Svc> services;
};

// ─── Main Window ────────────────────────────────────────────────────────────

class AutorunManager : public QMainWindow {
    Q_OBJECT
public:
    AutorunManager(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Autorun Applications Manager");
        resize(800, 550);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; "
            "border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; "
            "border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        xdgTab = new XdgTab();
        labwcTab = new LabwcTab();
        systemdTab = new SystemdTab();

        tabs->addTab(xdgTab, "XDG Autostart");
        tabs->addTab(labwcTab, "Labwc Script");
        tabs->addTab(systemdTab, "Systemd Services");

        connect(xdgTab, &XdgTab::statusMessage, this, &AutorunManager::showStatus);
        connect(labwcTab, &LabwcTab::statusMessage, this, &AutorunManager::showStatus);
        connect(systemdTab, &SystemdTab::statusMessage, this, &AutorunManager::showStatus);

        mainLayout->addWidget(tabs);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLabel { font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

private slots:
    void showStatus(const QString &msg) {
        statusBar()->showMessage(msg, 5000);
    }

private:
    QTabWidget *tabs;
    XdgTab *xdgTab;
    LabwcTab *labwcTab;
    SystemdTab *systemdTab;
};

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // CLI mode
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--add" && i + 2 < argc) {
            QString name = argv[++i];
            QString execPath = argv[++i];
            QString dir = QDir::homePath() + "/.config/autostart";
            QDir().mkpath(dir);
            QString safe = name;
            safe.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
            QFile f(dir + "/" + safe + ".desktop");
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&f);
                out << "[Desktop Entry]\nType=Application\nName=" << name << "\nExec=" << execPath
                    << "\nHidden=false\nTerminal=false\n";
                f.close();
                QProcess::execute("chmod", {"+x", execPath});
            }
            return 0;
        } else if (arg == "--remove" && i + 1 < argc) {
            QString fn = argv[++i];
            QFile::remove(QDir::homePath() + "/.config/autostart/" + fn);
            return 0;
        }
    }

    AutorunManager gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
