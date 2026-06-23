#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QLineEdit>
#include <QStatusBar>
#include <QMainWindow>
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>
#include <QProcess>

struct KeybindInfo {
    QString keyCombo;
    QString action;
    QString command;
};

class KeybindsGui : public QMainWindow {
    Q_OBJECT

public:
    KeybindsGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Global Keybindings Manager");
        resize(850, 600);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // --- Header ---
        auto *header = new QLabel("Global System-Level Keybindings (Labwc)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Search bar ---
        searchBar = new QLineEdit();
        searchBar->setPlaceholderText("Search by key (e.g. W-Return), action, or command...");
        searchBar->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(searchBar, &QLineEdit::textChanged, this, &KeybindsGui::filterBinds);
        layout->addWidget(searchBar);

        // --- Table ---
        table = new QTableWidget();
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Key Combination", "Action", "Command"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
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

        auto *refreshBtn = new QPushButton("Reload");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &KeybindsGui::loadKeybinds);
        btnLayout->addWidget(refreshBtn);

        auto *editBtn = new QPushButton("Edit Config");
        editBtn->setMinimumHeight(38);
        connect(editBtn, &QPushButton::clicked, this, &KeybindsGui::openEditor);
        btnLayout->addWidget(editBtn);

        btnLayout->addStretch();
        
        auto *reconfigBtn = new QPushButton("Apply Changes (Reconfigure)");
        reconfigBtn->setMinimumHeight(38);
        reconfigBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
            "QPushButton:pressed { background-color: #89b885; }"
        );
        connect(reconfigBtn, &QPushButton::clicked, this, &KeybindsGui::reconfigureLabwc);
        btnLayout->addWidget(reconfigBtn);

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

        loadKeybinds();
    }

private slots:
    void loadKeybinds() {
        allBinds.clear();
        
        // Use user's local config if it exists, otherwise fallback to the repo template
        QString configPath = QDir::homePath() + "/.config/labwc/rc.xml";
        if (!QFile::exists(configPath)) {
            configPath = QCoreApplication::applicationDirPath() + "/../../configs/dotfiles/labwc/rc.xml";
        }
        
        if (!QFile::exists(configPath)) {
            // Also try relative to current directory just in case
            configPath = "../../configs/dotfiles/labwc/rc.xml";
        }
        
        if (!QFile::exists(configPath)) {
            statusBar()->showMessage("Could not find labwc rc.xml configuration file.");
            return;
        }

        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            statusBar()->showMessage("Failed to open " + configPath);
            return;
        }

        QXmlStreamReader xml(&file);
        
        QString currentKey;
        
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            
            if (token == QXmlStreamReader::StartElement) {
                if (xml.name().toString() == "keybind") {
                    currentKey = xml.attributes().value("key").toString();
                } else if (xml.name().toString() == "action" && !currentKey.isEmpty()) {
                    QString action = xml.attributes().value("name").toString();
                    QString cmd = xml.attributes().value("command").toString();
                    allBinds.append({currentKey, action, cmd});
                }
            } else if (token == QXmlStreamReader::EndElement) {
                if (xml.name().toString() == "keybind") {
                    currentKey.clear();
                }
            }
        }
        
        populateTable();
        statusBar()->showMessage("Loaded keybindings from " + configPath);
    }

    void populateTable() {
        QString filter = searchBar->text().toLower();
        table->setRowCount(0);
        
        int row = 0;
        for (const auto &bind : allBinds) {
            if (!filter.isEmpty() && 
                !bind.keyCombo.toLower().contains(filter) && 
                !bind.action.toLower().contains(filter) && 
                !bind.command.toLower().contains(filter)) {
                continue;
            }
            
            table->insertRow(row);
            
            auto *keyItem = new QTableWidgetItem(bind.keyCombo);
            keyItem->setForeground(QColor("#f9e2af")); 
            table->setItem(row, 0, keyItem);
            
            auto *actionItem = new QTableWidgetItem(bind.action);
            table->setItem(row, 1, actionItem);
            
            auto *cmdItem = new QTableWidgetItem(bind.command.isEmpty() ? "-" : bind.command);
            cmdItem->setForeground(QColor("#a6adc8")); 
            table->setItem(row, 2, cmdItem);
            
            row++;
        }
    }

    void filterBinds(const QString &) {
        populateTable();
    }

    void openEditor() {
        QString configPath = QDir::homePath() + "/.config/labwc/rc.xml";
        if (!QFile::exists(configPath)) {
            configPath = QCoreApplication::applicationDirPath() + "/../../configs/dotfiles/labwc/rc.xml";
        }
        
        // Use default editor or xdg-open
        QProcess::startDetached("xdg-open", {configPath});
    }

    void reconfigureLabwc() {
        statusBar()->showMessage("Reconfiguring Labwc...");
        QProcess::startDetached("labwc", {"-r"});
        QMessageBox::information(this, "Success", "Sent reconfigure signal to Labwc.\nYour keybinding changes are now active!");
    }

private:
    QTableWidget *table;
    QLineEdit *searchBar;
    QList<KeybindInfo> allBinds;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--reconfigure") {
            QProcess::startDetached("labwc", {"-r"});
            return 0;
        }
    }

    KeybindsGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
