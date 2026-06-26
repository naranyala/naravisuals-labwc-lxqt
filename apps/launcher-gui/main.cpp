#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QStatusBar>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QIcon>
#include <QFileIconProvider>
#include <QProcess>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QSet>
#include <QFileDialog>
#include <algorithm>

struct DesktopApp {
    QString basename;
    QString name;
    QString genericName;
    QString icon;
    QString exec;
    QString categories;
    QString comment;
    bool noDisplay;
    bool hidden;
};

class LauncherGui : public QMainWindow {
public:
    LauncherGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Panel Launcher Configurator");
        resize(600, 700);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(8);
        mainLayout->setContentsMargins(12, 12, 12, 12);

        auto *targetRow = new QHBoxLayout();
        auto *targetLabel = new QLabel("Configuration Target:");
        m_targetCombo = new QComboBox();
        m_targetCombo->addItem("LXQt Panel Quicklaunch");
        m_targetCombo->addItem("Crystal Dock Pinned Apps");
        targetRow->addWidget(targetLabel);
        targetRow->addWidget(m_targetCombo, 1);
        mainLayout->addLayout(targetRow);

        m_tabs = new QTabWidget();
        mainLayout->addWidget(m_tabs);

        // --- Tab 1: Desktop Apps ---
        auto *desktopTab = new QWidget();
        auto *desktopLayout = new QVBoxLayout(desktopTab);
        desktopLayout->setContentsMargins(8, 8, 8, 8);

        auto *filterRow = new QHBoxLayout();
        m_searchEdit = new QLineEdit();
        m_searchEdit->setPlaceholderText("Filter apps...");
        filterRow->addWidget(m_searchEdit);
        m_categoryCombo = new QComboBox();
        m_categoryCombo->addItem("All Categories");
        filterRow->addWidget(m_categoryCombo);
        desktopLayout->addLayout(filterRow);

        m_appList = new QListWidget();
        m_appList->setSelectionMode(QAbstractItemView::MultiSelection);
        m_appList->setIconSize(QSize(24, 24));
        desktopLayout->addWidget(m_appList);

        auto *countRow = new QHBoxLayout();
        m_countLabel = new QLabel("0 apps available");
        countRow->addWidget(m_countLabel);
        countRow->addStretch();
        m_selectedLabel = new QLabel("0 selected");
        countRow->addWidget(m_selectedLabel);
        desktopLayout->addLayout(countRow);

        m_tabs->addTab(desktopTab, "Desktop Apps");

        // --- Tab 2: Binary Executables ---
        auto *binaryTab = new QWidget();
        auto *binaryLayout = new QVBoxLayout(binaryTab);
        binaryLayout->setContentsMargins(8, 8, 8, 8);

        auto *binFilterRow = new QHBoxLayout();
        m_binSearchEdit = new QLineEdit();
        m_binSearchEdit->setPlaceholderText("Search /usr/bin executables...");
        binFilterRow->addWidget(m_binSearchEdit);
        auto *btnScan = new QPushButton("Rescan");
        binFilterRow->addWidget(btnScan);
        auto *btnBrowse = new QPushButton("Browse...");
        binFilterRow->addWidget(btnBrowse);
        binaryLayout->addLayout(binFilterRow);

        m_binList = new QListWidget();
        m_binList->setSelectionMode(QAbstractItemView::MultiSelection);
        m_binList->setIconSize(QSize(24, 24));
        binaryLayout->addWidget(m_binList);

        auto *binCountRow = new QHBoxLayout();
        m_binCountLabel = new QLabel("0 binaries available");
        binCountRow->addWidget(m_binCountLabel);
        binCountRow->addStretch();
        m_binSelectedLabel = new QLabel("0 selected");
        binCountRow->addWidget(m_binSelectedLabel);
        binaryLayout->addLayout(binCountRow);

        m_tabs->addTab(binaryTab, "Binary Executables");

        // --- Bottom buttons ---
        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnSelectAll = new QPushButton("Select All");
        btnLayout->addWidget(btnSelectAll);
        auto *btnDeselectAll = new QPushButton("Deselect All");
        btnLayout->addWidget(btnDeselectAll);
        m_btnApply = new QPushButton("Apply to Panel");
        btnLayout->addWidget(m_btnApply);
        m_btnRestart = new QPushButton("Restart Panel");
        btnLayout->addWidget(m_btnRestart);
        mainLayout->addLayout(btnLayout);

        statusBar()->showMessage("Ready");

        loadDesktopFiles();
        loadBinaries();
        loadCurrentSelection();
        applyStyle();

        connect(m_searchEdit, &QLineEdit::textChanged, this, &LauncherGui::filterApps);
        connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LauncherGui::filterApps);
        connect(m_binSearchEdit, &QLineEdit::textChanged, this, &LauncherGui::filterBinaries);
        connect(btnScan, &QPushButton::clicked, this, &LauncherGui::loadBinaries);
        connect(btnBrowse, &QPushButton::clicked, this, &LauncherGui::browseBinary);
        connect(btnSelectAll, &QPushButton::clicked, this, &LauncherGui::selectAll);
        connect(btnDeselectAll, &QPushButton::clicked, this, &LauncherGui::deselectAll);
        connect(m_targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            loadCurrentSelection();
            if (index == 0) {
                m_btnApply->setText("Apply to Panel");
                m_btnRestart->setText("Restart Panel");
            } else {
                m_btnApply->setText("Apply to Dock");
                m_btnRestart->setText("Restart Dock");
            }
        });
        connect(m_btnApply, &QPushButton::clicked, this, &LauncherGui::applyToPanel);
        connect(m_btnRestart, &QPushButton::clicked, this, &LauncherGui::restartPanel);
        connect(m_appList, &QListWidget::itemSelectionChanged, this, &LauncherGui::updateSelectedCount);
        connect(m_binList, &QListWidget::itemSelectionChanged, this, &LauncherGui::updateBinSelectedCount);
    }

private:
    QComboBox *m_targetCombo;
    QPushButton *m_btnApply;
    QPushButton *m_btnRestart;
    QTabWidget *m_tabs;
    QListWidget *m_appList;
    QListWidget *m_binList;
    QLineEdit *m_searchEdit;
    QLineEdit *m_binSearchEdit;
    QComboBox *m_categoryCombo;
    QLabel *m_countLabel;
    QLabel *m_selectedLabel;
    QLabel *m_binCountLabel;
    QLabel *m_binSelectedLabel;
    QList<DesktopApp> m_apps;
    QStringList m_binaries;
    QSet<QString> m_currentSelection;
    QSet<QString> m_binSelection;

    QString panelConfPath() {
        QString path = QDir::homePath() + "/.config/lxqt/panel.conf";
        if (QFile::exists(path))
            return path;
        QString xdgConfig = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        return xdgConfig + "/lxqt/panel.conf";
    }

    QString crystalDockConfPath() {
        QString xdgConfig = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QString path = xdgConfig + "/crystal-dock/LXQt/panel_1.conf";
        if (QFile::exists(path))
            return path;
        path = xdgConfig + "/crystal-dock/Budgie/panel_1.conf";
        if (QFile::exists(path))
            return path;
        return xdgConfig + "/crystal-dock/panel_1.conf";
    }

    void scanDesktopDir(const QString &dirPath) {
        QDir dir(dirPath);
        if (!dir.exists())
            return;
        const auto entries = dir.entryList(QStringList() << "*.desktop", QDir::Files);
        for (const QString &entry : entries) {
            DesktopApp app;
            app.basename = entry.chopped(8); // remove .desktop

            QFile file(dir.absoluteFilePath(entry));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;

            bool inDesktopEntry = false;
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line == "[Desktop Entry]") {
                    inDesktopEntry = true;
                    continue;
                }
                if (line.startsWith('[') && line.endsWith(']')) {
                    inDesktopEntry = false;
                    continue;
                }
                if (!inDesktopEntry)
                    continue;

                if (line.startsWith("Name=") && app.name.isEmpty())
                    app.name = line.mid(5);
                else if (line.startsWith("GenericName="))
                    app.genericName = line.mid(12);
                else if (line.startsWith("Icon="))
                    app.icon = line.mid(5);
                else if (line.startsWith("Exec="))
                    app.exec = line.mid(5);
                else if (line.startsWith("Categories="))
                    app.categories = line.mid(11);
                else if (line.startsWith("Comment="))
                    app.comment = line.mid(8);
                else if (line == "NoDisplay=true")
                    app.noDisplay = true;
                else if (line == "Hidden=true")
                    app.hidden = true;
            }
            file.close();

            if (app.name.isEmpty())
                app.name = app.basename;
            if (app.hidden || app.noDisplay)
                continue;
            if (app.exec.isEmpty())
                continue;

            m_apps.append(app);
        }
    }

    void loadDesktopFiles() {
        m_apps.clear();

        QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        dataDirs << "/usr/share";
        for (const QString &d : dataDirs) {
            scanDesktopDir(d + "/applications");
        }
        scanDesktopDir(QDir::homePath() + "/.local/share/applications");

        std::sort(m_apps.begin(), m_apps.end(), [](const DesktopApp &a, const DesktopApp &b) {
            return a.name.toLower() < b.name.toLower();
        });

        QSet<QString> categories;
        for (const DesktopApp &app : m_apps) {
            const auto cats = app.categories.split(';', Qt::SkipEmptyParts);
            for (const QString &c : cats)
                categories.insert(c.trimmed());
        }
        m_categoryCombo->clear();
        m_categoryCombo->addItem("All Categories");
        QStringList catList = categories.values();
        catList.sort();
        for (const QString &c : catList)
            m_categoryCombo->addItem(c);

        filterApps();
        m_countLabel->setText(QString("%1 apps available").arg(m_apps.size()));
    }

    void loadCurrentSelection() {
        m_currentSelection.clear();

        if (m_targetCombo->currentIndex() == 0) {
            // LXQt Panel
            QString confPath = panelConfPath();
            if (QFile::exists(confPath)) {
                QFile file(confPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    bool inQuicklaunch = false;
                    QTextStream in(&file);
                    while (!in.atEnd()) {
                        QString line = in.readLine().trimmed();
                        if (line == "[quicklaunch]") {
                            inQuicklaunch = true;
                            continue;
                        }
                        if (line.startsWith('[') && line.endsWith(']')) {
                            inQuicklaunch = false;
                            continue;
                        }
                        if (!inQuicklaunch)
                            continue;

                        if (line.startsWith("apps=")) {
                            const auto apps = line.mid(5).split(',', Qt::SkipEmptyParts);
                            for (const QString &a : apps)
                                m_currentSelection.insert(a.trimmed());
                        }
                    }
                    file.close();
                }
            }
        } else {
            // Crystal Dock
            QString confPath = crystalDockConfPath();
            if (QFile::exists(confPath)) {
                QFile file(confPath);
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    while (!in.atEnd()) {
                        QString line = in.readLine().trimmed();
                        if (line.startsWith("launchers=")) {
                            QString val = line.mid(10).trimmed();
                            if (val.startsWith('"') && val.endsWith('"')) {
                                val = val.mid(1, val.length() - 2);
                            }
                            const auto apps = val.split(';', Qt::SkipEmptyParts);
                            for (const QString &a : apps) {
                                if (a != "separator")
                                    m_currentSelection.insert(a.trimmed());
                            }
                        }
                    }
                    file.close();
                }
            }
        }

        m_appList->blockSignals(true);
        for (int i = 0; i < m_appList->count(); ++i) {
            auto *item = m_appList->item(i);
            item->setSelected(m_currentSelection.contains(item->data(Qt::UserRole).toString()));
        }
        m_appList->blockSignals(false);
        updateSelectedCount();
    }

    void filterApps() {
        QString filter = m_searchEdit->text().toLower();
        QString category = m_categoryCombo->currentText();

        m_appList->clear();
        for (const DesktopApp &app : m_apps) {
            if (!filter.isEmpty() && !app.name.toLower().contains(filter)
                && !app.genericName.toLower().contains(filter)
                && !app.basename.toLower().contains(filter))
                continue;
            if (category != "All Categories" && !app.categories.contains(category))
                continue;

            auto *item = new QListWidgetItem();
            if (!app.icon.isEmpty()) {
                QIcon icon = QIcon::fromTheme(app.icon);
                if (icon.isNull())
                    icon = QFileIconProvider().icon(QFileInfo("/usr/share/applications/" + app.basename + ".desktop"));
                item->setIcon(icon);
            }
            QString display = app.name;
            if (!app.genericName.isEmpty())
                display += "  (" + app.genericName + ")";
            item->setText(display);
            item->setData(Qt::UserRole, app.basename);
            item->setToolTip(app.comment.isEmpty() ? app.exec : app.comment);
            m_appList->addItem(item);
        }

        for (int i = 0; i < m_appList->count(); ++i) {
            auto *item = m_appList->item(i);
            if (m_currentSelection.contains(item->data(Qt::UserRole).toString()))
                item->setSelected(true);
        }
        updateSelectedCount();
    }

    void loadBinaries() {
        m_binaries.clear();
        scanBinDir("/usr/bin");
        scanBinDir("/usr/local/bin");
        QString localBin = QDir::homePath() + "/.local/bin";
        if (QDir(localBin).exists())
            scanBinDir(localBin);

        m_binaries.removeDuplicates();
        m_binaries.sort();

        m_binCountLabel->setText(QString("%1 binaries available").arg(m_binaries.size()));
        filterBinaries();
    }

    void scanBinDir(const QString &dirPath) {
        QDir dir(dirPath);
        if (!dir.exists())
            return;
        const auto entries = dir.entryList(QDir::Files | QDir::Executable);
        for (const QString &entry : entries) {
            QFileInfo fi(dir.absoluteFilePath(entry));
            if (fi.isDir())
                continue;
            if (entry.contains('.'))
                continue;
            m_binaries.append(fi.absoluteFilePath());
        }
    }

    void filterBinaries() {
        QString filter = m_binSearchEdit->text().toLower();

        m_binList->clear();
        for (const QString &binPath : m_binaries) {
            QString name = QFileInfo(binPath).fileName();
            if (!filter.isEmpty() && !name.toLower().contains(filter)
                && !binPath.toLower().contains(filter))
                continue;

            auto *item = new QListWidgetItem();
            QIcon icon = QFileIconProvider().icon(QFileInfo(binPath));
            item->setIcon(icon);
            item->setText(name);
            item->setData(Qt::UserRole, binPath);
            item->setToolTip(binPath);
            m_binList->addItem(item);
        }

        for (int i = 0; i < m_binList->count(); ++i) {
            auto *item = m_binList->item(i);
            if (m_binSelection.contains(item->data(Qt::UserRole).toString()))
                item->setSelected(true);
        }
        updateBinSelectedCount();
    }

    void browseBinary() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Select Executable",
            "/usr/bin",
            "All Files (*)"
        );
        if (filePath.isEmpty())
            return;

        QFileInfo fi(filePath);
        if (!fi.exists() || !fi.isFile()) {
            QMessageBox::warning(this, "Invalid", "Selected path is not a valid file.");
            return;
        }

        if (!m_binaries.contains(filePath)) {
            m_binaries.append(filePath);
            m_binaries.sort();
            m_binCountLabel->setText(QString("%1 binaries available").arg(m_binaries.size()));
        }

        filterBinaries();

        for (int i = 0; i < m_binList->count(); ++i) {
            auto *item = m_binList->item(i);
            if (item->data(Qt::UserRole).toString() == filePath) {
                item->setSelected(true);
                m_binList->scrollToItem(item);
                break;
            }
        }
        updateBinSelectedCount();
        statusBar()->showMessage("Added: " + filePath);
    }

    void updateBinSelectedCount() {
        int count = 0;
        for (int i = 0; i < m_binList->count(); ++i) {
            if (m_binList->item(i)->isSelected())
                count++;
        }
        m_binSelectedLabel->setText(QString("%1 selected").arg(count));
    }

    void selectAll() {
        QListWidget *active = (m_tabs->currentIndex() == 0) ? m_appList : m_binList;
        for (int i = 0; i < active->count(); ++i)
            active->item(i)->setSelected(true);
    }

    void deselectAll() {
        QListWidget *active = (m_tabs->currentIndex() == 0) ? m_appList : m_binList;
        for (int i = 0; i < active->count(); ++i)
            active->item(i)->setSelected(false);
    }

    void updateSelectedCount() {
        int count = 0;
        for (int i = 0; i < m_appList->count(); ++i) {
            if (m_appList->item(i)->isSelected())
                count++;
        }
        m_selectedLabel->setText(QString("%1 selected").arg(count));
    }

    QStringList selectedBasenames() {
        QStringList result;
        for (int i = 0; i < m_appList->count(); ++i) {
            if (m_appList->item(i)->isSelected())
                result.append(m_appList->item(i)->data(Qt::UserRole).toString());
        }
        for (int i = 0; i < m_binList->count(); ++i) {
            if (m_binList->item(i)->isSelected()) {
                QString binPath = m_binList->item(i)->data(Qt::UserRole).toString();
                QString desktopName = ensureDesktopForBinary(binPath);
                if (!desktopName.isEmpty())
                    result.append(desktopName);
            }
        }
        return result;
    }

    QString ensureDesktopForBinary(const QString &binPath) {
        QFileInfo fi(binPath);
        QString name = fi.fileName();
        QString desktopName = "nv-bin-" + name;
        QString desktopPath = QDir::homePath() + "/.local/share/applications/" + desktopName + ".desktop";

        QFile existing(desktopPath);
        if (existing.exists())
            return desktopName;

        QFile file(desktopPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return QString();
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=" << name << "\n";
        out << "Exec=" << binPath << "\n";
        out << "Icon=application-x-executable\n";
        out << "Terminal=false\n";
        out << "Categories=System;\n";
        file.close();

        statusBar()->showMessage("Created desktop file: " + desktopName);
        return desktopName;
    }

    void applyToPanel() {
        if (m_targetCombo->currentIndex() == 0) {
            // LXQt Panel
            QString confPath = panelConfPath();
            if (!QFile::exists(confPath)) {
                QMessageBox::warning(this, "Error", "panel.conf not found:\n" + confPath);
                return;
            }

            QFile file(confPath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, "Error", "Cannot read panel.conf");
                return;
            }

            QStringList lines;
            QTextStream in(&file);
            while (!in.atEnd())
                lines.append(in.readLine());
            file.close();

            bool inQuicklaunch = false;
            bool appsWritten = false;
            QStringList newLines;
            QStringList selected = selectedBasenames();
            QString appsValue = selected.join(',');

            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines[i].trimmed();

                if (line == "[quicklaunch]") {
                    inQuicklaunch = true;
                    newLines.append(lines[i]);
                    continue;
                }
                if (line.startsWith('[') && line.endsWith(']')) {
                    if (inQuicklaunch && !appsWritten) {
                        newLines.append("apps=" + appsValue);
                        appsWritten = true;
                    }
                    inQuicklaunch = false;
                    newLines.append(lines[i]);
                    continue;
                }

                if (inQuicklaunch && line.startsWith("apps=")) {
                    newLines.append("apps=" + appsValue);
                    appsWritten = true;
                    continue;
                }

                newLines.append(lines[i]);
            }

            if (inQuicklaunch && !appsWritten)
                newLines.append("apps=" + appsValue);

            if (!QFile::exists(confPath + ".bak"))
                QFile::copy(confPath, confPath + ".bak");

            QFile outFile(confPath);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QMessageBox::warning(this, "Error", "Cannot write panel.conf");
                return;
            }
            QTextStream out(&outFile);
            for (const QString &line : newLines)
                out << line << "\n";
            outFile.close();

            m_currentSelection.clear();
            for (const QString &s : selected)
                m_currentSelection.insert(s);

            statusBar()->showMessage(QString("Applied %1 apps to panel quicklaunch").arg(selected.size()));
            QMessageBox::information(this, "Applied",
                QString("%1 apps saved to panel.conf.\nClick 'Restart Panel' to apply.").arg(selected.size()));
        } else {
            // Crystal Dock
            QString confPath = crystalDockConfPath();
            if (!QFile::exists(confPath)) {
                QMessageBox::warning(this, "Error", "Crystal Dock panel_1.conf not found:\n" + confPath);
                return;
            }

            QFile file(confPath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, "Error", "Cannot read Crystal Dock panel_1.conf");
                return;
            }

            QStringList lines;
            QTextStream in(&file);
            while (!in.atEnd())
                lines.append(in.readLine());
            file.close();

            QStringList newLines;
            QStringList selected = selectedBasenames();

            // Count separators in original config and append them
            int separatorCount = 0;
            for (const QString &line : lines) {
                if (line.trimmed().startsWith("launchers=")) {
                    QString val = line.trimmed().mid(10).trimmed();
                    if (val.startsWith('"') && val.endsWith('"')) val = val.mid(1, val.length() - 2);
                    for (const QString &a : val.split(';', Qt::SkipEmptyParts)) {
                        if (a.trimmed() == "separator") separatorCount++;
                    }
                }
            }

            for (int i = 0; i < separatorCount; ++i) {
                selected.append("separator");
            }

            QString launchersValue = "\"" + selected.join(';') + "\"";
            bool launchersWritten = false;

            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines[i].trimmed();
                if (line.startsWith("launchers=")) {
                    newLines.append("launchers=" + launchersValue);
                    launchersWritten = true;
                    continue;
                }
                newLines.append(lines[i]);
            }

            if (!launchersWritten) {
                newLines.append("launchers=" + launchersValue);
            }

            if (!QFile::exists(confPath + ".bak"))
                QFile::copy(confPath, confPath + ".bak");

            QFile outFile(confPath);
            if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QMessageBox::warning(this, "Error", "Cannot write Crystal Dock panel_1.conf");
                return;
            }
            QTextStream out(&outFile);
            for (const QString &line : newLines)
                out << line << "\n";
            outFile.close();

            m_currentSelection.clear();
            for (const QString &s : selected) {
                if (s != "separator")
                    m_currentSelection.insert(s);
            }

            statusBar()->showMessage(QString("Applied %1 apps to Crystal Dock").arg(selected.size() - separatorCount));
            QMessageBox::information(this, "Applied",
                QString("%1 apps saved to Crystal Dock config.\nClick 'Restart Dock' to apply.").arg(selected.size() - separatorCount));
        }
    }

    void restartPanel() {
        if (m_targetCombo->currentIndex() == 0) {
            QProcess::startDetached("pkill", QStringList() << "lxqt-panel");
            QTimer::singleShot(500, this, []() {
                QProcess::startDetached("lxqt-panel");
            });
            statusBar()->showMessage("Panel restarted");
        } else {
            QProcess::startDetached("pkill", QStringList() << "crystal-dock");
            QTimer::singleShot(500, this, []() {
                QProcess::startDetached("crystal-dock");
            });
            statusBar()->showMessage("Crystal Dock restarted");
        }
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: 'Noto Sans', sans-serif; font-size: 13px; }"
            "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
            "QComboBox { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; selection-background-color: #45475a; }"
            "QListWidget { background-color: #181825; border: 1px solid #313244; border-radius: 6px; padding: 4px; }"
            "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
            "QListWidget::item:selected { background-color: #45475a; color: #cdd6f4; }"
            "QListWidget::item:hover { background-color: #313244; }"
            "QLabel { color: #a6adc8; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
            "QTabWidget::pane { border: 1px solid #45475a; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; border: 1px solid #45475a; padding: 8px 20px; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
            "QTabBar::tab:selected { background-color: #313244; color: #cdd6f4; }"
            "QTabBar::tab:hover { background-color: #45475a; }"
        );
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nv-launcher-gui");
    LauncherGui gui;
    gui.show();
    return app.exec();
}
