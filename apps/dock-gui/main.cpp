#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QIcon>
#include <QPixmap>
#include <QRegularExpression>
#include <QSplitter>
#include <QGroupBox>
#include <QScrollArea>
#include <QTimer>
#include <QStatusBar>

// ─── Theme constants ────────────────────────────────────────────────────────

static const QString BG       = "#1e1e2e";
static const QString BG_DARK  = "#181825";
static const QString SURFACE  = "#313244";
static const QString OVERLAY  = "#45475a";
static const QString TEXT     = "#cdd6f4";
static const QString SUBTEXT  = "#a6adc8";
static const QString BLUE     = "#89b4fa";
static const QString GREEN    = "#a6e3a1";
static const QString RED      = "#f38ba8";
static const QString YELLOW   = "#f9e2af";
static const QString MAUVE    = "#cba6f7";

static const QString STYLE_BTN = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(BLUE, BG, MAUVE, SURFACE);

static const QString STYLE_BTN_DANGER = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(RED, BG, "#f5a8c0", "#d16b8a");

static const QString STYLE_BTN_SUCCESS = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(GREEN, BG, "#b4f9b8", "#89b885");

static const QString STYLE_LIST = QString(
    "QListWidget { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 4px; }"
    "QListWidget::item { padding: 8px; border-radius: 4px; margin: 2px; }"
    "QListWidget::item:selected { background-color: %4; color: %5; }"
    "QListWidget::item:hover { background-color: %3; }"
).arg(BG_DARK, TEXT, SURFACE, BLUE, BG);

// ─── Desktop file info ──────────────────────────────────────────────────────

struct DesktopApp {
    QString id;          // filename without .desktop
    QString name;
    QString exec;
    QString icon;
    QString desktopFile; // full path
};

// ─── Dock config parser ─────────────────────────────────────────────────────

struct DockConfig {
    QString sessionName;     // e.g. "LXQt"
    QString panelFile;       // full path to panel_1.conf
    QVector<QString> launchers;  // list of launcher IDs
    QHash<QString, QString> settings;  // other key=value settings
};

class DockParser {
public:
    static DockConfig detectSession() {
        DockConfig config;
        QString baseDir = QDir::homePath() + "/.config/crystal-dock";

        // Try common session names
        QStringList sessions = {"LXQt", "Budgie", "Hyprland", "Sway"};
        for (const QString &session : sessions) {
            QString panelFile = baseDir + "/" + session + "/panel_1.conf";
            if (QFile::exists(panelFile)) {
                config.sessionName = session;
                config.panelFile = panelFile;
                break;
            }
        }

        if (config.panelFile.isEmpty()) {
            // Try to find any panel_1.conf
            QDir baseDirDir(baseDir);
            for (const QString &subdir : baseDirDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                QString panelFile = baseDir + "/" + subdir + "/panel_1.conf";
                if (QFile::exists(panelFile)) {
                    config.sessionName = subdir;
                    config.panelFile = panelFile;
                    break;
                }
            }
        }

        if (!config.panelFile.isEmpty()) {
            parsePanelFile(config);
        }

        return config;
    }

    static void parsePanelFile(DockConfig &config) {
        QFile file(config.panelFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#') || line.startsWith('['))
                continue;

            int eq = line.indexOf('=');
            if (eq > 0) {
                QString key = line.left(eq).trimmed();
                QString val = line.mid(eq + 1).trimmed();

                if (key == "launchers") {
                    // Remove surrounding quotes
                    if (val.startsWith('"') && val.endsWith('"')) {
                        val = val.mid(1, val.length() - 2);
                    }
                    config.launchers = val.split(';', Qt::SkipEmptyParts);
                } else {
                    config.settings[key] = val;
                }
            }
        }
        file.close();
    }

    static bool savePanelFile(const DockConfig &config) {
        QFile file(config.panelFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&file);
        out << "[General]\n";

        // Write launchers
        out << "launchers=\"" << config.launchers.join(';') << "\"\n";

        // Write other settings
        for (auto it = config.settings.constBegin(); it != config.settings.constEnd(); ++it) {
            out << it.key() << "=" << it.value() << "\n";
        }

        file.close();
        return true;
    }
};

// ─── Icon resolver ──────────────────────────────────────────────────────────

static QIcon resolveIcon(const QString &iconName) {
    if (iconName.isEmpty()) return {};

    // 1. Try QIcon::fromTheme first
    QIcon icon = QIcon::fromTheme(iconName);
    if (!icon.isNull()) return icon;

    // 2. Try with common extensions stripped/added
    if (!iconName.endsWith(".png") && !iconName.endsWith(".svg")) {
        icon = QIcon::fromTheme(iconName + ".png");
        if (!icon.isNull()) return icon;
    }

    // 3. Direct file lookup in hicolor/icon theme directories
    QStringList iconDirs = {
        QDir::homePath() + "/.local/share/icons",
        "/usr/share/icons",
        "/usr/local/share/icons"
    };
    QStringList sizes = {"512x512", "256x256", "128x128", "96x96", "64x64", "48x48", "32x32", "24x24", "22x22", "16x16"};

    for (const QString &baseDir : iconDirs) {
        for (const QString &size : sizes) {
            for (const QString &sub : {"apps", "categories", "devices", "mimetypes", "places", "status"}) {
                QString path = baseDir + "/hicolor/" + size + "/" + sub + "/" + iconName;
                if (!path.endsWith(".png") && !path.endsWith(".svg")) {
                    if (QFile::exists(path + ".png")) { path += ".png"; }
                    else if (QFile::exists(path + ".svg")) { path += ".svg"; }
                    else continue;
                }
                if (QFile::exists(path)) {
                    QIcon loaded(path);
                    if (!loaded.isNull()) return loaded;
                }
            }
        }
    }

    // 4. Try absolute path
    if (QFile::exists(iconName)) {
        QIcon loaded(iconName);
        if (!loaded.isNull()) return loaded;
    }

    return {};
}

// ─── Desktop file scanner ───────────────────────────────────────────────────

class DesktopFileScanner {
public:
    static QVector<DesktopApp> scanDesktopFiles() {
        QVector<DesktopApp> apps;
        QStringList searchDirs = {
            "/usr/share/applications",
            QDir::homePath() + "/.local/share/applications",
            "/usr/local/share/applications",
            QDir::homePath() + "/.local/share/flatpak/exports/share/applications",
            "/var/lib/flatpak/exports/share/applications"
        };

        for (const QString &dir : searchDirs) {
            QDir d(dir);
            if (!d.exists()) continue;

            for (const QFileInfo &fi : d.entryInfoList({"*.desktop"}, QDir::Files)) {
                DesktopApp app = parseDesktopFile(fi.absoluteFilePath());
                if (!app.name.isEmpty() && !app.exec.isEmpty()) {
                    apps.append(app);
                }
            }
        }

        // Sort by name
        std::sort(apps.begin(), apps.end(), [](const DesktopApp &a, const DesktopApp &b) {
            return a.name.toLower() < b.name.toLower();
        });

        return apps;
    }

    static QVector<DesktopApp> getAvailableApps(const QVector<QString> &currentLaunchers) {
        QVector<DesktopApp> all = scanDesktopFiles();
        QVector<DesktopApp> available;

        for (const DesktopApp &app : all) {
            bool exists = false;
            for (const QString &id : currentLaunchers) {
                if (id == app.id || id == app.id + ".desktop") {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                available.append(app);
            }
        }

        return available;
    }

    static DesktopApp parseDesktopFile(const QString &filePath) {
        DesktopApp app;
        app.desktopFile = filePath;
        app.id = QFileInfo(filePath).baseName();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return app;

        QTextStream in(&file);
        bool inDesktopEntry = false;
        bool gotName = false;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line == "[Desktop Entry]") {
                inDesktopEntry = true;
                continue;
            }

            if (line.startsWith('[') && inDesktopEntry)
                break;

            if (!inDesktopEntry) continue;

            // Skip localized versions
            if (line.contains('[')) continue;

            int eq = line.indexOf('=');
            if (eq <= 0) continue;

            QString key = line.left(eq).trimmed();
            QString val = line.mid(eq + 1).trimmed();

            if (key == "Name" && !gotName) {
                app.name = val;
                gotName = true;
            } else if (key == "Exec") {
                app.exec = val;
            } else if (key == "Icon") {
                app.icon = val;
            } else if (key == "NoDisplay" && val.toLower() == "true") {
                app.name.clear();  // Skip hidden apps
            } else if (key == "Terminal" && val.toLower() == "true") {
                app.name.clear();  // Skip terminal apps
            }
        }

        file.close();
        return app;
    }
};

// ─── Main Window ────────────────────────────────────────────────────────────

class DockManagerWindow : public QMainWindow {
    Q_OBJECT
public:
    DockManagerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("NaraVisuals — Crystal Dock Manager");
        setMinimumSize(900, 600);
        resize(1100, 750);

        setupUi();
        loadConfig();
    }

private slots:
    void onAddLauncher() {
        QVector<DesktopApp> available = DesktopFileScanner::getAvailableApps(config.launchers);
        if (available.isEmpty()) {
            QMessageBox::information(this, "Add Launcher", "No available applications to add.");
            return;
        }

        QStringList items;
        for (const DesktopApp &app : available) {
            items.append(app.name + "  (" + app.id + ")");
        }

        bool ok;
        QString choice = QInputDialog::getItem(this, "Add Launcher", "Choose an application:", items, 0, false, &ok);
        if (!ok || choice.isEmpty()) return;

        // Extract ID from "Name  (id)"
        int parenStart = choice.lastIndexOf("  (");
        if (parenStart >= 0) {
            QString id = choice.mid(parenStart + 3);
            id.chop(1);  // Remove trailing ")"
            config.launchers.insert(currentInsertPos(), id);
        }

        refreshList();
        saveConfig();
    }

    void onRemoveLauncher() {
        int row = launcherList->currentRow();
        if (row < 0) return;

        QString id = config.launchers[row];
        QString name = getLauncherName(id);

        int ret = QMessageBox::question(this, "Remove Launcher",
            "Remove \"" + name + "\" from the dock?");
        if (ret != QMessageBox::Yes) return;

        config.launchers.removeAt(row);
        refreshList();
        saveConfig();
    }

    void onMoveUp() {
        int row = launcherList->currentRow();
        if (row <= 0) return;
        config.launchers.swapItemsAt(row, row - 1);
        refreshList();
        launcherList->setCurrentRow(row - 1);
        saveConfig();
    }

    void onMoveDown() {
        int row = launcherList->currentRow();
        if (row < 0 || row >= config.launchers.size() - 1) return;
        config.launchers.swapItemsAt(row, row + 1);
        refreshList();
        launcherList->setCurrentRow(row + 1);
        saveConfig();
    }

    void onAddSeparator() {
        int pos = currentInsertPos();
        config.launchers.insert(pos, "separator");
        refreshList();
        launcherList->setCurrentRow(pos);
        saveConfig();
    }

    void onAddShowDesktop() {
        int pos = currentInsertPos();
        config.launchers.insert(pos, "show-desktop");
        refreshList();
        launcherList->setCurrentRow(pos);
        saveConfig();
    }

    void onReloadDock() {
        saveConfig();
        // Kill and restart crystal-dock
        QProcess::execute("pkill", {"crystal-dock"});
        // Wait briefly then restart
        QTimer::singleShot(500, this, []() {
            QProcess::startDetached("crystal-dock");
        });
        statusBar()->showMessage("Crystal Dock restarted", 3000);
    }

    void onRestartDock() {
        QProcess::execute("pkill", {"crystal-dock"});
        QTimer::singleShot(500, this, []() {
            QProcess::startDetached("crystal-dock");
        });
        statusBar()->showMessage("Crystal Dock restarted", 3000);
    }

    void onAddCustomLauncher() {
        QString execPath = QFileDialog::getOpenFileName(this, "Select executable or script",
            QDir::homePath());
        if (execPath.isEmpty()) return;

        bool ok;
        QString name = QInputDialog::getText(this, "Custom Launcher", "Name:",
            QLineEdit::Normal, QFileInfo(execPath).baseName(), &ok);
        if (!ok || name.isEmpty()) return;

        // Create a custom desktop file
        QString safe = name;
        safe.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
        QString desktopDir = QDir::homePath() + "/.local/share/applications";
        QDir().mkpath(desktopDir);
        QString desktopFile = desktopDir + "/nv-custom-" + safe.toLower() + ".desktop";

        QFile f(desktopFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=" << name << "\n";
            out << "Exec=" << execPath << "\n";
            out << "Terminal=false\n";
            out << "Categories=Utility;\n";
            f.close();
        }

        // Add to launchers
        QString id = "nv-custom-" + safe.toLower();
        int pos = currentInsertPos();
        config.launchers.insert(pos, id);
        refreshList();
        launcherList->setCurrentRow(pos);
        saveConfig();
    }

private:
    void setupUi() {
        auto *central = new QWidget(this);
        setCentralWidget(central);
        setStyleSheet(QString("QMainWindow, QWidget { background-color: %1; color: %2; }").arg(BG, TEXT));

        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(8, 6, 8, 8);
        mainLayout->setSpacing(6);

        // Header — compact single row
        auto *headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(8);
        titleLabel = new QLabel("Crystal Dock — Pinned Launchers");
        titleLabel->setStyleSheet(QString("font-size: 13px; font-weight: bold; color: %1;").arg(MAUVE));
        headerLayout->addWidget(titleLabel);

        sessionLabel = new QLabel("");
        sessionLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(SUBTEXT));
        headerLayout->addStretch();
        headerLayout->addWidget(sessionLabel);
        mainLayout->addLayout(headerLayout);

        // Splitter: Available apps | Current launchers
        auto *splitter = new QSplitter(Qt::Horizontal);
        splitter->setHandleWidth(2);

        // Left: Available apps
        auto *leftWidget = new QWidget();
        auto *leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 0, 0);
        leftLayout->setSpacing(8);

        auto *availLabel = new QLabel("Available Applications");
        availLabel->setStyleSheet(QString("font-size: 12px; font-weight: bold; color: %1;").arg(BLUE));
        leftLayout->addWidget(availLabel);

        // Search
        searchInput = new QLineEdit();
        searchInput->setPlaceholderText("Search applications...");
        searchInput->setStyleSheet(QString(
            "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px; }"
            "QLineEdit:focus { border: 1px solid %4; }"
        ).arg(BG, TEXT, SURFACE, BLUE));
        connect(searchInput, &QLineEdit::textChanged, this, &DockManagerWindow::filterAvailable);
        leftLayout->addWidget(searchInput);

        availableList = new QListWidget();
        availableList->setStyleSheet(STYLE_LIST);
        leftLayout->addWidget(availableList);

        auto *addBtn = new QPushButton("Add Selected →");
        addBtn->setStyleSheet(STYLE_BTN_SUCCESS);
        connect(addBtn, &QPushButton::clicked, this, &DockManagerWindow::onAddFromAvailable);
        leftLayout->addWidget(addBtn);

        splitter->addWidget(leftWidget);

        // Right: Current launchers
        auto *rightWidget = new QWidget();
        auto *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(0, 0, 0, 0);
        rightLayout->setSpacing(8);

        auto *currentLabel = new QLabel("Current Dock Launchers");
        currentLabel->setStyleSheet(QString("font-size: 12px; font-weight: bold; color: %1;").arg(GREEN));
        rightLayout->addWidget(currentLabel);

        launcherList = new QListWidget();
        launcherList->setStyleSheet(STYLE_LIST);
        rightLayout->addWidget(launcherList);

        // Buttons row
        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(6);

        auto *btnUp = new QPushButton("▲");
        btnUp->setToolTip("Move Up");
        btnUp->setStyleSheet(STYLE_BTN);
        btnUp->setFixedWidth(40);
        connect(btnUp, &QPushButton::clicked, this, &DockManagerWindow::onMoveUp);

        auto *btnDown = new QPushButton("▼");
        btnDown->setToolTip("Move Down");
        btnDown->setStyleSheet(STYLE_BTN);
        btnDown->setFixedWidth(40);
        connect(btnDown, &QPushButton::clicked, this, &DockManagerWindow::onMoveDown);

        auto *btnRemove = new QPushButton("— Remove");
        btnRemove->setStyleSheet(STYLE_BTN_DANGER);
        connect(btnRemove, &QPushButton::clicked, this, &DockManagerWindow::onRemoveLauncher);

        auto *btnSep = new QPushButton("+ Separator");
        btnSep->setStyleSheet(STYLE_BTN);
        connect(btnSep, &QPushButton::clicked, this, &DockManagerWindow::onAddSeparator);

        auto *btnShowDesktop = new QPushButton("+ Show Desktop");
        btnShowDesktop->setStyleSheet(STYLE_BTN);
        connect(btnShowDesktop, &QPushButton::clicked, this, &DockManagerWindow::onAddShowDesktop);

        auto *btnCustom = new QPushButton("+ Custom...");
        btnCustom->setStyleSheet(STYLE_BTN);
        connect(btnCustom, &QPushButton::clicked, this, &DockManagerWindow::onAddCustomLauncher);

        btnRow->addWidget(btnUp);
        btnRow->addWidget(btnDown);
        btnRow->addStretch();
        btnRow->addWidget(btnSep);
        btnRow->addWidget(btnShowDesktop);
        btnRow->addWidget(btnCustom);
        btnRow->addWidget(btnRemove);
        rightLayout->addLayout(btnRow);

        splitter->addWidget(rightWidget);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);

        mainLayout->addWidget(splitter);

        // Bottom bar
        auto *bottomBar = new QWidget();
        bottomBar->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2; border-radius: 6px;").arg(BG_DARK, SURFACE));
        auto *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(8, 5, 8, 5);

        auto *btnReload = new QPushButton("Save & Restart Dock");
        btnReload->setStyleSheet(STYLE_BTN_SUCCESS);
        connect(btnReload, &QPushButton::clicked, this, &DockManagerWindow::onReloadDock);

        auto *btnRestart = new QPushButton("Restart Dock");
        btnRestart->setStyleSheet(STYLE_BTN);
        connect(btnRestart, &QPushButton::clicked, this, &DockManagerWindow::onRestartDock);

        pathLabel = new QLabel("");
        pathLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(SUBTEXT));

        bottomLayout->addWidget(btnReload);
        bottomLayout->addWidget(btnRestart);
        bottomLayout->addStretch();
        bottomLayout->addWidget(pathLabel);

        mainLayout->addWidget(bottomBar);
    }

    void loadConfig() {
        config = DockParser::detectSession();

        if (config.panelFile.isEmpty()) {
            sessionLabel->setText("No Crystal Dock config found");
            QMessageBox::warning(this, "Error",
                "Could not find Crystal Dock configuration.\n"
                "Expected at: ~/.config/crystal-dock/<session>/panel_1.conf");
            return;
        }

        sessionLabel->setText("Session: " + config.sessionName);
        pathLabel->setText(config.panelFile);

        allDesktopApps = DesktopFileScanner::scanDesktopFiles();
        refreshAvailable();
        refreshList();
    }

    void saveConfig() {
        if (config.panelFile.isEmpty()) return;
        DockParser::savePanelFile(config);
    }

    void refreshAvailable() {
        availableList->clear();
        allAvailable.clear();
        for (const DesktopApp &app : allDesktopApps) {
            bool exists = false;
            for (const QString &id : config.launchers) {
                if (id == app.id || id == app.id + ".desktop" ||
                    app.id == id || app.id + ".desktop" == id) {
                    exists = true;
                    break;
                }
            }
            if (!exists) allAvailable.append(app);
        }
        populateAvailableList(allAvailable);
    }

    void populateAvailableList(const QVector<DesktopApp> &apps) {
        availableList->clear();
        for (const DesktopApp &app : apps) {
            auto *item = new QListWidgetItem();
            item->setText(app.name);
            item->setToolTip(app.id);
            item->setData(Qt::UserRole, app.id);

            if (!app.icon.isEmpty()) {
                QIcon icon = resolveIcon(app.icon);
                if (!icon.isNull()) {
                    item->setIcon(icon);
                }
            }

            availableList->addItem(item);
        }
    }

    void refreshList() {
        launcherList->clear();
        for (const QString &id : config.launchers) {
            auto *item = new QListWidgetItem();
            QString name = getLauncherName(id);
            if (id == "separator") {
                item->setText("─── Separator ───");
                item->setForeground(QColor(YELLOW));
            } else if (id == "show-desktop") {
                item->setText("Show Desktop");
                item->setForeground(QColor(MAUVE));
            } else {
                item->setText(name);
                item->setToolTip(id);
                item->setForeground(QColor(TEXT));
            }
            item->setData(Qt::UserRole, id);

            if (id != "separator" && id != "show-desktop") {
                DesktopApp app = findDesktopApp(id);
                if (!app.icon.isEmpty()) {
                    QIcon icon = resolveIcon(app.icon);
                    if (!icon.isNull()) {
                        item->setIcon(icon);
                    }
                }
            }

            launcherList->addItem(item);
        }
    }

    void filterAvailable(const QString &text) {
        if (text.isEmpty()) {
            populateAvailableList(allAvailable);
            return;
        }

        QVector<DesktopApp> filtered;
        for (const DesktopApp &app : allAvailable) {
            if (app.name.contains(text, Qt::CaseInsensitive) ||
                app.id.contains(text, Qt::CaseInsensitive)) {
                filtered.append(app);
            }
        }
        populateAvailableList(filtered);
    }

    void onAddFromAvailable() {
        QListWidgetItem *item = availableList->currentItem();
        if (!item) return;

        QString id = item->data(Qt::UserRole).toString();
        int pos = currentInsertPos();
        config.launchers.insert(pos, id);
        refreshList();
        refreshAvailable();
        launcherList->setCurrentRow(pos);
        saveConfig();
    }

    QString getLauncherName(const QString &id) {
        if (id == "separator") return "--- Separator ---";
        if (id == "show-desktop") return "Show Desktop";

        DesktopApp app = findDesktopApp(id);
        return app.name.isEmpty() ? id : app.name;
    }

    DesktopApp findDesktopApp(const QString &id) {
        for (const DesktopApp &app : allDesktopApps) {
            if (app.id == id) return app;
        }
        for (const DesktopApp &app : allDesktopApps) {
            if (app.id == id + ".desktop" || id == app.id + ".desktop") return app;
        }

        QStringList searchDirs = {
            "/usr/share/applications",
            QDir::homePath() + "/.local/share/applications",
            "/usr/local/share/applications",
            QDir::homePath() + "/.local/share/flatpak/exports/share/applications",
            "/var/lib/flatpak/exports/share/applications"
        };
        for (const QString &dir : searchDirs) {
            QString path = dir + "/" + id + ".desktop";
            if (QFile::exists(path)) {
                return DesktopFileScanner::parseDesktopFile(path);
            }
        }

        return {};
    }

    int currentInsertPos() {
        int row = launcherList->currentRow();
        if (row < 0) return config.launchers.size();
        return row + 1;
    }

    // UI elements
    QLabel *titleLabel = nullptr;
    QLabel *sessionLabel = nullptr;
    QLabel *pathLabel = nullptr;
    QLineEdit *searchInput = nullptr;
    QListWidget *availableList = nullptr;
    QListWidget *launcherList = nullptr;

    // Data
    DockConfig config;
    QVector<DesktopApp> allDesktopApps;   // full cache for lookups
    QVector<DesktopApp> allAvailable;     // filtered (not yet pinned)
};

// ─── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nv-dock-gui");
    app.setApplicationVersion("1.0.0");

    DockManagerWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
