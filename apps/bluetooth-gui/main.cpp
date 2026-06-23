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
#include <QTimer>

struct BtDevice {
    QString mac;
    QString name;
    QString type;
    QString status;
};

class BluetoothGui : public QMainWindow {
    Q_OBJECT

public:
    BluetoothGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Bluetooth Manager");
        resize(600, 460);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Pair, connect, and manage Bluetooth devices");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        table = new QTableWidget();
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"Name", "MAC", "Type", "Status"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }"
        );
        layout->addWidget(table);

        auto *btnRow = new QHBoxLayout();
        btnRow->addSpacing(10);

        auto *scanBtn = new QPushButton("Scan");
        scanBtn->setMinimumHeight(38);
        connect(scanBtn, &QPushButton::clicked, this, &BluetoothGui::scanDevices);
        btnRow->addWidget(scanBtn);

        auto *pairBtn = new QPushButton("Pair & Connect");
        pairBtn->setMinimumHeight(38);
        pairBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(pairBtn, &QPushButton::clicked, this, &BluetoothGui::pairDevice);
        btnRow->addWidget(pairBtn);

        auto *disconnectBtn = new QPushButton("Disconnect");
        disconnectBtn->setMinimumHeight(38);
        disconnectBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }"
        );
        connect(disconnectBtn, &QPushButton::clicked, this, &BluetoothGui::disconnectDevice);
        btnRow->addWidget(disconnectBtn);

        btnRow->addStretch();
        layout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar()->showMessage("Ready");
        applyStyle();
        scanDevices();
    }

private slots:
    void scanDevices() {
        statusBar()->showMessage("Scanning...");
        devices.clear();

        // Get paired devices
        QProcess p;
        p.start("bluetoothctl", {"devices"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            // Format: "Device XX:XX:XX:XX:XX:XX Device Name"
            QString trimmed = line.trimmed();
            if (!trimmed.startsWith("Device")) continue;
            QStringList parts = trimmed.split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 3) continue;

            BtDevice dev;
            dev.mac = parts[1];
            dev.name = parts.mid(2).join(' ');
            dev.type = "Device";
            dev.status = "Paired";
            devices.append(dev);
        }

        // Get info on each paired device
        for (auto &dev : devices) {
            QProcess info;
            info.start("bluetoothctl", {"info", dev.mac});
            info.waitForFinished(3000);
            QString infoOut = info.readAllStandardOutput();
            if (infoOut.contains("Connected: yes")) dev.status = "Connected";
            else dev.status = "Paired";
        }

        populateTable();
    }

    void populateTable() {
        table->setRowCount(0);
        for (const auto &dev : devices) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(dev.name));
            table->setItem(row, 1, new QTableWidgetItem(dev.mac));
            table->setItem(row, 2, new QTableWidgetItem(dev.type));
            table->setItem(row, 3, new QTableWidgetItem(dev.status));
        }
        statusBar()->showMessage(QString("Found %1 device(s)").arg(devices.size()));
    }

    void pairDevice() {
        int row = table->currentRow();
        if (row < 0) { statusBar()->showMessage("Select a device first"); return; }
        QString mac = table->item(row, 1)->text();

        statusBar()->showMessage("Pairing " + mac + "...");
        QProcess::execute("bluetoothctl", {"pair", mac});
        QProcess::execute("bluetoothctl", {"connect", mac});
        scanDevices();
    }

    void disconnectDevice() {
        int row = table->currentRow();
        if (row < 0) { statusBar()->showMessage("Select a device first"); return; }
        QString mac = table->item(row, 1)->text();

        QProcess::execute("bluetoothctl", {"disconnect", mac});
        scanDevices();
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
    QList<BtDevice> devices;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--list") {
            QProcess p;
            p.start("bluetoothctl", {"devices"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
        if (arg == "--pair" && i + 1 < argc) {
            QString mac = argv[++i];
            printf("Pairing with %s...\n", mac.toLocal8Bit().constData());
            QProcess::execute("bluetoothctl", {"pair", mac});
            printf("Connecting to %s...\n", mac.toLocal8Bit().constData());
            QProcess::execute("bluetoothctl", {"connect", mac});
            return 0;
        }
        if (arg == "--disconnect" && i + 1 < argc) {
            QString mac = argv[++i];
            printf("Disconnecting %s...\n", mac.toLocal8Bit().constData());
            QProcess::execute("bluetoothctl", {"disconnect", mac});
            return 0;
        }
        if (arg == "--remove" && i + 1 < argc) {
            QString mac = argv[++i];
            printf("Removing %s...\n", mac.toLocal8Bit().constData());
            QProcess::execute("bluetoothctl", {"remove", mac});
            return 0;
        }
        if (arg == "--scan") {
            printf("Scanning for Bluetooth devices (10 seconds)...\n");
            QProcess p;
            p.start("bash", {"-c",
                "(echo 'scan on'; sleep 10; echo 'scan off'; sleep 1; "
                "echo 'devices'; sleep 1; echo 'quit') | bluetoothctl"});
            p.waitForFinished(20000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
    }
    BluetoothGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
