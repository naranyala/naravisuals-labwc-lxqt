#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QProcess>
#include <QStatusBar>
#include <QTimer>
#include <QTabWidget>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

struct WifiNetwork {
    QString ssid;
    QString signal;
    QString security;
    QString inUse;
};

class NetworkGui : public QMainWindow {
    Q_OBJECT

public:
    NetworkGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("WiFi Manager");
        resize(650, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        auto *tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        // --- WiFi Tab ---
        auto *wifiTab = new QWidget();
        auto *layout = new QVBoxLayout(wifiTab);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Scan, connect, and manage WiFi networks");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        auto *pwRow = new QHBoxLayout();
        auto *pwLabel = new QLabel("Password:");
        pwLabel->setStyleSheet("color: #a6adc8;");
        pwRow->addWidget(pwLabel);
        pwEdit = new QLineEdit();
        pwEdit->setEchoMode(QLineEdit::Password);
        pwEdit->setPlaceholderText("Enter WiFi password...");
        pwEdit->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }");
        pwRow->addWidget(pwEdit, 1);
        layout->addLayout(pwRow);

        table = new QTableWidget();
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"SSID", "Signal", "Security", "Active"});
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
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }");
        layout->addWidget(table);

        auto *btnRow = new QHBoxLayout();
        auto *scanBtn = new QPushButton("Scan");
        scanBtn->setMinimumHeight(36);
        connect(scanBtn, &QPushButton::clicked, this, &NetworkGui::scanNetworks);
        btnRow->addWidget(scanBtn);

        auto *connectBtn = new QPushButton("Connect");
        connectBtn->setMinimumHeight(36);
        connectBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }");
        connect(connectBtn, &QPushButton::clicked, this, &NetworkGui::connectNetwork);
        btnRow->addWidget(connectBtn);

        auto *disconnectBtn = new QPushButton("Disconnect");
        disconnectBtn->setMinimumHeight(36);
        disconnectBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }");
        connect(disconnectBtn, &QPushButton::clicked, this, &NetworkGui::disconnectNetwork);
        btnRow->addWidget(disconnectBtn);

        btnRow->addStretch();
        layout->addLayout(btnRow);
        tabs->addTab(wifiTab, "WiFi");

        // --- Health Check Tab ---
        auto *checker = new NetworkHealthChecker(this);
        auto *healthWidget = new HealthCheckWidget(checker);
        tabs->addTab(healthWidget, "Health Check");

        mainLayout->addWidget(tabs);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
        scanNetworks();
    }

private slots:
    void scanNetworks() {
        statusBar()->showMessage("Scanning...");
        networks.clear();

        QProcess p;
        p.start("nmcli", {"-t", "-f", "SSID,SIGNAL,SECURITY,ACTIVE", "dev", "wifi", "list"});
        p.waitForFinished(10000);
        QString out = p.readAllStandardOutput();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(':');
            if (parts.size() < 3) continue;
            WifiNetwork net;
            net.ssid = parts[0];
            net.signal = parts[1] + "%";
            net.security = parts.mid(2, parts.size() - 3).join(':');
            net.inUse = (parts.last() == "yes") ? "*" : "";
            if (!net.ssid.isEmpty()) networks.append(net);
        }

        table->setRowCount(0);
        for (const auto &net : networks) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(net.ssid));
            table->setItem(row, 1, new QTableWidgetItem(net.signal));
            table->setItem(row, 2, new QTableWidgetItem(net.security));
            auto *activeItem = new QTableWidgetItem(net.inUse);
            activeItem->setForeground(QColor("#a6e3a1"));
            table->setItem(row, 3, activeItem);
        }
        statusBar()->showMessage(QString("Found %1 network(s)").arg(networks.size()));
    }

    void connectNetwork() {
        int row = table->currentRow();
        if (row < 0) { statusBar()->showMessage("Select a network"); return; }
        QString ssid = table->item(row, 0)->text();
        QString pw = pwEdit->text();

        statusBar()->showMessage("Connecting to " + ssid + "...");
        if (pw.isEmpty()) {
            QProcess::startDetached("nmcli", {"dev", "wifi", "connect", ssid});
        } else {
            QProcess::startDetached("nmcli", {"dev", "wifi", "connect", ssid, "password", pw});
        }
        QTimer::singleShot(3000, this, &NetworkGui::scanNetworks);
    }

    void disconnectNetwork() {
        QProcess p;
        p.start("nmcli", {"-t", "-f", "NAME,DEVICE", "con", "show", "--active"});
        p.waitForFinished(3000);
        QString out = p.readAllStandardOutput();
        if (!out.isEmpty()) {
            QString name = out.split('\n').first().split(':').first();
            QProcess::startDetached("nmcli", {"con", "down", name});
        }
        QTimer::singleShot(1000, this, &NetworkGui::scanNetworks);
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }");
    }

    QTableWidget *table;
    QLineEdit *pwEdit;
    QList<WifiNetwork> networks;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    NetworkGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
