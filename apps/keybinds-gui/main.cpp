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
#include <QXmlStreamWriter>
#include <QProcess>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QShortcut>
#include <QKeySequence>

struct KeybindInfo {
    QString keyCombo;
    QString action;
    QString command;
    QString direction;
    QString region;
    QString toDesktop;
};

static const QStringList COMMON_ACTIONS = {
    "Execute", "Close", "ToggleMaximize", "ToggleFullscreen",
    "ToggleDecorations", "ToggleShade", "ToggleAlwaysOnTop",
    "ToggleOmnipresent", "Iconify", "Raise", "Lower",
    "GoToDesktop", "SendToDesktop", "SnapToEdge", "SnapToRegion",
    "MoveToEdge", "NextWindow", "PreviousWindow",
    "ShowMenu", "Reconfigure", "Exit", "None"
};

static QString resolveConfigPath() {
    QString path = QDir::homePath() + "/.config/labwc/rc.xml";
    if (QFile::exists(path)) return path;
    path = QCoreApplication::applicationDirPath() + "/../../configs/dotfiles/labwc/rc.xml";
    if (QFile::exists(path)) return path;
    path = "../../configs/dotfiles/labwc/rc.xml";
    if (QFile::exists(path)) return path;
    return {};
}

class AddEditDialog : public QDialog {
    Q_OBJECT
public:
    AddEditDialog(const KeybindInfo &info = {}, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle(info.keyCombo.isEmpty() ? "Add Keybinding" : "Edit Keybinding");
        setMinimumWidth(420);

        auto *form = new QFormLayout(this);
        form->setSpacing(12);

        keyEdit = new QLineEdit(info.keyCombo);
        keyEdit->setPlaceholderText("e.g. W-Return, A-F2, Print");
        form->addRow("Key Combination:", keyEdit);

        actionCombo = new QComboBox();
        actionCombo->setEditable(true);
        actionCombo->addItems(COMMON_ACTIONS);
        int idx = actionCombo->findText(info.action);
        if (idx >= 0) actionCombo->setCurrentIndex(idx);
        else if (!info.action.isEmpty()) actionCombo->setEditText(info.action);
        form->addRow("Action:", actionCombo);

        cmdEdit = new QLineEdit(info.command);
        cmdEdit->setPlaceholderText("e.g. qterminal, lxqt-runner");
        form->addRow("Command:", cmdEdit);

        directionEdit = new QLineEdit(info.direction);
        directionEdit->setPlaceholderText("e.g. left, right, up, down");
        form->addRow("Direction:", directionEdit);

        regionEdit = new QLineEdit(info.region);
        regionEdit->setPlaceholderText("e.g. left, right, top, bottom");
        form->addRow("Region:", regionEdit);

        toDesktopEdit = new QLineEdit(info.toDesktop);
        toDesktopEdit->setPlaceholderText("e.g. 1, 2, left, right");
        form->addRow("To Desktop:", toDesktopEdit);

        auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(btnBox);

        setStyleSheet(
            "QDialog { background-color: #1e1e2e; }"
            "QLabel { color: #cdd6f4; font-size: 13px; }"
            "QLineEdit, QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 4px; padding: 6px; font-size: 13px; }"
            "QLineEdit:focus, QComboBox:focus { border: 1px solid #89b4fa; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; selection-background-color: #313244; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 4px; padding: 6px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
    }

    KeybindInfo getResult() const {
        KeybindInfo info;
        info.keyCombo = keyEdit->text().trimmed();
        info.action = actionCombo->currentText().trimmed();
        info.command = cmdEdit->text().trimmed();
        info.direction = directionEdit->text().trimmed();
        info.region = regionEdit->text().trimmed();
        info.toDesktop = toDesktopEdit->text().trimmed();
        return info;
    }

private:
    QLineEdit *keyEdit;
    QComboBox *actionCombo;
    QLineEdit *cmdEdit;
    QLineEdit *directionEdit;
    QLineEdit *regionEdit;
    QLineEdit *toDesktopEdit;
};

class KeybindsGui : public QMainWindow {
    Q_OBJECT

public:
    KeybindsGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Global Keybindings Manager");
        resize(900, 650);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Global Keybindings (Labwc rc.xml)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        searchBar = new QLineEdit();
        searchBar->setPlaceholderText("Search by key, action, or command...");
        searchBar->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(searchBar, &QLineEdit::textChanged, this, &KeybindsGui::filterBinds);
        layout->addWidget(searchBar);

        table = new QTableWidget();
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels({"Key", "Action", "Command", "Details"});
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
        connect(table, &QTableWidget::doubleClicked, this, &KeybindsGui::editSelected);
        layout->addWidget(table);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(10);

        auto *addBtn = new QPushButton("+ Add");
        addBtn->setMinimumHeight(38);
        addBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(addBtn, &QPushButton::clicked, this, &KeybindsGui::addKeybind);
        btnLayout->addWidget(addBtn);

        auto *editBtn = new QPushButton("Edit");
        editBtn->setMinimumHeight(38);
        connect(editBtn, &QPushButton::clicked, this, &KeybindsGui::editSelected);
        btnLayout->addWidget(editBtn);

        auto *delBtn = new QPushButton("Delete");
        delBtn->setMinimumHeight(38);
        delBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }"
        );
        connect(delBtn, &QPushButton::clicked, this, &KeybindsGui::deleteSelected);
        btnLayout->addWidget(delBtn);

        auto *reloadBtn = new QPushButton("Reload");
        reloadBtn->setMinimumHeight(38);
        connect(reloadBtn, &QPushButton::clicked, this, &KeybindsGui::loadKeybinds);
        btnLayout->addWidget(reloadBtn);

        btnLayout->addStretch();

        auto *saveBtn = new QPushButton("Save to rc.xml");
        saveBtn->setMinimumHeight(38);
        saveBtn->setStyleSheet(
            "QPushButton { background-color: #f9e2af; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #f5d998; }"
        );
        connect(saveBtn, &QPushButton::clicked, this, &KeybindsGui::saveConfig);
        btnLayout->addWidget(saveBtn);

        auto *applyBtn = new QPushButton("Apply (labwc -r)");
        applyBtn->setMinimumHeight(38);
        applyBtn->setStyleSheet(
            "QPushButton { background-color: #cba6f7; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #d4b8fa; }"
        );
        connect(applyBtn, &QPushButton::clicked, this, &KeybindsGui::reconfigureLabwc);
        btnLayout->addWidget(applyBtn);

        layout->addLayout(btnLayout);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        auto *delShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
        connect(delShortcut, &QShortcut::activated, this, &KeybindsGui::deleteSelected);

        applyStyle();
        loadKeybinds();
    }

private slots:
    void loadKeybinds() {
        allBinds.clear();
        configPath = resolveConfigPath();
        if (configPath.isEmpty()) {
            statusBar()->showMessage("Could not find labwc rc.xml");
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
                    KeybindInfo info;
                    info.keyCombo = currentKey;
                    info.action = xml.attributes().value("name").toString();
                    info.command = xml.attributes().value("command").toString();
                    info.direction = xml.attributes().value("direction").toString();
                    info.region = xml.attributes().value("region").toString();
                    info.toDesktop = xml.attributes().value("to").toString();
                    allBinds.append(info);
                }
            } else if (token == QXmlStreamReader::EndElement) {
                if (xml.name().toString() == "keybind") currentKey.clear();
            }
        }

        populateTable();
        statusBar()->showMessage("Loaded " + QString::number(allBinds.size()) + " keybindings from " + configPath);
    }

    void populateTable() {
        QString filter = searchBar->text().toLower();
        table->setRowCount(0);

        for (int i = 0; i < allBinds.size(); i++) {
            const auto &b = allBinds[i];
            if (!filter.isEmpty() &&
                !b.keyCombo.toLower().contains(filter) &&
                !b.action.toLower().contains(filter) &&
                !b.command.toLower().contains(filter)) {
                continue;
            }

            int row = table->rowCount();
            table->insertRow(row);

            auto *keyItem = new QTableWidgetItem(b.keyCombo);
            keyItem->setForeground(QColor("#f9e2af"));
            keyItem->setData(Qt::UserRole, i);
            table->setItem(row, 0, keyItem);

            auto *actionItem = new QTableWidgetItem(b.action);
            table->setItem(row, 1, actionItem);

            auto *cmdItem = new QTableWidgetItem(b.command.isEmpty() ? "-" : b.command);
            cmdItem->setForeground(QColor("#a6adc8"));
            table->setItem(row, 2, cmdItem);

            QString details;
            if (!b.direction.isEmpty()) details += "dir=" + b.direction + " ";
            if (!b.region.isEmpty()) details += "region=" + b.region + " ";
            if (!b.toDesktop.isEmpty()) details += "to=" + b.toDesktop;
            auto *detailItem = new QTableWidgetItem(details.trimmed());
            detailItem->setForeground(QColor("#585b70"));
            table->setItem(row, 3, detailItem);
        }
    }

    void filterBinds(const QString &) { populateTable(); }

    void addKeybind() {
        AddEditDialog dlg({}, this);
        if (dlg.exec() != QDialog::Accepted) return;
        KeybindInfo info = dlg.getResult();
        if (info.keyCombo.isEmpty() || info.action.isEmpty()) {
            QMessageBox::warning(this, "Validation", "Key and Action are required.");
            return;
        }
        if (hasConflict(info.keyCombo)) {
            auto reply = QMessageBox::question(this, "Conflict",
                "Key '" + info.keyCombo + "' is already bound. Add anyway?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply != QMessageBox::Yes) return;
        }
        allBinds.append(info);
        populateTable();
        statusBar()->showMessage("Added " + info.keyCombo + " -> " + info.action + " (unsaved)");
    }

    void editSelected() {
        int row = table->currentRow();
        if (row < 0) return;
        int idx = table->item(row, 0)->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= allBinds.size()) return;

        AddEditDialog dlg(allBinds[idx], this);
        if (dlg.exec() != QDialog::Accepted) return;
        KeybindInfo info = dlg.getResult();
        if (info.keyCombo.isEmpty() || info.action.isEmpty()) {
            QMessageBox::warning(this, "Validation", "Key and Action are required.");
            return;
        }
        allBinds[idx] = info;
        populateTable();
        statusBar()->showMessage("Edited " + info.keyCombo + " (unsaved)");
    }

    void deleteSelected() {
        int row = table->currentRow();
        if (row < 0) return;
        int idx = table->item(row, 0)->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= allBinds.size()) return;

        auto reply = QMessageBox::question(this, "Confirm Delete",
            "Delete binding '" + allBinds[idx].keyCombo + "' -> " + allBinds[idx].action + "?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;

        allBinds.removeAt(idx);
        populateTable();
        statusBar()->showMessage("Deleted keybinding (unsaved)");
    }

    void saveConfig() {
        if (configPath.isEmpty()) {
            QMessageBox::critical(this, "Error", "No config path resolved.");
            return;
        }

        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot read: " + configPath);
            return;
        }

        QString content = file.readAll();
        file.close();

        int kbStart = content.indexOf("<keyboard>");
        int kbEnd = content.indexOf("</keyboard>");
        if (kbStart < 0 || kbEnd < 0) {
            QMessageBox::critical(this, "Error", "Could not find <keyboard> section in rc.xml");
            return;
        }
        kbEnd += QString("</keyboard>").length();

        QString newKb = "<keyboard>\n    <default />\n";
        for (const auto &b : allBinds) {
            newKb += "    <keybind key=\"" + b.keyCombo + "\">\n";
            newKb += "      <action name=\"" + b.action + "\"";
            if (!b.command.isEmpty()) newKb += " command=\"" + b.command + "\"";
            if (!b.direction.isEmpty()) newKb += " direction=\"" + b.direction + "\"";
            if (!b.region.isEmpty()) newKb += " region=\"" + b.region + "\"";
            if (!b.toDesktop.isEmpty()) newKb += " to=\"" + b.toDesktop + "\"";
            newKb += " />\n";
            newKb += "    </keybind>\n";
        }
        newKb += "  </keyboard>";

        content.replace(kbStart, kbEnd - kbStart, newKb);

        QFile out(configPath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot write: " + configPath);
            return;
        }
        out.write(content.toUtf8());
        out.close();

        QMessageBox::information(this, "Saved", "Keybindings saved to:\n" + configPath);
        statusBar()->showMessage("Saved " + QString::number(allBinds.size()) + " keybindings");
    }

    void reconfigureLabwc() {
        QProcess::startDetached("labwc", {"-r"});
        QMessageBox::information(this, "Applied", "Labwc reconfigured. Changes are live.");
    }

private:
    bool hasConflict(const QString &key) {
        int count = 0;
        for (const auto &b : allBinds)
            if (b.keyCombo == key) count++;
        return count > 0;
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QTableWidget *table;
    QLineEdit *searchBar;
    QList<KeybindInfo> allBinds;
    QString configPath;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--reconfigure") {
            QProcess::startDetached("labwc", {"-r"});
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            puts("nv-keybinds-gui — Labwc Keybinding Manager\n\n"
                 "Usage:\n"
                 "  nv-keybinds-gui           Launch GUI\n"
                 "  nv-keybinds-gui --reconfigure  Send labwc -r and exit\n"
                 "  nv-keybinds-gui --help     Show this help");
            return 0;
        }
    }

    KeybindsGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
