#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QStatusBar>
#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QTextStream>

struct MimeItem {
    QString type;
    QString desktopFile;
};

class MimeGui : public QMainWindow {
    Q_OBJECT

public:
    MimeGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Default Applications Configurator");
        resize(800, 500);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // Header
        auto *header = new QLabel("Manage Default Applications (mimeapps.list)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // Table
        table = new QTableWidget();
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({"File Type (MIME)", "Default Application (.desktop)"});
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
        connect(refreshBtn, &QPushButton::clicked, this, &MimeGui::loadConfig);
        btnLayout->addWidget(refreshBtn);

        btnLayout->addStretch();

        auto *removeBtn = new QPushButton("Clear Selected Default");
        removeBtn->setMinimumHeight(38);
        removeBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #f5a8c0; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, &MimeGui::removeVar);
        btnLayout->addWidget(removeBtn);

        auto *addBtn = new QPushButton("Set Default App");
        addBtn->setMinimumHeight(38);
        addBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(addBtn, &QPushButton::clicked, this, &MimeGui::addVarDialog);
        btnLayout->addWidget(addBtn);

        layout->addLayout(btnLayout);

        // Status
        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        // Style
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );

        mimeFile = QDir::homePath() + "/.config/mimeapps.list";
        loadConfig();
    }

private slots:
    void loadConfig() {
        items.clear();
        QSettings settings(mimeFile, QSettings::IniFormat);
        settings.beginGroup("Default Applications");
        
        for (const QString &key : settings.childKeys()) {
            items.append({key, settings.value(key).toString()});
        }
        settings.endGroup();
        
        populateTable();
        statusBar()->showMessage(QString("Loaded %1 MIME types from %2").arg(items.size()).arg(mimeFile));
    }

    void populateTable() {
        table->setRowCount(0);
        int row = 0;
        for (const auto &i : items) {
            table->insertRow(row);
            
            auto *kItem = new QTableWidgetItem(i.type);
            kItem->setForeground(QColor("#f9e2af"));
            table->setItem(row, 0, kItem);
            
            auto *vItem = new QTableWidgetItem(i.desktopFile);
            vItem->setForeground(QColor("#a6adc8"));
            table->setItem(row, 1, vItem);
            row++;
        }
    }

    void saveConfig() {
        // Read file contents except Default Applications
        QString beforeSection;
        QString afterSection;
        bool inDefault = false;
        bool foundDefault = false;
        
        QFile file(mimeFile);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.startsWith("[Default Applications]")) {
                    inDefault = true;
                    foundDefault = true;
                    continue;
                }
                if (inDefault && line.startsWith("[")) {
                    inDefault = false;
                }
                
                if (!inDefault) {
                    if (!foundDefault) beforeSection += line + "\n";
                    else afterSection += line + "\n";
                }
            }
            file.close();
        }

        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (!beforeSection.isEmpty()) out << beforeSection;
            out << "[Default Applications]\n";
            for (const auto &v : items) {
                out << v.type << "=" << v.desktopFile << "\n";
            }
            if (!afterSection.isEmpty()) out << afterSection;
            file.close();
        } else {
            QMessageBox::critical(this, "Error", "Failed to save config to " + mimeFile);
        }
    }

    void addVarDialog() {
        bool ok;
        QString key = QInputDialog::getText(this, "MIME Type",
                                            "Enter MIME type (e.g. video/mp4, text/html):", QLineEdit::Normal,
                                            "", &ok);
        if (!ok || key.isEmpty()) return;

        QString desktopPath = QFileDialog::getOpenFileName(this, "Select Application .desktop File", "/usr/share/applications", "Desktop Files (*.desktop)");
        if (desktopPath.isEmpty()) return;
        
        QString desktopName = QFileInfo(desktopPath).fileName();

        // Update if exists, else append
        bool found = false;
        for (auto &v : items) {
            if (v.type == key) {
                v.desktopFile = desktopName;
                found = true;
                break;
            }
        }
        if (!found) items.append({key, desktopName});

        saveConfig();
        loadConfig();
    }

    void removeVar() {
        int row = table->currentRow();
        if (row < 0 || row >= items.size()) return;

        auto reply = QMessageBox::question(this, "Confirm",
                                           "Clear default app for " + items[row].type + "?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            items.removeAt(row);
            saveConfig();
            loadConfig();
        }
    }

private:
    QTableWidget *table;
    QList<MimeItem> items;
    QString mimeFile;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MimeGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
