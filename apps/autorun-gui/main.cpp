#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QStatusBar>
#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QSettings>
#include <QProcess>
#include <QRegularExpression>

struct AutorunItem {
    QString fileName;
    QString name;
    QString exec;
    bool isHidden;
};

class AutorunGui : public QMainWindow {
    Q_OBJECT

public:
    AutorunGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Autorun Applications Manager");
        resize(800, 500);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // --- Header ---
        auto *header = new QLabel("Manage GUI programs and shell scripts that run on login");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Table ---
        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Name", "Command / Script", "Enabled"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
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
        connect(refreshBtn, &QPushButton::clicked, this, &AutorunGui::loadItems);
        btnLayout->addWidget(refreshBtn);

        btnLayout->addStretch();

        auto *removeBtn = new QPushButton("Remove Selected");
        removeBtn->setMinimumHeight(38);
        removeBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #f5a8c0; }"
            "QPushButton:pressed { background-color: #d16b8a; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, &AutorunGui::removeItem);
        btnLayout->addWidget(removeBtn);

        auto *addBtn = new QPushButton("Add New Program/Script");
        addBtn->setMinimumHeight(38);
        addBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
            "QPushButton:pressed { background-color: #89b885; }"
        );
        connect(addBtn, &QPushButton::clicked, this, &AutorunGui::addItemDialog);
        btnLayout->addWidget(addBtn);

        layout->addLayout(btnLayout);

        // --- Status ---
        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        
        // --- Style ---
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLabel { font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );

        autostartDir = QDir::homePath() + "/.config/autostart";
        QDir().mkpath(autostartDir);

        loadItems();
    }

private slots:
    void loadItems() {
        items.clear();
        QDir dir(autostartDir);
        QStringList filters;
        filters << "*.desktop";
        dir.setNameFilters(filters);
        
        QFileInfoList list = dir.entryInfoList();
        for (const QFileInfo &fileInfo : list) {
            QSettings settings(fileInfo.absoluteFilePath(), QSettings::IniFormat);
            settings.beginGroup("Desktop Entry");
            
            QString name = settings.value("Name", fileInfo.baseName()).toString();
            QString exec = settings.value("Exec", "").toString();
            bool isHidden = settings.value("Hidden", false).toBool();
            
            settings.endGroup();
            
            items.append({fileInfo.fileName(), name, exec, isHidden});
        }
        
        populateTable();
        statusBar()->showMessage(QString("Loaded %1 autorun items from ~/.config/autostart").arg(items.size()));
    }

    void populateTable() {
        table->setRowCount(0);
        int row = 0;
        for (const auto &item : items) {
            table->insertRow(row);
            
            auto *nameItem = new QTableWidgetItem(item.name);
            nameItem->setForeground(QColor("#f9e2af"));
            table->setItem(row, 0, nameItem);
            
            auto *execItem = new QTableWidgetItem(item.exec);
            execItem->setForeground(QColor("#a6adc8"));
            table->setItem(row, 1, execItem);
            
            auto *statusItem = new QTableWidgetItem(item.isHidden ? "Disabled" : "Enabled");
            statusItem->setForeground(item.isHidden ? QColor("#f38ba8") : QColor("#a6e3a1"));
            table->setItem(row, 2, statusItem);
            
            row++;
        }
    }

    void addItemDialog() {
        QString execPath = QFileDialog::getOpenFileName(this, "Select Executable or Script", QDir::homePath());
        if (execPath.isEmpty()) return;

        bool ok;
        QString name = QInputDialog::getText(this, "Application Name",
                                             "Enter a friendly name for this autorun item:", QLineEdit::Normal,
                                             QFileInfo(execPath).baseName(), &ok);
        if (!ok || name.isEmpty()) return;

        QString safeName = name;
        safeName.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
        QString desktopFileName = autostartDir + "/" + safeName + ".desktop";

        QFile file(desktopFileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=" << name << "\n";
            out << "Exec=" << execPath << "\n";
            out << "Hidden=false\n";
            out << "Terminal=false\n";
            file.close();

            // Try to make the script executable just in case
            QProcess::execute("chmod", {"+x", execPath});

            loadItems();
            QMessageBox::information(this, "Success", "Added to autorun successfully.\n\nIt will run automatically on your next login.");
        } else {
            QMessageBox::critical(this, "Error", "Failed to create autorun entry in ~/.config/autostart.");
        }
    }

    void removeItem() {
        int row = table->currentRow();
        if (row < 0 || row >= items.size()) {
            QMessageBox::warning(this, "Warning", "Please select an item to remove.");
            return;
        }

        auto reply = QMessageBox::question(this, "Confirm Remove",
                                           "Are you sure you want to remove '" + items[row].name + "' from autorun?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QString filePath = autostartDir + "/" + items[row].fileName;
            if (QFile::remove(filePath)) {
                loadItems();
            } else {
                QMessageBox::critical(this, "Error", "Failed to remove the file.");
            }
        }
    }

private:
    QTableWidget *table;
    QList<AutorunItem> items;
    QString autostartDir;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--add" && i + 2 < argc) {
            QString name = argv[++i];
            QString execPath = argv[++i];
            QString autostartDir = QDir::homePath() + "/.config/autostart";
            QDir().mkpath(autostartDir);
            QString safeName = name;
            safeName.replace(QRegularExpression("[^a-zA-Z0-9]"), "_");
            QString desktopFileName = autostartDir + "/" + safeName + ".desktop";
            QFile file(desktopFileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "[Desktop Entry]\nType=Application\nName=" << name << "\nExec=" << execPath << "\nHidden=false\nTerminal=false\n";
                file.close();
                QProcess::execute("chmod", {"+x", execPath});
            }
            return 0;
        } else if (arg == "--remove" && i + 1 < argc) {
            QString fileName = argv[++i];
            QString autostartDir = QDir::homePath() + "/.config/autostart";
            QFile::remove(autostartDir + "/" + fileName);
            return 0;
        }
    }

    AutorunGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
