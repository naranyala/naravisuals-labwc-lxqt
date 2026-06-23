#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QTextStream>
#include <QStatusBar>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QListWidget>
#include <QComboBox>
#include <QTabWidget>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>
#include <QSet>
#include <QMap>

struct DesktopEntry {
    QString filePath;
    QString type;
    QString name;
    QString genericName;
    QString comment;
    QString icon;
    QString exec;
    QString path;
    bool terminal = false;
    QStringList categories;
    QStringList mimeType;
    QString startupWMClass;
    QStringList keywords;
    bool noDisplay = false;
    bool hidden = false;
    QStringList onlyShowIn;
    QStringList notShowIn;
    QString dbusActivatable;
    QString tryExec;
};

class DesktopGui : public QMainWindow {
    Q_OBJECT

public:
    DesktopGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Desktop Entry Manager");
        resize(1000, 650);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(8);
        mainLayout->setContentsMargins(12, 12, 12, 12);

        // Toolbar
        auto *toolbar = new QHBoxLayout();
        toolbar->setSpacing(8);

        searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search desktop entries...");
        searchEdit->setMinimumHeight(34);
        searchEdit->setClearButtonEnabled(true);
        connect(searchEdit, &QLineEdit::textChanged, this, &DesktopGui::filterEntries);
        toolbar->addWidget(searchEdit);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(34);
        connect(refreshBtn, &QPushButton::clicked, this, &DesktopGui::refreshList);
        toolbar->addWidget(refreshBtn);

        auto *newBtn = new QPushButton("New Entry");
        newBtn->setMinimumHeight(34);
        newBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(newBtn, &QPushButton::clicked, this, &DesktopGui::newEntry);
        toolbar->addWidget(newBtn);

        mainLayout->addLayout(toolbar);

        // Main splitter
        auto *splitter = new QSplitter(Qt::Horizontal);

        // Left panel - tree
        auto *leftPanel = new QWidget();
        auto *leftLayout = new QVBoxLayout(leftPanel);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        tree = new QTreeWidget();
        tree->setHeaderLabels({"Desktop Entries"});
        tree->setAnimated(true);
        tree->setExpandsOnDoubleClick(true);
        tree->setRootIsDecorated(true);
        tree->setMinimumWidth(300);
        tree->header()->setStretchLastSection(true);
        tree->setStyleSheet(
            "QTreeWidget { background-color: #1e1e2e; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; }"
            "QTreeWidget::item { padding: 4px; }"
            "QTreeWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QTreeWidget::branch:has-siblings:!adjoins-item { border-image: none; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 4px; border: 1px solid #313244; font-weight: bold; }"
        );
        connect(tree, &QTreeWidget::currentItemChanged, this, &DesktopGui::onEntrySelected);
        leftLayout->addWidget(tree);

        splitter->addWidget(leftPanel);

        // Right panel - details
        auto *rightPanel = new QWidget();
        auto *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(8, 0, 0, 0);

        detailStack = new QStackedWidget();

        // Welcome page
        auto *welcomePage = new QWidget();
        auto *welcomeLayout = new QVBoxLayout(welcomePage);
        welcomeLayout->addStretch();
        auto *welcomeLabel = new QLabel("Select a desktop entry from the list\nor create a new one");
        welcomeLabel->setAlignment(Qt::AlignCenter);
        welcomeLabel->setStyleSheet("font-size: 18px; color: #585b70; padding: 40px;");
        welcomeLayout->addWidget(welcomeLabel);
        welcomeLayout->addStretch();
        detailStack->addWidget(welcomePage);

        // Detail page
        detailPage = new QWidget();
        buildDetailPage();
        detailStack->addWidget(detailPage);

        detailStack->setCurrentIndex(0);
        rightLayout->addWidget(detailStack);

        splitter->addWidget(rightPanel);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        mainLayout->addWidget(splitter);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 13px; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QLineEdit:focus { border-color: #89b4fa; }"
            "QCheckBox { spacing: 6px; }"
            "QCheckBox::indicator { width: 18px; height: 18px; border-radius: 4px; border: 1px solid #45475a; background-color: #181825; }"
            "QCheckBox::indicator:checked { background-color: #89b4fa; border-color: #89b4fa; }"
            "QTextEdit { background-color: #181825; border: 1px solid #313244; border-radius: 4px; color: #cdd6f4; padding: 4px; }"
            "QComboBox { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QComboBox:hover { border-color: #89b4fa; }"
            "QComboBox::drop-down { border: none; background-color: #313244; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; selection-background-color: #313244; border: 1px solid #45475a; }"
            "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding-top: 16px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 5px; padding: 8px 14px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
            "QLabel { color: #cdd6f4; }"
        );

        scanDirectories();
    }

private slots:
    void refreshList() {
        searchEdit->clear();
        scanDirectories();
        statusBar()->showMessage("Refreshed desktop entries", 3000);
    }

    void filterEntries(const QString &text) {
        for (int i = 0; i < tree->topLevelItemCount(); ++i) {
            auto *dirItem = tree->topLevelItem(i);
            bool dirVisible = false;
            for (int j = 0; j < dirItem->childCount(); ++j) {
                auto *entry = dirItem->child(j);
                bool matches = text.isEmpty() ||
                    entry->text(0).contains(text, Qt::CaseInsensitive) ||
                    entry->data(0, Qt::UserRole).toString().contains(text, Qt::CaseInsensitive);
                entry->setHidden(!matches);
                if (matches) dirVisible = true;
            }
            dirItem->setHidden(!dirVisible);
            if (dirVisible) dirItem->setExpanded(true);
        }
    }

    void onEntrySelected(QTreeWidgetItem *current, QTreeWidgetItem *) {
        if (!current || !current->parent()) {
            detailStack->setCurrentIndex(0);
            return;
        }
        QString filePath = current->data(0, Qt::UserRole).toString();
        if (filePath.isEmpty()) {
            detailStack->setCurrentIndex(0);
            return;
        }
        loadEntry(filePath);
        detailStack->setCurrentIndex(1);
    }

    void newEntry() {
        bool ok;
        QString dir = QInputDialog::getItem(this, "Target Directory",
            "Where to create the desktop entry?",
            writableDirs, 0, false, &ok);
        if (!ok || dir.isEmpty()) return;

        QString name = QInputDialog::getText(this, "Entry Name",
            "Application name:", QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;

        QString exec = QInputDialog::getText(this, "Executable",
            "Command to run:", QLineEdit::Normal, "", &ok);
        if (!ok || exec.isEmpty()) return;

        QString fileName = name.toLower().replace(QRegularExpression("[^a-zA-Z0-9]"), "-") + ".desktop";
        QString filePath = dir + "/" + fileName;

        QFile file(filePath);
        if (file.exists()) {
            QMessageBox::warning(this, "Exists", "A file named " + fileName + " already exists.");
            return;
        }

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot write to " + filePath);
            return;
        }

        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=" << name << "\n";
        out << "Exec=" << exec << "\n";
        out << "Terminal=false\n";
        out << "Categories=Utility;\n";
        file.close();

        statusBar()->showMessage("Created " + filePath, 5000);
        scanDirectories();
        selectEntry(filePath);
    }

    void saveEntry() {
        if (currentFilePath.isEmpty()) return;

        QFile file(currentFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot write to " + currentFilePath);
            return;
        }

        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=" << typeCombo->currentText() << "\n";
        out << "Name=" << nameEdit->text() << "\n";
        if (!genericNameEdit->text().isEmpty())
            out << "GenericName=" << genericNameEdit->text() << "\n";
        if (!commentEdit->text().isEmpty())
            out << "Comment=" << commentEdit->text() << "\n";
        out << "Exec=" << execEdit->text() << "\n";
        if (!pathEdit->text().isEmpty())
            out << "Path=" << pathEdit->text() << "\n";
        if (!iconEdit->text().isEmpty())
            out << "Icon=" << iconEdit->text() << "\n";
        out << "Terminal=" << (terminalCheck->isChecked() ? "true" : "false") << "\n";
        if (!categoriesEdit->text().isEmpty())
            out << "Categories=" << categoriesEdit->text() << "\n";
        if (!mimeEdit->text().isEmpty())
            out << "MimeType=" << mimeEdit->text() << "\n";
        if (!startupWmEdit->text().isEmpty())
            out << "StartupWMClass=" << startupWmEdit->text() << "\n";
        if (!keywordsEdit->text().isEmpty())
            out << "Keywords=" << keywordsEdit->text() << "\n";
        if (noDisplayCheck->isChecked())
            out << "NoDisplay=true\n";

        file.close();

        statusBar()->showMessage("Saved " + currentFilePath, 3000);
        scanDirectories();
    }

    void deleteEntry() {
        if (currentFilePath.isEmpty()) return;

        auto reply = QMessageBox::question(this, "Confirm Delete",
            "Delete " + currentFilePath + "?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        QFile file(currentFilePath);
        if (file.remove()) {
            statusBar()->showMessage("Deleted " + currentFilePath, 3000);
            currentFilePath.clear();
            detailStack->setCurrentIndex(0);
            scanDirectories();
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete " + currentFilePath);
        }
    }

    void launchEntry() {
        if (currentFilePath.isEmpty()) return;

        DesktopEntry entry = parseDesktopFile(currentFilePath);
        if (entry.exec.isEmpty()) {
            QMessageBox::warning(this, "No Exec", "This entry has no Exec command.");
            return;
        }

        QString cmd = entry.exec;
        cmd.remove(QRegularExpression("%[fFuUdDnNickvm]"));
        cmd = cmd.trimmed();

        if (!QProcess::startDetached("/bin/sh", {"-c", cmd})) {
            QMessageBox::critical(this, "Error", "Failed to launch:\n" + cmd);
        }
    }

    void showExecutable() {
        if (currentFilePath.isEmpty()) return;

        DesktopEntry entry = parseDesktopFile(currentFilePath);
        QString exec = entry.exec;
        exec.remove(QRegularExpression("%[fFuUdDnNickvm]"));
        exec = exec.trimmed();

        // Extract first word as binary path
        QString bin = exec.split(" ").first();
        QString binPath = QStandardPaths::findExecutable(bin);
        if (binPath.isEmpty()) {
            QMessageBox::information(this, "Executable", "Exec: " + exec +
                "\n\nBinary '" + bin + "' not found in PATH.");
            return;
        }

        QFileInfo fi(binPath);
        if (fi.isSymLink()) binPath = fi.symLinkTarget();

        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(binPath).absolutePath()));
        statusBar()->showMessage("Executable: " + binPath, 5000);
    }

    void showIcon() {
        if (currentFilePath.isEmpty()) return;

        DesktopEntry entry = parseDesktopFile(currentFilePath);
        QString iconName = entry.icon;
        if (iconName.isEmpty()) {
            QMessageBox::information(this, "Icon", "No icon specified.");
            return;
        }

        // Check if it's a full path
        if (iconName.startsWith("/")) {
            if (QFile::exists(iconName)) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(iconName).absolutePath()));
            } else {
                QMessageBox::warning(this, "Icon", "Icon path does not exist:\n" + iconName);
            }
            return;
        }

        // Search in standard icon themes
        QStringList iconDirs = {
            "/usr/share/icons",
            QDir::homePath() + "/.local/share/icons",
            "/usr/share/pixmaps"
        };

        QStringList found;
        for (const auto &dir : iconDirs) {
            QDirIterator it(dir, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString fp = it.next();
                if (QFileInfo(fp).baseName() == iconName) {
                    found.append(fp);
                }
            }
        }

        if (found.isEmpty()) {
            QMessageBox::information(this, "Icon",
                "Icon '" + iconName + "' not found in standard paths.");
        } else {
            QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(found.first()).absolutePath()));
            statusBar()->showMessage("Icon: " + found.first(), 5000);
        }
    }

    void duplicateEntry() {
        if (currentFilePath.isEmpty()) return;

        bool ok;
        QString dir = QInputDialog::getItem(this, "Target Directory",
            "Copy to which directory?",
            writableDirs, 0, false, &ok);
        if (!ok || dir.isEmpty()) return;

        QFileInfo fi(currentFilePath);
        QString newPath = dir + "/" + fi.fileName();

        if (QFile::exists(newPath)) {
            QMessageBox::warning(this, "Exists", fi.fileName() + " already exists in target.");
            return;
        }

        if (QFile::copy(currentFilePath, newPath)) {
            statusBar()->showMessage("Duplicated to " + newPath, 3000);
            scanDirectories();
            selectEntry(newPath);
        } else {
            QMessageBox::critical(this, "Error", "Failed to copy file.");
        }
    }

private:
    void buildDetailPage() {
        detailPage = new QWidget();
        auto *pageLayout = new QVBoxLayout(detailPage);
        pageLayout->setSpacing(8);

        auto *scrollContent = new QWidget();
        auto *scrollLayout = new QVBoxLayout(scrollContent);
        scrollLayout->setSpacing(6);

        // Basic info group
        auto *basicGroup = new QGroupBox("Basic Information");
        auto *basicForm = new QFormLayout(basicGroup);
        basicForm->setSpacing(6);
        basicForm->setContentsMargins(10, 20, 10, 10);

        typeCombo = new QComboBox();
        typeCombo->addItems({"Application", "Link", "Directory"});
        basicForm->addRow("Type:", typeCombo);

        nameEdit = new QLineEdit();
        basicForm->addRow("Name:", nameEdit);

        genericNameEdit = new QLineEdit();
        basicForm->addRow("Generic Name:", genericNameEdit);

        commentEdit = new QLineEdit();
        basicForm->addRow("Comment:", commentEdit);

        scrollLayout->addWidget(basicGroup);

        // Execution group
        auto *execGroup = new QGroupBox("Execution");
        auto *execForm = new QFormLayout(execGroup);
        execForm->setSpacing(6);
        execForm->setContentsMargins(10, 20, 10, 10);

        execEdit = new QLineEdit();
        execEdit->setPlaceholderText("/usr/bin/application %f");
        execForm->addRow("Exec:", execEdit);

        pathEdit = new QLineEdit();
        pathEdit->setPlaceholderText("Working directory (optional)");
        execForm->addRow("Path:", pathEdit);

        terminalCheck = new QCheckBox("Run in terminal");
        execForm->addRow("", terminalCheck);

        scrollLayout->addWidget(execGroup);

        // Appearance group
        auto *appGroup = new QGroupBox("Appearance");
        auto *appForm = new QFormLayout(appGroup);
        appForm->setSpacing(6);
        appForm->setContentsMargins(10, 20, 10, 10);

        iconEdit = new QLineEdit();
        iconEdit->setPlaceholderText("Icon name or full path");
        appForm->addRow("Icon:", iconEdit);

        startupWmEdit = new QLineEdit();
        startupWmEdit->setPlaceholderText("WM Class (e.g. firefox)");
        appForm->addRow("StartupWM Class:", startupWmEdit);

        scrollLayout->addWidget(appGroup);

        // Categories group
        auto *catGroup = new QGroupBox("Categories & Associations");
        auto *catForm = new QFormLayout(catGroup);
        catForm->setSpacing(6);
        catForm->setContentsMargins(10, 20, 10, 10);

        categoriesEdit = new QLineEdit();
        categoriesEdit->setPlaceholderText("Network;WebBrowser;");
        catForm->addRow("Categories:", categoriesEdit);

        mimeEdit = new QLineEdit();
        mimeEdit->setPlaceholderText("text/html;x-scheme-handler/http;");
        catForm->addRow("MIME Types:", mimeEdit);

        keywordsEdit = new QLineEdit();
        keywordsEdit->setPlaceholderText("Keyword1;Keyword2;");
        catForm->addRow("Keywords:", keywordsEdit);

        scrollLayout->addWidget(catGroup);

        // Options group
        auto *optGroup = new QGroupBox("Options");
        auto *optForm = new QFormLayout(optGroup);
        optForm->setSpacing(6);
        optForm->setContentsMargins(10, 20, 10, 10);

        noDisplayCheck = new QCheckBox("Hide from menus (NoDisplay)");
        optForm->addRow("", noDisplayCheck);

        scrollLayout->addWidget(optGroup);
        scrollLayout->addStretch();

        pageLayout->addWidget(scrollContent);

        // Bottom action buttons
        auto *actionBar = new QHBoxLayout();
        actionBar->setSpacing(8);

        auto *launchBtn = new QPushButton("Launch");
        launchBtn->setMinimumHeight(36);
        launchBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(launchBtn, &QPushButton::clicked, this, &DesktopGui::launchEntry);
        actionBar->addWidget(launchBtn);

        auto *showExecBtn = new QPushButton("Show Executable");
        showExecBtn->setMinimumHeight(36);
        connect(showExecBtn, &QPushButton::clicked, this, &DesktopGui::showExecutable);
        actionBar->addWidget(showExecBtn);

        auto *showIconBtn = new QPushButton("Show Icon");
        showIconBtn->setMinimumHeight(36);
        connect(showIconBtn, &QPushButton::clicked, this, &DesktopGui::showIcon);
        actionBar->addWidget(showIconBtn);

        actionBar->addStretch();

        auto *duplicateBtn = new QPushButton("Duplicate");
        duplicateBtn->setMinimumHeight(36);
        duplicateBtn->setStyleSheet(
            "QPushButton { background-color: #f9e2af; color: #1e1e2e; }"
            "QPushButton:hover { background-color: #f5e6c0; }"
        );
        connect(duplicateBtn, &QPushButton::clicked, this, &DesktopGui::duplicateEntry);
        actionBar->addWidget(duplicateBtn);

        auto *deleteBtn = new QPushButton("Delete");
        deleteBtn->setMinimumHeight(36);
        deleteBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; }"
            "QPushButton:hover { background-color: #f5a8c0; }"
        );
        connect(deleteBtn, &QPushButton::clicked, this, &DesktopGui::deleteEntry);
        actionBar->addWidget(deleteBtn);

        auto *saveBtn = new QPushButton("Save");
        saveBtn->setMinimumHeight(36);
        saveBtn->setStyleSheet(
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
        connect(saveBtn, &QPushButton::clicked, this, &DesktopGui::saveEntry);
        actionBar->addWidget(saveBtn);

        pageLayout->addLayout(actionBar);
    }

    void scanDirectories() {
        tree->clear();
        entriesByPath.clear();

        QStringList searchDirs = {
            "/usr/share/applications",
            "/usr/local/share/applications",
            QDir::homePath() + "/.local/share/applications",
            "/var/lib/snapd/desktop/applications",
            "/var/lib/flatpak/exports/share/applications"
        };

        writableDirs.clear();

        for (const auto &dir : searchDirs) {
            QDir d(dir);
            if (!d.exists()) continue;

            if (QFileInfo(dir).isWritable())
                writableDirs.append(dir);

            auto *dirItem = new QTreeWidgetItem(tree);
            dirItem->setText(0, dir);
            dirItem->setExpanded(dir == QDir::homePath() + "/.local/share/applications" ||
                                 dir == "/usr/share/applications");

            QFont dirFont = dirItem->font(0);
            dirFont.setBold(true);
            dirItem->setFont(0, dirFont);
            dirItem->setForeground(0, QColor("#89b4fa"));

            QStringList filters;
            filters << "*.desktop";
            QFileInfoList files = d.entryInfoList(filters, QDir::Files, QDir::Name);

            for (const auto &fi : files) {
                DesktopEntry entry = parseDesktopFile(fi.absoluteFilePath());
                auto *child = new QTreeWidgetItem(dirItem);
                child->setText(0, fi.completeBaseName().replace("-", " ").replace("_", " "));
                child->setData(0, Qt::UserRole, fi.absoluteFilePath());
                child->setData(0, Qt::UserRole + 1, entry.name);
                child->setData(0, Qt::UserRole + 2, entry.exec);

                if (entry.name.isEmpty()) {
                    child->setForeground(0, QColor("#f38ba8"));
                } else {
                    child->setForeground(0, QColor("#cdd6f4"));
                }

                child->setToolTip(0, fi.absoluteFilePath() + "\nName: " + entry.name +
                                  "\nExec: " + entry.exec);

                entriesByPath.insert(fi.absoluteFilePath(), child);
            }
        }
    }

    DesktopEntry parseDesktopFile(const QString &filePath) {
        DesktopEntry entry;
        entry.filePath = filePath;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return entry;

        QTextStream in(&file);
        bool inDesktopEntry = false;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();

            if (line.startsWith("[")) {
                inDesktopEntry = line.startsWith("[Desktop Entry]");
                continue;
            }

            if (!inDesktopEntry || line.isEmpty() || line.startsWith("#"))
                continue;

            int eq = line.indexOf('=');
            if (eq < 0) continue;

            QString key = line.left(eq).trimmed();
            QString value = line.mid(eq + 1).trimmed();

            if (key == "Type") entry.type = value;
            else if (key == "Name") entry.name = value;
            else if (key == "GenericName") entry.genericName = value;
            else if (key == "Comment") entry.comment = value;
            else if (key == "Icon") entry.icon = value;
            else if (key == "Exec") entry.exec = value;
            else if (key == "Path") entry.path = value;
            else if (key == "Terminal") entry.terminal = (value == "true");
            else if (key == "Categories") entry.categories = value.split(";", Qt::SkipEmptyParts);
            else if (key == "MimeType") entry.mimeType = value.split(";", Qt::SkipEmptyParts);
            else if (key == "StartupWMClass") entry.startupWMClass = value;
            else if (key == "Keywords") entry.keywords = value.split(";", Qt::SkipEmptyParts);
            else if (key == "NoDisplay") entry.noDisplay = (value == "true");
            else if (key == "Hidden") entry.hidden = (value == "true");
            else if (key == "OnlyShowIn") entry.onlyShowIn = value.split(";", Qt::SkipEmptyParts);
            else if (key == "NotShowIn") entry.notShowIn = value.split(";", Qt::SkipEmptyParts);
            else if (key == "DBusActivatable") entry.dbusActivatable = value;
            else if (key == "TryExec") entry.tryExec = value;
        }

        file.close();
        return entry;
    }

    void loadEntry(const QString &filePath) {
        currentFilePath = filePath;
        DesktopEntry entry = parseDesktopFile(filePath);

        int idx = typeCombo->findText(entry.type.isEmpty() ? "Application" : entry.type);
        if (idx >= 0) typeCombo->setCurrentIndex(idx);
        nameEdit->setText(entry.name);
        genericNameEdit->setText(entry.genericName);
        commentEdit->setText(entry.comment);
        execEdit->setText(entry.exec);
        iconEdit->setText(entry.icon);
        pathEdit->setText(entry.path);
        terminalCheck->setChecked(entry.terminal);
        categoriesEdit->setText(entry.categories.join(";") + (entry.categories.isEmpty() ? "" : ";"));
        mimeEdit->setText(entry.mimeType.join(";") + (entry.mimeType.isEmpty() ? "" : ";"));
        keywordsEdit->setText(entry.keywords.join(";") + (entry.keywords.isEmpty() ? "" : ";"));
        startupWmEdit->setText(entry.startupWMClass);
        noDisplayCheck->setChecked(entry.noDisplay);
    }

    void selectEntry(const QString &filePath) {
        auto it = entriesByPath.constFind(filePath);
        if (it != entriesByPath.constEnd()) {
            tree->setCurrentItem(it.value());
            tree->scrollToItem(it.value());
        }
    }

    QTreeWidget *tree;
    QStackedWidget *detailStack;
    QWidget *detailPage;
    QLineEdit *searchEdit;

    QComboBox *typeCombo;
    QLineEdit *nameEdit;
    QLineEdit *genericNameEdit;
    QLineEdit *commentEdit;
    QLineEdit *execEdit;
    QLineEdit *pathEdit;
    QLineEdit *iconEdit;
    QLineEdit *startupWmEdit;
    QLineEdit *categoriesEdit;
    QLineEdit *mimeEdit;
    QLineEdit *keywordsEdit;
    QCheckBox *terminalCheck;
    QCheckBox *noDisplayCheck;

    QString currentFilePath;
    QStringList writableDirs;
    QMap<QString, QTreeWidgetItem *> entriesByPath;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nv-desktop-gui");
    DesktopGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
