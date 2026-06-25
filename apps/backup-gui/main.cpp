#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QFileDialog>
#include <QThread>

struct BackupEntry {
    QString path;
    QString size;
    QString date;
};

class BackupCreateWorker : public QThread {
    Q_OBJECT
public:
    BackupCreateWorker(const QString &output, const QStringList &files, QObject *parent = nullptr)
        : QThread(parent), outputPath(output), sourceFiles(files) {}

    void run() override {
        QStringList args = {"-czf", outputPath, "-T", "-"};

        QProcess proc;
        proc.start("tar", args);
        proc.waitForStarted();

        for (const QString &f : sourceFiles) {
            proc.write((f + "\n").toUtf8());
        }
        proc.closeWriteChannel();
        proc.waitForFinished(60000);

        emit finished(proc.exitCode() == 0, proc.readAllStandardError());
    }

signals:
    void finished(bool success, const QString &error);

private:
    QString outputPath;
    QStringList sourceFiles;
};

class BackupGui : public QMainWindow {
    Q_OBJECT

public:
    BackupGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Backup & Restore");
        resize(650, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Backup and restore your LXQt/labwc configurations");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        // What to backup
        auto *selectGroup = new QGroupBox("What to Backup");
        selectGroup->setStyleSheet(groupBoxStyle());
        auto *selectLayout = new QVBoxLayout(selectGroup);

        lxqtCheck = new QCheckBox("LXQt config (~/.config/lxqt/)");
        lxqtCheck->setChecked(true);
        selectLayout->addWidget(lxqtCheck);

        labwcCheck = new QCheckBox("Labwc config (~/.config/labwc/)");
        labwcCheck->setChecked(true);
        selectLayout->addWidget(labwcCheck);

        themesCheck = new QCheckBox("Themes (~/.themes, ~/.icons, ~/.local/share/icons)");
        themesCheck->setChecked(true);
        selectLayout->addWidget(themesCheck);

        fontsCheck = new QCheckBox("Fonts (~/.local/share/fonts)");
        fontsCheck->setChecked(true);
        selectLayout->addWidget(fontsCheck);

        gtkCheck = new QCheckBox("GTK settings (~/.config/gtk-*/)");
        gtkCheck->setChecked(true);
        selectLayout->addWidget(gtkCheck);

        qtCheck = new QCheckBox("Qt settings (~/.config/qt6ct/, ~/.config/Kvantum/)");
        qtCheck->setChecked(true);
        selectLayout->addWidget(qtCheck);

        mainLayout->addWidget(selectGroup);

        // Existing backups
        auto *backupGroup = new QGroupBox("Available Backups");
        backupGroup->setStyleSheet(groupBoxStyle());
        auto *backupLayout = new QVBoxLayout(backupGroup);

        backupTable = new QTableWidget();
        backupTable->setColumnCount(3);
        backupTable->setHorizontalHeaderLabels({"File", "Size", "Date"});
        backupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        backupTable->setSelectionMode(QAbstractItemView::SingleSelection);
        backupTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        backupTable->verticalHeader()->setVisible(false);
        backupTable->horizontalHeader()->setStretchLastSection(true);
        backupTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        backupTable->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; "
            "border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; "
            "border: 1px solid #313244; font-weight: bold; }"
        );
        backupLayout->addWidget(backupTable);

        mainLayout->addWidget(backupGroup);

        // Buttons
        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);

        auto *backupBtn = new QPushButton("Create Backup");
        backupBtn->setMinimumHeight(40);
        backupBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
            "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(backupBtn, &QPushButton::clicked, this, &BackupGui::createBackup);
        btnRow->addWidget(backupBtn);

        auto *restoreBtn = new QPushButton("Restore Selected");
        restoreBtn->setMinimumHeight(40);
        restoreBtn->setStyleSheet(
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
        connect(restoreBtn, &QPushButton::clicked, this, &BackupGui::restoreBackup);
        btnRow->addWidget(restoreBtn);

        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(40);
        connect(refreshBtn, &QPushButton::clicked, this, &BackupGui::refreshBackups);
        btnRow->addWidget(refreshBtn);

        mainLayout->addLayout(btnRow);

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;"
        );
        statusBar()->showMessage("Ready");

        applyStyle();
        refreshBackups();
    }

private slots:
    void createBackup() {
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        QString backupDir = QDir::homePath() + "/.local/share/naravisuals/backups";
        QDir().mkpath(backupDir);
        QString outputPath = backupDir + "/naravisuals-backup-" + timestamp + ".tar.gz";

        // Collect files
        QStringList files;
        if (lxqtCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.config/lxqt");
        }
        if (labwcCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.config/labwc");
        }
        if (themesCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.themes");
            appendDirFiles(files, QDir::homePath() + "/.icons");
            appendDirFiles(files, QDir::homePath() + "/.local/share/icons");
        }
        if (fontsCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.local/share/fonts");
        }
        if (gtkCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.config/gtk-3.0");
            appendDirFiles(files, QDir::homePath() + "/.config/gtk-4.0");
        }
        if (qtCheck->isChecked()) {
            appendDirFiles(files, QDir::homePath() + "/.config/qt6ct");
            appendDirFiles(files, QDir::homePath() + "/.config/Kvantum");
        }

        if (files.isEmpty()) {
            QMessageBox::warning(this, "Nothing Selected", "Select at least one item to backup.");
            return;
        }

        statusBar()->showMessage("Creating backup...");
        QApplication::processEvents();

        auto *worker = new BackupCreateWorker(outputPath, files, this);
        connect(worker, &BackupCreateWorker::finished, this, [this, outputPath, worker](bool ok, const QString &err) {
            if (ok) {
                QFileInfo fi(outputPath);
                statusBar()->showMessage(QString("Backup created: %1 (%2)")
                    .arg(fi.fileName(), formatSize(fi.size())));
                refreshBackups();
            } else {
                QMessageBox::critical(this, "Backup Failed", "Error: " + err);
                statusBar()->showMessage("Backup failed");
            }
            worker->deleteLater();
        });
        worker->start();
    }

    void restoreBackup() {
        auto *item = backupTable->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Select a backup to restore.");
            return;
        }

        int row = item->row();
        QString fileName = backupTable->item(row, 0)->text();
        QString backupDir = QDir::homePath() + "/.local/share/naravisuals/backups";
        QString filePath = backupDir + "/" + fileName;

        auto reply = QMessageBox::question(this, "Confirm Restore",
            QString("Restore configs from:\n%1\n\nThis will overwrite current settings.\nA pre-restore backup will be saved.").arg(fileName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) return;

        statusBar()->showMessage("Restoring...");

        // Extract to home directory
        QProcess proc;
        proc.start("tar", {"-xzf", filePath, "-C", QDir::homePath()});
        proc.waitForFinished(30000);

        if (proc.exitCode() == 0) {
            statusBar()->showMessage("Restore complete. Restart session to apply.");
            QMessageBox::information(this, "Restore Complete",
                "Configs restored successfully.\nRestart your session to apply changes.");
        } else {
            QMessageBox::critical(this, "Restore Failed",
                "Error: " + QString::fromUtf8(proc.readAllStandardError()));
        }
    }

    void refreshBackups() {
        backupTable->setRowCount(0);

        QString backupDir = QDir::homePath() + "/.local/share/naravisuals/backups";
        QDir dir(backupDir);
        QStringList filters = {"naravisuals-backup-*.tar.gz"};
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

        for (const QFileInfo &fi : files) {
            int row = backupTable->rowCount();
            backupTable->insertRow(row);
            backupTable->setItem(row, 0, new QTableWidgetItem(fi.fileName()));
            backupTable->setItem(row, 1, new QTableWidgetItem(formatSize(fi.size())));
            backupTable->setItem(row, 2, new QTableWidgetItem(fi.lastModified().toString("yyyy-MM-dd HH:mm")));
        }

        statusBar()->showMessage(QString("Found %1 backups").arg(files.size()));
    }

private:
    void appendDirFiles(QStringList &list, const QString &dirPath) {
        QDir dir(dirPath);
        if (!dir.exists()) return;

        QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            list << it.next();
        }
    }

    QString formatSize(qint64 bytes) {
        if (bytes >= 1073741824) return QString::number(bytes / 1073741824.0, 'f', 1) + " GB";
        if (bytes >= 1048576) return QString::number(bytes / 1048576.0, 'f', 1) + " MB";
        if (bytes >= 1024) return QString::number(bytes / 1024.0, 'f', 1) + " KB";
        return QString::number(bytes) + " B";
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QCheckBox { spacing: 8px; }"
            "QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid #313244; "
            "border-radius: 4px; background-color: #181825; }"
            "QCheckBox::indicator:checked { background-color: #a6e3a1; border-color: #a6e3a1; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
    }

    QCheckBox *lxqtCheck;
    QCheckBox *labwcCheck;
    QCheckBox *themesCheck;
    QCheckBox *fontsCheck;
    QCheckBox *gtkCheck;
    QCheckBox *qtCheck;
    QTableWidget *backupTable;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    BackupGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
