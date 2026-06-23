#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QLineEdit>
#include <QStatusBar>
#include <QMainWindow>
#include <QRegularExpression>
#include <QFile>
#include <QTabWidget>
#include <QGroupBox>
#include <QScrollArea>
#include <QFrame>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QGridLayout>
#include <QTimer>

struct FontInfo {
    QString file;
    QString family;
    QString style;
};

struct HealthIssue {
    QString id;
    QString category;
    QString title;
    QString description;
    QString fixCommand;
    QString fixDescription;
    bool autoFixable;
    bool detected;
};

// ---- Health Check Engine ----
class HealthChecker : public QObject {
    Q_OBJECT

public:
    explicit HealthChecker(QObject *parent = nullptr) : QObject(parent) {}

    QList<HealthIssue> runCheck() {
        issues.clear();
        checkFontconfigExists();
        checkFontconfigTools();
        checkFontRenderingEnvVars();
        checkRequiredFonts();
        checkFontCacheFreshness();
        checkFontconfigConfig();
        checkMonospacePreference();
        checkDpiSettings();
        return issues;
    }

    bool fixIssue(const QString &issueId) {
        for (auto &issue : issues) {
            if (issue.id == issueId && issue.autoFixable) {
                return executeFix(issue);
            }
        }
        return false;
    }

    QList<HealthIssue> issues;

private:
    void addIssue(const QString &id, const QString &cat, const QString &title,
                  const QString &desc, const QString &fixCmd, const QString &fixDesc,
                  bool autoFix, bool detected) {
        issues.append({id, cat, title, desc, fixCmd, fixDesc, autoFix, detected});
    }

    void checkFontconfigExists() {
        QProcess proc;
        proc.start("fc-match", {"sans-serif"});
        proc.waitForFinished(5000);
        bool ok = (proc.exitCode() == 0 && !proc.readAllStandardOutput().trimmed().isEmpty());
        addIssue("fontconfig-tools", "System",
                 "Fontconfig tools",
                 ok ? "fc-match and fc-cache are available" : "fc-match or fc-cache not found",
                 "sudo apt install -y fontconfig", "Install fontconfig package",
                 true, !ok);
    }

    void checkFontconfigTools() {
        QProcess proc;
        proc.start("fc-cache", {"--version"});
        proc.waitForFinished(5000);
        bool ok = (proc.exitCode() == 0);
        addIssue("fc-cache-version", "System",
                 "fc-cache version",
                 ok ? QString("fc-cache %1").arg(QString(proc.readAllStandardOutput().trimmed()).split('\n').first())
                   : "fc-cache not available",
                 "", "", false, !ok);
    }

    void checkFontRenderingEnvVars() {
        QString envFile = QDir::homePath() + "/.config/labwc/environment";
        QStringList requiredVars = {"GDK_FONTCONFIG_HINT", "GDK_RENDERING", "PANGOCAIRO_BACKEND"};
        QStringList expectedValues = {"1", "image", "fontconfig"};
        QStringList missing;

        QFile file(envFile);
        QString content;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(file.readAll());
            file.close();
        }

        for (int i = 0; i < requiredVars.size(); i++) {
            QRegularExpression re(QString("^%1=(.+)$").arg(requiredVars[i]), QRegularExpression::MultilineOption);
            QRegularExpressionMatch match = re.match(content);
            if (!match.hasMatch() || match.captured(1).trimmed() != expectedValues[i]) {
                missing.append(QString("%1=%2").arg(requiredVars[i], expectedValues[i]));
            }
        }

        bool ok = missing.isEmpty();
        QString desc = ok ? "All font rendering env vars are set"
                          : QString("Missing: %1").arg(missing.join(", "));
        QString fix = ok ? "" : QString("bash -c 'echo -e \"\\n# Font rendering (Wayland)\\nGDK_FONTCONFIG_HINT=1\\nGDK_RENDERING=image\\nPANGOCAIRO_BACKEND=fontconfig\" >> \"%1\"'").arg(envFile);

        addIssue("font-env-vars", "Environment",
                 "Font rendering environment variables",
                 desc, fix, "Add font rendering env vars to labwc environment",
                 true, !ok);
    }

    void checkRequiredFonts() {
        QProcess proc;
        proc.start("fc-list");
        proc.waitForFinished(10000);
        QString output = proc.readAllStandardOutput();

        QStringList required = {"Noto Sans", "Noto Sans Mono", "Noto Serif"};
        QStringList missing;

        for (const QString &font : required) {
            if (!output.contains(font, Qt::CaseInsensitive)) {
                missing.append(font);
            }
        }

        bool ok = missing.isEmpty();
        QString desc = ok ? "All required fonts (Noto Sans, Noto Sans Mono, Noto Serif) installed"
                          : QString("Missing: %1").arg(missing.join(", "));
        QString fixCmd = missing.isEmpty() ? "" : "sudo apt install -y fonts-noto fonts-noto-extra";

        addIssue("required-fonts", "Fonts",
                 "Required UI fonts",
                 desc, fixCmd, "Install Noto font family",
                 true, !ok);
    }

    void checkFontCacheFreshness() {
        QString cacheDir = QDir::homePath() + "/.cache/fontconfig";
        QDir dir(cacheDir);
        if (!dir.exists()) {
            addIssue("font-cache", "Cache",
                     "Font cache",
                     "Font cache directory does not exist",
                     "fc-cache -fv", "Create and populate font cache",
                     true, true);
            return;
        }

        QFileInfoList entries = dir.entryInfoList(QDir::Files);
        if (entries.isEmpty()) {
            addIssue("font-cache", "Cache",
                     "Font cache",
                     "Font cache is empty",
                     "fc-cache -fv", "Rebuild font cache",
                     true, true);
            return;
        }

        QDateTime oldest = entries.first().lastModified();
        for (const QFileInfo &fi : entries) {
            if (fi.lastModified() < oldest) oldest = fi.lastModified();
        }

        qint64 daysOld = QDateTime::currentDateTime().secsTo(oldest) / -86400;
        bool ok = (daysOld < 7);
        QString desc = ok ? QString("Font cache is fresh (%1 days old)").arg(daysOld)
                          : QString("Font cache is %1 days old — may cause rendering issues").arg(daysOld);

        addIssue("font-cache-age", "Cache",
                 "Font cache freshness",
                 desc, "fc-cache -fv", "Rebuild font cache",
                 true, !ok);
    }

    void checkFontconfigConfig() {
        QString confPath = QDir::homePath() + "/.config/fontconfig/fonts.conf";
        bool exists = QFile::exists(confPath);

        if (!exists) {
            addIssue("fontconfig-missing", "Config",
                     "Fontconfig user config",
                     "~/.config/fontconfig/fonts.conf is MISSING — this causes empty text/buttons",
                     "", "Deploy fontconfig config (uses setup-fontconfig.sh)",
                     true, true);
            return;
        }

        QFile file(confPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            addIssue("fontconfig-readable", "Config",
                     "Fontconfig user config",
                     "Cannot read fonts.conf — check file permissions",
                     "", "", false, true);
            return;
        }

        QString content = QString(file.readAll());
        file.close();

        // Check antialiasing
        bool hasAntialias = content.contains("antialias") && content.contains("<bool>true</bool>");
        bool hasHinting = content.contains("hinting") || content.contains("hintstyle");
        bool hasDpi = content.contains("dpi");

        QStringList missing;
        if (!hasAntialias) missing.append("antialiasing");
        if (!hasHinting) missing.append("hinting");
        if (!hasDpi) missing.append("DPI settings");

        bool ok = missing.isEmpty();
        QString desc = ok ? "fonts.conf has antialiasing, hinting, and DPI configured"
                          : QString("fonts.conf missing: %1").arg(missing.join(", "));

        addIssue("fontconfig-content", "Config",
                 "Fontconfig config completeness",
                 desc, "", "", false, !ok);
    }

    void checkMonospacePreference() {
        QString confPath = QDir::homePath() + "/.config/fontconfig/fonts.conf";
        if (!QFile::exists(confPath)) return;

        QFile file(confPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QString content = QString(file.readAll());
        file.close();

        // Check if monospace preference matches app configs (Noto Sans Mono)
        QRegularExpression re("<family>monospace</family>.*?<prefer>.*?<family>(.*?)</family>",
                              QRegularExpression::DotMatchesEverythingOption | QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = re.match(content);

        if (match.hasMatch()) {
            QString preferred = match.captured(1).trimmed();
            bool ok = preferred.contains("Noto Sans Mono", Qt::CaseInsensitive);
            addIssue("monospace-preference", "Config",
                     "Monospace font preference",
                     ok ? QString("Preferred monospace: %1 (matches app configs)").arg(preferred)
                        : QString("Preferred monospace: %1 — should be Noto Sans Mono").arg(preferred),
                     "", "", false, !ok);
        }
    }

    void checkDpiSettings() {
        QString confPath = QDir::homePath() + "/.config/fontconfig/fonts.conf";
        if (!QFile::exists(confPath)) return;

        QFile file(confPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QString content = QString(file.readAll());
        file.close();

        QRegularExpression re("<double>(\\d+)</double>");
        QRegularExpressionMatchIterator it = re.globalMatch(content);
        bool has96 = false;
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            if (m.captured(1) == "96") { has96 = true; break; }
        }

        addIssue("dpi-settings", "Config",
                 "DPI settings",
                 has96 ? "DPI set to 96 (standard)" : "DPI not set or not 96",
                 "", "", false, !has96);
    }

    bool executeFix(const HealthIssue &issue) {
        if (issue.fixCommand.isEmpty()) return false;

        // Special handling for fontconfig deploy
        if (issue.id == "fontconfig-missing") {
            return deployFontconfig();
        }

        QProcess proc;
        proc.start("bash", {"-c", issue.fixCommand});
        proc.waitForFinished(30000);
        return proc.exitCode() == 0;
    }

    bool deployFontconfig() {
        // Try project config first, then create minimal
        QStringList searchPaths = {
            QCoreApplication::applicationDirPath() + "/../../configs/dotfiles/fontconfig/fonts.conf",
            QCoreApplication::applicationDirPath() + "/../../../configs/dotfiles/fontconfig/fonts.conf",
            QDir::homePath() + "/.local/share/naravisuals/configs/dotfiles/fontconfig/fonts.conf"
        };

        QString srcPath;
        for (const QString &p : searchPaths) {
            if (QFile::exists(p)) { srcPath = p; break; }
        }

        QString dstPath = QDir::homePath() + "/.config/fontconfig/fonts.conf";
        QDir().mkpath(QFileInfo(dstPath).absolutePath());

        if (!srcPath.isEmpty()) {
            return QFile::copy(srcPath, dstPath);
        }

        // Create minimal fontconfig
        QFile file(dstPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&file);
        out << "<?xml version=\"1.0\"?>\n"
            << "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n"
            << "<fontconfig>\n"
            << "  <match target=\"font\">\n"
            << "    <edit name=\"antialias\" mode=\"assign\"><bool>true</bool></edit>\n"
            << "  </match>\n"
            << "  <match target=\"font\">\n"
            << "    <edit name=\"rgba\" mode=\"assign\"><const>rgb</const></edit>\n"
            << "  </match>\n"
            << "  <match target=\"font\">\n"
            << "    <edit name=\"hinting\" mode=\"assign\"><bool>true</bool></edit>\n"
            << "  </match>\n"
            << "  <match target=\"font\">\n"
            << "    <edit name=\"hintstyle\" mode=\"assign\"><const>hintslight</const></edit>\n"
            << "  </match>\n"
            << "  <match target=\"pattern\">\n"
            << "    <edit name=\"dpi\" mode=\"assign\"><double>96</double></edit>\n"
            << "  </match>\n"
            << "  <alias><family>monospace</family><prefer><family>Noto Sans Mono</family></prefer></alias>\n"
            << "</fontconfig>\n";
        file.close();
        return true;
    }
};

// ---- Health Check UI Widget ----
class HealthCheckWidget : public QWidget {
    Q_OBJECT

public:
    explicit HealthCheckWidget(QWidget *parent = nullptr) : QWidget(parent) {
        checker = new HealthChecker(this);
        setupUI();
    }

    void runCheck() {
        // Clear previous results
        QLayoutItem *item;
        while ((item = resultsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        issues = checker->runCheck();

        int issuesFound = 0;
        for (const auto &issue : issues) {
            if (issue.detected) issuesFound++;
            addIssueCard(issue);
        }

        // Summary
        auto *summary = new QLabel();
        summary->setWordWrap(true);
        if (issuesFound == 0) {
            summary->setText("✅ All checks passed. Your font rendering configuration looks healthy.");
            summary->setStyleSheet("color: #a6e3a1; font-size: 14px; padding: 10px; background-color: #1e1e2e; border-radius: 8px;");
        } else {
            summary->setText(QString("⚠️ Found %1 issue(s) that may affect text rendering.").arg(issuesFound));
            summary->setStyleSheet("color: #f9e2af; font-size: 14px; padding: 10px; background-color: #1e1e2e; border-radius: 8px;");
        }
        resultsLayout->addWidget(summary);
    }

signals:
    void issueFixed();

private:
    HealthChecker *checker;
    QVBoxLayout *resultsLayout;
    QScrollArea *scrollArea;
    QList<HealthIssue> issues;

    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        // Header with check button
        auto *headerLayout = new QHBoxLayout();
        auto *titleLabel = new QLabel("Font Rendering Health Check");
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #cdd6f4;");
        headerLayout->addWidget(titleLabel);
        headerLayout->addStretch();

        auto *checkBtn = new QPushButton("Run Check");
        checkBtn->setMinimumHeight(36);
        checkBtn->setStyleSheet(
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
        connect(checkBtn, &QPushButton::clicked, this, &HealthCheckWidget::runCheck);
        headerLayout->addWidget(checkBtn);

        layout->addLayout(headerLayout);

        // Results area
        scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

        auto *resultsWidget = new QWidget();
        resultsLayout = new QVBoxLayout(resultsWidget);
        resultsLayout->setContentsMargins(0, 10, 0, 0);
        resultsLayout->setSpacing(8);
        resultsLayout->addStretch();
        scrollArea->setWidget(resultsWidget);
        layout->addWidget(scrollArea);
    }

    void addIssueCard(const HealthIssue &issue) {
        auto *card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);

        QString statusColor = issue.detected ? "#f38ba8" : "#a6e3a1";
        QString statusIcon = issue.detected ? "❌" : "✅";

        card->setStyleSheet(QString(
            "QFrame { background-color: #1e1e2e; border: 1px solid %1; border-radius: 8px; padding: 12px; }"
        ).arg(issue.detected ? "#45475a" : "#313244"));

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 8, 12, 8);
        cardLayout->setSpacing(4);

        // Title row
        auto *titleRow = new QHBoxLayout();
        auto *iconLabel = new QLabel(statusIcon);
        iconLabel->setStyleSheet("font-size: 16px;");
        titleRow->addWidget(iconLabel);

        auto *titleLabel = new QLabel(issue.title);
        titleLabel->setStyleSheet(QString("font-weight: bold; font-size: 13px; color: %1;").arg(statusColor));
        titleRow->addWidget(titleLabel);

        auto *categoryLabel = new QLabel(issue.category);
        categoryLabel->setStyleSheet("color: #6c7086; font-size: 11px; padding: 2px 6px; background-color: #313244; border-radius: 4px;");
        titleRow->addWidget(categoryLabel);
        titleRow->addStretch();

        cardLayout->addLayout(titleRow);

        // Description
        auto *descLabel = new QLabel(issue.description);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet("color: #a6adc8; font-size: 12px;");
        cardLayout->addWidget(descLabel);

        // Fix button (if fixable and detected)
        if (issue.detected && issue.autoFixable && !issue.fixCommand.isEmpty()) {
            auto *fixRow = new QHBoxLayout();
            fixRow->addStretch();

            auto *fixBtn = new QPushButton(QString("Fix: %1").arg(issue.fixDescription));
            fixBtn->setMinimumHeight(30);
            fixBtn->setStyleSheet(
                "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 4px; padding: 6px 12px; font-weight: bold; font-size: 11px; border: none; }"
                "QPushButton:hover { background-color: #b4f9b8; }"
            );

            QString issueId = issue.id;
            connect(fixBtn, &QPushButton::clicked, this, [this, issueId, card]() {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    card, "Confirm Fix",
                    "Apply this fix? You may be prompted for your password.",
                    QMessageBox::Yes | QMessageBox::No
                );
                if (reply == QMessageBox::Yes) {
                    if (checker->fixIssue(issueId)) {
                        QMessageBox::information(card, "Fixed", "Fix applied successfully. Running re-check...");
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

        // Insert before the stretch at the end
        resultsLayout->insertWidget(resultsLayout->count() - 1, card);
    }
};

// ---- Font List Tab ----
class FontListWidget : public QWidget {
    Q_OBJECT

public:
    explicit FontListWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
        scanFonts();
    }

private slots:
    void scanFonts() {
        statusBar->showMessage("Scanning fonts...");
        allFonts.clear();

        QProcess proc;
        proc.start("fc-list");
        proc.waitForFinished(10000);
        QString output = proc.readAllStandardOutput();

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            int firstColon = line.indexOf(": ");
            if (firstColon == -1) continue;

            QString file = line.left(firstColon).trimmed();
            QString rest = line.mid(firstColon + 2).trimmed();

            int secondColon = rest.indexOf(":style=");
            QString family;
            QString style = "Regular";

            if (secondColon != -1) {
                family = rest.left(secondColon).trimmed();
                style = rest.mid(secondColon + 7).trimmed();
                int comma = style.indexOf(',');
                if (comma != -1) style = style.left(comma).trimmed();
            } else {
                family = rest;
            }

            int familyComma = family.indexOf(',');
            if (familyComma != -1) family = family.left(familyComma).trimmed();

            allFonts.append({file, family, style});
        }

        std::sort(allFonts.begin(), allFonts.end(), [](const FontInfo &a, const FontInfo &b) {
            return a.family.compare(b.family, Qt::CaseInsensitive) < 0;
        });

        populateTable();
    }

    void filterFonts(const QString &text) {
        Q_UNUSED(text);
        populateTable();
    }

    void rebuildFontCache() {
        statusBar->showMessage("Rebuilding font cache...");
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int, QProcess::ExitStatus) {
            QApplication::restoreOverrideCursor();
            QMessageBox::information(this, "Success",
                "Font cache rebuilt. You may need to log out and back in for full effect.");
            scanFonts();
        });

        // Try project script first
        QString scriptPath = QCoreApplication::applicationDirPath() + "/../../fix-lxqt-text-rendering.sh";
        if (QFile::exists(scriptPath)) {
            proc->start("bash", {scriptPath});
        } else {
            proc->start("fc-cache", {"-r", "-v"});
        }
    }

private:
    QTableWidget *table;
    QLineEdit *searchBar;
    QStatusBar *statusBar;
    QList<FontInfo> allFonts;

    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        searchBar = new QLineEdit();
        searchBar->setPlaceholderText("Search fonts by family or style...");
        searchBar->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(searchBar, &QLineEdit::textChanged, this, &FontListWidget::filterFonts);
        layout->addWidget(searchBar);

        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Family", "Style", "File"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }"
        );
        layout->addWidget(table);

        auto *btnLayout = new QHBoxLayout();
        auto *refreshBtn = new QPushButton("Refresh List");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &FontListWidget::scanFonts);
        btnLayout->addWidget(refreshBtn);
        btnLayout->addStretch();

        auto *rebuildBtn = new QPushButton("Rebuild Font Cache");
        rebuildBtn->setMinimumHeight(36);
        rebuildBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(rebuildBtn, &QPushButton::clicked, this, &FontListWidget::rebuildFontCache);
        btnLayout->addWidget(rebuildBtn);
        layout->addLayout(btnLayout);

        statusBar = new QStatusBar();
        statusBar->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar->showMessage("Ready");
        layout->addWidget(statusBar);
    }

    void populateTable() {
        QString filter = searchBar->text().toLower();
        table->setRowCount(0);

        int row = 0;
        for (const auto &font : allFonts) {
            if (!filter.isEmpty() && !font.family.toLower().contains(filter) && !font.style.toLower().contains(filter))
                continue;

            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(font.family));
            table->setItem(row, 1, new QTableWidgetItem(font.style));
            auto *fileItem = new QTableWidgetItem(font.file);
            fileItem->setForeground(QColor("#a6adc8"));
            table->setItem(row, 2, fileItem);
            row++;
        }

        statusBar->showMessage(QString("Showing %1 / %2 fonts").arg(row).arg(allFonts.size()));
    }
};

// ---- Main Window ----
class FontGui : public QMainWindow {
    Q_OBJECT

public:
    FontGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Naravisuals Font Manager");
        resize(900, 650);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        // Tab widget
        tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        // Health Check tab
        healthWidget = new HealthCheckWidget();
        tabs->addTab(healthWidget, "🔍 Health Check");

        // Font List tab
        fontListWidget = new FontListWidget();
        tabs->addTab(fontListWidget, "📋 Font List");

        mainLayout->addWidget(tabs);

        // Style
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
        );

        // Auto-run health check on startup
        QTimer::singleShot(100, healthWidget, &HealthCheckWidget::runCheck);
    }

private:
    QTabWidget *tabs;
    HealthCheckWidget *healthWidget;
    FontListWidget *fontListWidget;
};

#include "main.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Naravisuals Font Manager");

    FontGui gui;
    gui.show();
    return app.exec();
}
