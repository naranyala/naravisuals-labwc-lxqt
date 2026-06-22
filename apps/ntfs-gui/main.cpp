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
#include <QDir>
#include <QTextStream>
#include <QGroupBox>

#include <QStatusBar>
#include <QMainWindow>

struct Partition {
    QString path;
    QString fsType;
    QString size;
    QString label;
    QString uuid;
    bool mounted;
    QString mountPoint;
};

class NtfsGui : public QMainWindow {
    Q_OBJECT

public:
    NtfsGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("NTFS Partition Manager");
        resize(720, 480);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // --- Header ---
        auto *header = new QLabel("Select a partition to mount or unmount");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Partition Table ---
        table = new QTableWidget();
        table->setColumnCount(6);
        table->setHorizontalHeaderLabels({"Device", "Label", "FS Type", "Size", "Status", "Mount Point"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }"
        );
        layout->addWidget(table);

        // --- Buttons ---
        auto *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &NtfsGui::scanPartitions);
        btnLayout->addWidget(refreshBtn);

        btnLayout->addStretch();

        mountBtn = new QPushButton("Mount");
        mountBtn->setMinimumHeight(38);
        mountBtn->setEnabled(false);
        connect(mountBtn, &QPushButton::clicked, this, &NtfsGui::mountPartition);
        btnLayout->addWidget(mountBtn);

        unmountBtn = new QPushButton("Unmount");
        unmountBtn->setMinimumHeight(38);
        unmountBtn->setEnabled(false);
        unmountBtn->setStyleSheet(
            "QPushButton { background-color: #bf616a; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #d08770; }"
            "QPushButton:pressed { background-color: #a6523e; }"
            "QPushButton:disabled { background-color: #45475a; color: #585b70; }"
        );
        connect(unmountBtn, &QPushButton::clicked, this, &NtfsGui::unmountPartition);
        btnLayout->addWidget(unmountBtn);

        layout->addLayout(btnLayout);

        // --- Status ---
        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar()->showMessage("Ready");

        // --- Connect table selection ---
        connect(table, &QTableWidget::itemSelectionChanged, this, &NtfsGui::onSelectionChanged);

        // --- Style ---
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLabel { font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
            "QPushButton:disabled { background-color: #45475a; color: #585b70; }"
        );

        // --- Scan on startup ---
        scanPartitions();
    }

private slots:
    void scanPartitions() {
        statusBar()->showMessage("Scanning partitions...");
        table->setRowCount(0);
        partitions.clear();

        QProcess proc;
        proc.start("lsblk", {"-lnpo", "NAME,TYPE,FSTYPE,SIZE"});
        proc.waitForFinished(5000);
        QString output = proc.readAllStandardOutput();

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 4) continue;
            if (parts[1] != "part") continue;
            // Skip loop devices
            if (parts[0].startsWith("/dev/loop")) continue;

            Partition p;
            p.path = parts[0].trimmed();
            p.fsType = parts[2].trimmed();
            p.size = parts[3].trimmed();

            // Get label
            QProcess blkid;
            blkid.start("blkid", {"-s", "LABEL", "-o", "value", p.path});
            blkid.waitForFinished(3000);
            p.label = blkid.readAllStandardOutput().trimmed();

            // Get UUID
            QProcess uuidProc;
            uuidProc.start("blkid", {"-s", "UUID", "-o", "value", p.path});
            uuidProc.waitForFinished(3000);
            p.uuid = uuidProc.readAllStandardOutput().trimmed();

            // Check if mounted
            QProcess mountProc;
            mountProc.start("findmnt", {"-n", "-o", "TARGET", p.path});
            mountProc.waitForFinished(3000);
            QString mountOut = mountProc.readAllStandardOutput().trimmed();
            p.mounted = !mountOut.isEmpty();
            p.mountPoint = mountOut;

            partitions.append(p);
        }

        // Populate table
        table->setRowCount(partitions.size());
        for (int i = 0; i < partitions.size(); ++i) {
            const Partition &p = partitions[i];
            auto addItem = [&](int col, const QString &text, const QColor &color = QColor("#cdd6f4")) {
                auto *item = new QTableWidgetItem(text);
                item->setForeground(color);
                table->setItem(i, col, item);
            };

            addItem(0, p.path);
            addItem(1, p.label.isEmpty() ? "(no label)" : p.label, p.label.isEmpty() ? QColor("#585b70") : QColor("#cdd6f4"));
            addItem(2, p.fsType);
            addItem(3, p.size);

            if (p.mounted) {
                addItem(4, "Mounted", QColor("#a6e3a1"));
                addItem(5, p.mountPoint);
            } else {
                addItem(4, "Not mounted", QColor("#585b70"));
                addItem(5, "-");
            }
        }

        statusBar()->showMessage(QString("Found %1 partition(s)").arg(partitions.size()));
    }

    void onSelectionChanged() {
        int row = table->currentRow();
        if (row < 0 || row >= partitions.size()) {
            mountBtn->setEnabled(false);
            unmountBtn->setEnabled(false);
            return;
        }

        const Partition &p = partitions[row];
        mountBtn->setEnabled(!p.mounted);
        unmountBtn->setEnabled(p.mounted);
    }

    void mountPartition() {
        int row = table->currentRow();
        if (row < 0 || row >= partitions.size()) return;
        const Partition &p = partitions[row];

        QString userName = qgetenv("USER");
        if (userName.isEmpty()) userName = "naranyala";
        QString mountBase = "/media/" + userName;
        QString diskName = p.label.isEmpty() ? p.uuid : p.label;
        QString mountPoint = mountBase + "/" + diskName;

        // Build the mount script
        QString script = QString(
            "mkdir -p '%1' && "
            "mount '%2' '%3'"
        ).arg(mountPoint, p.path, mountPoint);

        // Use pkexec to run the mount command
        QProcess proc;
        proc.start("pkexec", {"bash", "-c", script});
        proc.waitForFinished(30000);

        if (proc.exitCode() == 0) {
            QMessageBox::information(this, "Success",
                QString("Successfully mounted at:\n%1").arg(mountPoint));
            statusBar()->showMessage(QString("Mounted %1 at %2").arg(p.path, mountPoint));
        } else {
            QString err = proc.readAllStandardError().trimmed();
            if (err.isEmpty()) err = proc.readAllStandardOutput().trimmed();
            QMessageBox::critical(this, "Mount Failed",
                QString("Failed to mount %1\n\n%2").arg(p.path, err));
            statusBar()->showMessage("Mount failed");
        }

        scanPartitions();
    }

    void unmountPartition() {
        int row = table->currentRow();
        if (row < 0 || row >= partitions.size()) return;
        const Partition &p = partitions[row];

        QString script = QString("umount '%1'").arg(p.path);

        QProcess proc;
        proc.start("pkexec", {"bash", "-c", script});
        proc.waitForFinished(30000);

        if (proc.exitCode() == 0) {
            QMessageBox::information(this, "Success",
                QString("Successfully unmounted %1").arg(p.path));
            statusBar()->showMessage(QString("Unmounted %1").arg(p.path));
        } else {
            QString err = proc.readAllStandardError().trimmed();
            if (err.isEmpty()) err = proc.readAllStandardOutput().trimmed();
            QMessageBox::critical(this, "Unmount Failed",
                QString("Failed to unmount %1\n\n%2").arg(p.path, err));
            statusBar()->showMessage("Unmount failed");
        }

        scanPartitions();
    }

private:
    QTableWidget *table;
    QPushButton *mountBtn;
    QPushButton *unmountBtn;
    QList<Partition> partitions;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    NtfsGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
