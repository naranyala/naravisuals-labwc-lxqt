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
#include <QRegularExpression>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

struct ServiceInfo {
    QString name;
    QString loadState;
    QString activeState;
    QString subState;
    QString description;
};

class ServiceGui : public QMainWindow {
    Q_OBJECT

public:
    ServiceGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Systemd User Services");
        resize(700, 550);

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

        // --- Services Tab ---
        auto *servicesTab = new QWidget();
        auto *layout = new QVBoxLayout(servicesTab);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Manage systemd user services (start/stop/enable/disable)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        table = new QTableWidget();
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"Service", "Load", "Active", "Sub"});
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
        auto *startBtn = new QPushButton("Start");
        startBtn->setMinimumHeight(36);
        connect(startBtn, &QPushButton::clicked, this, &ServiceGui::startService);
        btnRow->addWidget(startBtn);

        auto *stopBtn = new QPushButton("Stop");
        stopBtn->setMinimumHeight(36);
        stopBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }");
        connect(stopBtn, &QPushButton::clicked, this, &ServiceGui::stopService);
        btnRow->addWidget(stopBtn);

        auto *enableBtn = new QPushButton("Enable");
        enableBtn->setMinimumHeight(36);
        enableBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }");
        connect(enableBtn, &QPushButton::clicked, this, &ServiceGui::enableService);
        btnRow->addWidget(enableBtn);

        auto *disableBtn = new QPushButton("Disable");
        disableBtn->setMinimumHeight(36);
        connect(disableBtn, &QPushButton::clicked, this, &ServiceGui::disableService);
        btnRow->addWidget(disableBtn);

        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &ServiceGui::refreshServices);
        btnRow->addWidget(refreshBtn);

        layout->addLayout(btnRow);
        tabs->addTab(servicesTab, "Services");

        // --- Health Check Tab ---
        auto *checker = new ServiceHealthChecker(this);
        auto *healthWidget = new HealthCheckWidget(checker);
        tabs->addTab(healthWidget, "Health Check");

        mainLayout->addWidget(tabs);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
        refreshServices();
    }

private slots:
    void refreshServices() {
        services.clear();
        QProcess p;
        p.start("systemctl", {"--user", "list-units", "--type=service", "--all", "--no-pager", "--no-legend"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 4) continue;
            ServiceInfo svc;
            svc.name = parts[0];
            svc.loadState = parts[1];
            svc.activeState = parts[2];
            svc.subState = parts[3];
            services.append(svc);
        }

        table->setRowCount(0);
        for (const auto &svc : services) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(svc.name));
            table->setItem(row, 1, new QTableWidgetItem(svc.loadState));
            auto *activeItem = new QTableWidgetItem(svc.activeState);
            if (svc.activeState == "active") activeItem->setForeground(QColor("#a6e3a1"));
            else if (svc.activeState == "failed") activeItem->setForeground(QColor("#f38ba8"));
            table->setItem(row, 2, activeItem);
            table->setItem(row, 3, new QTableWidgetItem(svc.subState));
        }
        statusBar()->showMessage(QString("%1 services loaded").arg(services.size()));
    }

    QString selectedService() {
        int row = table->currentRow();
        if (row < 0) return QString();
        return table->item(row, 0)->text();
    }

    void startService() {
        QString svc = selectedService();
        if (svc.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "start", svc});
        refreshServices();
    }

    void stopService() {
        QString svc = selectedService();
        if (svc.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "stop", svc});
        refreshServices();
    }

    void enableService() {
        QString svc = selectedService();
        if (svc.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "enable", svc});
        refreshServices();
    }

    void disableService() {
        QString svc = selectedService();
        if (svc.isEmpty()) return;
        QProcess::execute("systemctl", {"--user", "disable", svc});
        refreshServices();
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
    QList<ServiceInfo> services;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ServiceGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
