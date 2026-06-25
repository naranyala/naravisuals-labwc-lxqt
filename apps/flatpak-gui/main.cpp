#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QProcess>
#include <QStatusBar>
#include <QTabWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QThread>

struct FlatpakApp {
    QString id;
    QString name;
    QString version;
    QString branch;
    QString origin;
    QString installation;
};

class FlatpakWorker : public QThread {
    Q_OBJECT
public:
    FlatpakWorker(QObject *parent = nullptr) : QThread(parent) {}

    void run() override {
        QProcess proc;
        proc.start("flatpak", {"list", "--app", "--columns=application,name,version,branch,origin,installation"});
        proc.waitForFinished(10000);
        emit finished(QString::fromUtf8(proc.readAllStandardOutput()));
    }

signals:
    void finished(const QString &output);
};

class FlatpakGui : public QMainWindow {
    Q_OBJECT

public:
    FlatpakGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Flatpak Manager");
        resize(750, 550);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Manage Flatpak applications (install, remove, update)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; "
            "border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; "
            "border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        // --- Installed Tab ---
        auto *installedTab = new QWidget();
        auto *instLayout = new QVBoxLayout(installedTab);
        instLayout->setSpacing(12);
        instLayout->setContentsMargins(16, 16, 16, 16);

        table = new QTableWidget();
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({"Name", "ID", "Version", "Branch", "Origin"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->setSortingEnabled(true);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; "
            "border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; "
            "border: 1px solid #313244; font-weight: bold; }"
        );
        instLayout->addWidget(table);

        auto *instBtnRow = new QHBoxLayout();
        instBtnRow->setSpacing(8);

        auto *removeBtn = new QPushButton("Remove");
        removeBtn->setMinimumHeight(36);
        removeBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, &FlatpakGui::removeApp);
        instBtnRow->addWidget(removeBtn);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &FlatpakGui::refreshInstalled);
        instBtnRow->addWidget(refreshBtn);

        instBtnRow->addStretch();

        auto *countLabel = new QLabel("Installed: 0");
        countLabel->setStyleSheet("color: #a6adc8; font-size: 13px;");
        instBtnRow->addWidget(countLabel);
        installedCountLabel = countLabel;

        instLayout->addLayout(instBtnRow);
        tabs->addTab(installedTab, "Installed");

        // --- Search Tab ---
        auto *searchTab = new QWidget();
        auto *searchLayout = new QVBoxLayout(searchTab);
        searchLayout->setSpacing(12);
        searchLayout->setContentsMargins(16, 16, 16, 16);

        auto *searchRow = new QHBoxLayout();
        searchRow->setSpacing(8);

        searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Search Flathub for apps...");
        searchEdit->setMinimumHeight(36);
        searchRow->addWidget(searchEdit, 1);

        auto *searchBtn = new QPushButton("Search");
        searchBtn->setMinimumHeight(36);
        connect(searchBtn, &QPushButton::clicked, this, &FlatpakGui::searchApps);
        connect(searchEdit, &QLineEdit::returnPressed, this, &FlatpakGui::searchApps);
        searchRow->addWidget(searchBtn);

        searchLayout->addLayout(searchRow);

        searchTable = new QTableWidget();
        searchTable->setColumnCount(3);
        searchTable->setHorizontalHeaderLabels({"Name", "App ID", "Description"});
        searchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        searchTable->setSelectionMode(QAbstractItemView::SingleSelection);
        searchTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        searchTable->verticalHeader()->setVisible(false);
        searchTable->horizontalHeader()->setStretchLastSection(true);
        searchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        searchTable->setStyleSheet(table->styleSheet());
        searchLayout->addWidget(searchTable);

        auto *searchBtnRow = new QHBoxLayout();
        searchBtnRow->setSpacing(8);

        auto *installBtn = new QPushButton("Install Selected");
        installBtn->setMinimumHeight(36);
        installBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(installBtn, &QPushButton::clicked, this, &FlatpakGui::installApp);
        searchBtnRow->addWidget(installBtn);

        searchBtnRow->addStretch();
        searchLayout->addLayout(searchBtnRow);

        tabs->addTab(searchTab, "Search Flathub");

        mainLayout->addWidget(tabs);

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;"
        );
        statusBar()->showMessage("Ready");

        applyStyle();
        refreshInstalled();
    }

private slots:
    void refreshInstalled() {
        table->setRowCount(0);

        QProcess proc;
        proc.start("flatpak", {"list", "--app", "--columns=application,name,version,branch,origin"});
        proc.waitForFinished(10000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        installedApps.clear();

        for (const QString &line : lines) {
            QStringList cols = line.split('\t', Qt::SkipEmptyParts);
            if (cols.size() < 5) continue;

            FlatpakApp app;
            app.id = cols[0];
            app.name = cols[1];
            app.version = cols[2];
            app.branch = cols[3];
            app.origin = cols[4];
            installedApps.append(app);

            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(app.name));
            table->setItem(row, 1, new QTableWidgetItem(app.id));
            table->setItem(row, 2, new QTableWidgetItem(app.version));
            table->setItem(row, 3, new QTableWidgetItem(app.branch));
            table->setItem(row, 4, new QTableWidgetItem(app.origin));
        }

        installedCountLabel->setText(QString("Installed: %1").arg(installedApps.size()));
        statusBar()->showMessage(QString("Found %1 installed Flatpak apps").arg(installedApps.size()));
    }

    void removeApp() {
        auto *item = table->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Select an app to remove.");
            return;
        }
        int row = item->row();
        QString appId = table->item(row, 1)->text();
        QString appName = table->item(row, 0)->text();

        auto reply = QMessageBox::question(this, "Remove App",
            QString("Remove %1?\nThis may take a moment.").arg(appName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            statusBar()->showMessage("Removing " + appName + "...");
            QProcess::startDetached("flatpak", {"uninstall", "-y", appId});
            QTimer::singleShot(2000, this, &FlatpakGui::refreshInstalled);
        }
    }

    void searchApps() {
        QString query = searchEdit->text().trimmed();
        if (query.isEmpty()) return;

        searchTable->setRowCount(0);
        statusBar()->showMessage("Searching Flathub...");

        QProcess proc;
        proc.start("flatpak", {"search", query, "--columns=application,name,description"});
        proc.waitForFinished(15000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        int count = 0;

        for (const QString &line : lines) {
            QStringList cols = line.split('\t', Qt::SkipEmptyParts);
            if (cols.size() < 3) continue;

            int row = searchTable->rowCount();
            searchTable->insertRow(row);
            searchTable->setItem(row, 0, new QTableWidgetItem(cols[1]));
            searchTable->setItem(row, 1, new QTableWidgetItem(cols[0]));
            searchTable->setItem(row, 2, new QTableWidgetItem(cols[2]));
            count++;
        }

        statusBar()->showMessage(QString("Found %1 results for \"%2\"").arg(count, query));
    }

    void installApp() {
        auto *item = searchTable->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Select an app to install.");
            return;
        }
        int row = item->row();
        QString appId = searchTable->item(row, 1)->text();
        QString appName = searchTable->item(row, 0)->text();

        auto reply = QMessageBox::question(this, "Install App",
            QString("Install %1 from Flathub?").arg(appName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            statusBar()->showMessage("Installing " + appName + "...");
            QProcess::startDetached("flatpak", {"install", "-y", "flathub", appId});
            QTimer::singleShot(2000, this, &FlatpakGui::refreshInstalled);
        }
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
    }

    QTabWidget *tabs;
    QTableWidget *table;
    QTableWidget *searchTable;
    QLineEdit *searchEdit;
    QLabel *installedCountLabel;
    QList<FlatpakApp> installedApps;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    FlatpakGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
