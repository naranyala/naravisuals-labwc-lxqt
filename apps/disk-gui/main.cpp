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
#include <QGroupBox>

struct PartitionInfo {
    QString device;
    QString fstype;
    QString size;
    QString mountpoint;
    QString label;
};

class DiskGui : public QMainWindow {
    Q_OBJECT

public:
    DiskGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Disk Information");
        resize(650, 480);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("View disk partitions, usage, and SMART health");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Partition table ---
        table = new QTableWidget();
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({"Device", "Type", "Size", "Mount", "Label"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }"
        );
        layout->addWidget(table);

        // --- SMART info ---
        smartLabel = new QLabel("");
        smartLabel->setStyleSheet("color: #a6adc8; font-size: 13px; padding: 8px; background-color: #181825; border-radius: 6px;");
        smartLabel->setWordWrap(true);
        layout->addWidget(smartLabel);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &DiskGui::refreshDisks);
        btnRow->addWidget(refreshBtn);

        auto *smartBtn = new QPushButton("Check SMART");
        smartBtn->setMinimumHeight(38);
        smartBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(smartBtn, &QPushButton::clicked, this, &DiskGui::checkSmart);
        btnRow->addWidget(smartBtn);

        layout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
        refreshDisks();
    }

private slots:
    void refreshDisks() {
        partitions.clear();

        QProcess p;
        p.start("lsblk", {"-lnpo", "NAME,TYPE,FSTYPE,SIZE,MOUNTPOINT,LABEL"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 4) continue;
            if (parts[1] != "part") continue;
            PartitionInfo pi;
            pi.device = parts[0];
            pi.fstype = parts[2];
            pi.size = parts[3];
            pi.mountpoint = parts.size() > 4 ? parts[4] : "";
            pi.label = parts.size() > 5 ? parts.mid(5).join(' ') : "";
            partitions.append(pi);
        }

        table->setRowCount(0);
        for (const auto &pi : partitions) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(pi.device));
            table->setItem(row, 1, new QTableWidgetItem(pi.fstype));
            table->setItem(row, 2, new QTableWidgetItem(pi.size));
            table->setItem(row, 3, new QTableWidgetItem(pi.mountpoint));
            table->setItem(row, 4, new QTableWidgetItem(pi.label));
        }
        statusBar()->showMessage(QString("Found %1 partition(s)").arg(partitions.size()));
    }

    void checkSmart() {
        int row = table->currentRow();
        if (row < 0) { smartLabel->setText("Select a partition first"); return; }
        QString device = table->item(row, 0)->text();

        // Get parent disk
        QString disk = device;
        if (disk.contains("p")) disk = disk.left(disk.lastIndexOf('p'));

        QProcess p;
        p.start("sudo", {"smartctl", "-H", disk});
        p.waitForFinished(10000);
        QString out = p.readAllStandardOutput();
        if (out.isEmpty()) {
            out = p.readAllStandardError();
        }

        if (out.contains("PASSED") || out.contains("OK")) {
            smartLabel->setText(QString("SMART Health: OK ✅\nDevice: %1").arg(disk));
            smartLabel->setStyleSheet("color: #a6e3a1; font-size: 13px; padding: 8px; background-color: #181825; border-radius: 6px;");
        } else if (out.trimmed().isEmpty()) {
            smartLabel->setText(QString("smartctl not available or device doesn't support SMART.\nInstall smartmontools: sudo apt install smartmontools"));
            smartLabel->setStyleSheet("color: #f9e2af; font-size: 13px; padding: 8px; background-color: #181825; border-radius: 6px;");
        } else {
            smartLabel->setText(QString("SMART output for %1:\n%2").arg(disk, out.left(500)));
            smartLabel->setStyleSheet("color: #a6adc8; font-size: 13px; padding: 8px; background-color: #181825; border-radius: 6px;");
        }
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QTableWidget *table;
    QLabel *smartLabel;
    QList<PartitionInfo> partitions;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];

        if (arg == "--list") {
            QProcess p;
            p.start("lsblk", {"-lnpo", "NAME,TYPE,FSIZE,FSTYPE,MOUNTPOINT"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }

        if (arg == "--smart") {
            if (i + 1 >= argc) {
                printf("Error: --smart requires a device argument\n");
                printf("Usage: disk-gui --smart /dev/sda\n");
                return 1;
            }
            QString device = argv[++i];
            QProcess p;
            p.start("sudo", {"smartctl", "-H", device});
            p.waitForFinished(10000);
            QString out = p.readAllStandardOutput();
            if (out.isEmpty()) out = p.readAllStandardError();
            if (out.isEmpty()) {
                printf("smartctl not available or device doesn't support SMART.\n");
                printf("Install: sudo apt install smartmontools\n");
            } else {
                printf("%s", out.constData());
            }
            return 0;
        }

        if (arg == "--usage") {
            QProcess p;
            p.start("df", {"-h", "-x", "tmpfs", "-x", "devtmpfs", "-x", "squashfs", "-x", "overlay"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }

        if (arg == "--info") {
            if (i + 1 >= argc) {
                printf("Error: --info requires a device argument\n");
                printf("Usage: disk-gui --info /dev/sda1\n");
                return 1;
            }
            QString device = argv[++i];
            printf("=== Device Info: %s ===\n\n", qPrintable(device));

            QProcess p1;
            p1.start("lsblk", {"-lnpo", "NAME,TYPE,FSTYPE,SIZE,UUID,MOUNTPOINT,LABEL", device});
            p1.waitForFinished(5000);
            QString lsblkOut = p1.readAllStandardOutput().trimmed();
            if (!lsblkOut.isEmpty()) {
                printf("lsblk:\n%s\n\n", qPrintable(lsblkOut));
            } else {
                printf("lsblk: device not found\n\n");
            }

            QProcess p2;
            p2.start("blkid", {"-p", device});
            p2.waitForFinished(5000);
            QString blkidOut = p2.readAllStandardOutput().trimmed();
            if (!blkidOut.isEmpty()) {
                printf("blkid:\n%s\n", qPrintable(blkidOut));
            }

            return 0;
        }

        printf("Unknown argument: %s\n", qPrintable(arg));
        printf("Usage: disk-gui [--list | --smart <device> | --usage | --info <device>]\n");
        return 1;
    }
    DiskGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
