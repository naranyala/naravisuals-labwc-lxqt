#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QTabWidget>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

struct EnvVar {
    QString key;
    QString value;
};

class EnvGui : public QMainWindow {
    Q_OBJECT

public:
    EnvGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Environment Variables Manager");
        resize(700, 450);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *outerLayout = new QVBoxLayout(central);
        outerLayout->setContentsMargins(16, 16, 16, 16);

        auto *tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        // --- Variables Tab ---
        auto *varsTab = new QWidget();
        auto *layout = new QVBoxLayout(varsTab);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // Header
        auto *header = new QLabel("Manage Wayland and System Environment Variables");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // Table
        table = new QTableWidget(this);
        table->setObjectName("envTable");
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({"Variable", "Value"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 6px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; border: 1px solid #313244; font-weight: bold; }"
        );
        layout->addWidget(table);

        // Buttons
        auto *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &EnvGui::loadConfig);
        btnLayout->addWidget(refreshBtn);

        btnLayout->addStretch();

        auto *removeBtn = new QPushButton("Remove Selected");
        removeBtn->setMinimumHeight(38);
        removeBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #f5a8c0; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, &EnvGui::removeVar);
        btnLayout->addWidget(removeBtn);

        auto *addBtn = new QPushButton("Add / Edit Variable");
        addBtn->setMinimumHeight(38);
        addBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(addBtn, &QPushButton::clicked, this, &EnvGui::addVarDialog);
        btnLayout->addWidget(addBtn);

        layout->addLayout(btnLayout);

        tabs->addTab(varsTab, "Variables");

        // --- Health Check Tab ---
        auto *checker = new EnvHealthChecker(this);
        auto *healthWidget = new HealthCheckWidget(checker);
        tabs->addTab(healthWidget, "Health Check");

        outerLayout->addWidget(tabs);

        // Status
        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        // Style
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );

        envDir = QDir::homePath() + "/.config/environment.d";
        envFile = envDir + "/99-labwc.conf";
        QDir().mkpath(envDir);

        loadConfig();
    }

private slots:
    void loadConfig() {
        vars.clear();
        QFile file(envFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith("#")) continue;
                
                int eqIdx = line.indexOf('=');
                if (eqIdx != -1) {
                    QString key = line.left(eqIdx).trimmed();
                    QString val = line.mid(eqIdx + 1).trimmed();
                    if (key.startsWith("export ")) key = key.mid(7).trimmed();
                    vars.append({key, val});
                }
            }
            file.close();
        }
        populateTable();
        statusBar()->showMessage(QString("Loaded %1 variables from %2").arg(vars.size()).arg(envFile));
    }

    void populateTable() {
        table->setRowCount(0);
        int row = 0;
        for (const auto &v : vars) {
            table->insertRow(row);
            
            auto *kItem = new QTableWidgetItem(v.key);
            kItem->setForeground(QColor("#f9e2af"));
            table->setItem(row, 0, kItem);
            
            auto *vItem = new QTableWidgetItem(v.value);
            vItem->setForeground(QColor("#a6adc8"));
            table->setItem(row, 1, vItem);
            row++;
        }
    }

    void saveConfig() {
        QFile file(envFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Auto-generated by Naravisuals Environment Configurator\n";
            for (const auto &v : vars) {
                out << v.key << "=" << v.value << "\n";
            }
            file.close();
        } else {
            QMessageBox::critical(this, "Error", "Failed to save config to " + envFile);
        }
    }

    void addVarDialog() {
        bool ok;
        QString key = QInputDialog::getText(this, "Variable Name",
                                            "Enter variable name (e.g. MOZ_ENABLE_WAYLAND):", QLineEdit::Normal,
                                            "", &ok);
        if (!ok || key.isEmpty()) return;

        QString val = QInputDialog::getText(this, "Variable Value",
                                            "Enter value for " + key + ":", QLineEdit::Normal,
                                            "", &ok);
        if (!ok) return;

        bool found = false;
        for (auto &v : vars) {
            if (v.key == key) {
                v.value = val;
                found = true;
                break;
            }
        }
        if (!found) vars.append({key, val});

        saveConfig();
        loadConfig();
    }

    void removeVar() {
        int row = table->currentRow();
        if (row < 0 || row >= vars.size()) return;

        auto reply = QMessageBox::question(this, "Confirm",
                                           "Remove variable " + vars[row].key + "?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            vars.removeAt(row);
            saveConfig();
            loadConfig();
        }
    }

private:
    QTableWidget *table;
    QList<EnvVar> vars;
    QString envDir;
    QString envFile;
};

#endif // MAINWINDOW_H
