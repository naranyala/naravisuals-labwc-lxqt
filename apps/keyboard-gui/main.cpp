#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QProcess>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>

class KeyboardGui : public QMainWindow {
    Q_OBJECT

public:
    KeyboardGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Keyboard Settings");
        resize(520, 560);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Manage keyboard layouts, repeat rate, and compose key");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        // Active layouts
        auto *layoutGroup = new QGroupBox("Keyboard Layouts");
        layoutGroup->setStyleSheet(groupBoxStyle());
        auto *layoutLayout = new QVBoxLayout(layoutGroup);

        layoutList = new QListWidget();
        layoutList->setMinimumHeight(100);
        layoutList->setStyleSheet(
            "QListWidget { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; }"
            "QListWidget::item { padding: 6px; }"
            "QListWidget::item:selected { background-color: #313244; color: #89b4fa; }"
        );
        layoutLayout->addWidget(layoutList);

        auto *layoutBtnRow = new QHBoxLayout();
        layoutBtnRow->setSpacing(8);

        addLayoutCombo = new QComboBox();
        addLayoutCombo->setMinimumHeight(32);
        populateAvailableLayouts();
        layoutBtnRow->addWidget(addLayoutCombo, 1);

        auto *addBtn = new QPushButton("+ Add");
        addBtn->setMinimumHeight(32);
        addBtn->setStyleSheet(miniBtnStyle("#a6e3a1"));
        connect(addBtn, &QPushButton::clicked, this, &KeyboardGui::addLayout);
        layoutBtnRow->addWidget(addBtn);

        auto *removeBtn = new QPushButton("- Remove");
        removeBtn->setMinimumHeight(32);
        removeBtn->setStyleSheet(miniBtnStyle("#f38ba8"));
        connect(removeBtn, &QPushButton::clicked, this, &KeyboardGui::removeLayout);
        layoutBtnRow->addWidget(removeBtn);

        layoutLayout->addLayout(layoutBtnRow);
        mainLayout->addWidget(layoutGroup);

        // Repeat rate
        auto *repeatGroup = new QGroupBox("Key Repeat");
        repeatGroup->setStyleSheet(groupBoxStyle());
        auto *repeatForm = new QFormLayout(repeatGroup);
        repeatForm->setSpacing(8);
        repeatForm->setContentsMargins(10, 20, 10, 10);

        rateSpin = new QSpinBox();
        rateSpin->setRange(20, 1000);
        rateSpin->setValue(300);
        rateSpin->setSuffix(" ms");
        rateSpin->setMinimumHeight(32);
        rateSpin->setStyleSheet(spinStyle());
        repeatForm->addRow("Repeat delay:", rateSpin);

        delaySpin = new QSpinBox();
        delaySpin->setRange(100, 2000);
        delaySpin->setValue(500);
        delaySpin->setSuffix(" ms");
        delaySpin->setMinimumHeight(32);
        delaySpin->setStyleSheet(spinStyle());
        repeatForm->addRow("Repeat interval:", delaySpin);

        mainLayout->addWidget(repeatGroup);

        // Compose key
        auto *composeGroup = new QGroupBox("Compose Key");
        composeGroup->setStyleSheet(groupBoxStyle());
        auto *composeLayout = new QVBoxLayout(composeGroup);

        composeCheck = new QCheckBox("Enable compose key");
        composeCheck->setMinimumHeight(30);
        connect(composeCheck, &QCheckBox::toggled, this, &KeyboardGui::onComposeToggled);
        composeLayout->addWidget(composeCheck);

        auto *composeRow = new QHBoxLayout();
        composeRow->addWidget(new QLabel("Key:"));
        composeCombo = new QComboBox();
        composeCombo->addItems({"Right Ctrl", "Left Ctrl", "Caps Lock", "Menu", "Right Alt"});
        composeCombo->setMinimumHeight(32);
        composeCombo->setEnabled(false);
        composeRow->addWidget(composeCombo);
        composeRow->addStretch();
        composeLayout->addLayout(composeRow);

        mainLayout->addWidget(composeGroup);

        // Apply
        auto *applyBtn = new QPushButton("Apply Keyboard Settings");
        applyBtn->setMinimumHeight(40);
        applyBtn->setStyleSheet(accentBtnStyle());
        connect(applyBtn, &QPushButton::clicked, this, &KeyboardGui::applySettings);
        mainLayout->addWidget(applyBtn);

        mainLayout->addStretch();

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;"
        );
        statusBar()->showMessage("Ready");

        applyStyle();
        loadCurrentLayouts();
    }

private slots:
    void addLayout() {
        QString layout = addLayoutCombo->currentText();
        if (layout.isEmpty()) return;

        // Check if already added
        for (int i = 0; i < layoutList->count(); i++) {
            if (layoutList->item(i)->text() == layout) {
                QMessageBox::information(this, "Already Added", layout + " is already in the list.");
                return;
            }
        }

        layoutList->addItem(layout);
    }

    void removeLayout() {
        auto *item = layoutList->currentItem();
        if (item) {
            delete layoutList->takeItem(layoutList->row(item));
        }
    }

    void onComposeToggled(bool checked) {
        composeCombo->setEnabled(checked);
    }

    void applySettings() {
        // Collect layouts
        QStringList layouts;
        for (int i = 0; i < layoutList->count(); i++) {
            layouts << layoutList->item(i)->text();
        }

        if (layouts.isEmpty()) {
            QMessageBox::warning(this, "No Layouts", "Add at least one keyboard layout.");
            return;
        }

        // Set repeat rate via xset
        int rate = rateSpin->value();
        int delay = delaySpin->value();
        QProcess::execute("xset", {"r", "rate", QString::number(delay), QString::number(rate)});

        // Set keyboard layout via setxkbmap
        QString layoutStr = layouts.join(',');
        QProcess::execute("setxkbmap", {layoutStr});

        // Set compose key
        if (composeCheck->isChecked()) {
            QString key = composeCombo->currentText().toLower().replace(' ', '_');
            QProcess::execute("setxkbmap", {"-option", "compose:" + key});
        }

        // Save to labwc environment for persistence
        QString envPath = QDir::homePath() + "/.config/labwc/environment";
        QFile envFile(envPath);
        QString envContent;
        if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            envContent = QString(envFile.readAll());
            envFile.close();
        }

        envContent.replace(QRegularExpression("\n# Keyboard layouts.*\n(XKB_DEFAULT_LAYOUT.*)"), "");

        QString kbEnv = QString("\n# Keyboard layouts\nXKB_DEFAULT_LAYOUT=%1\n").arg(layoutStr);
        envContent += kbEnv;

        if (envFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            envFile.write(envContent.toUtf8());
            envFile.close();
        }

        statusBar()->showMessage("Keyboard settings applied: " + layoutStr);
    }

private:
    void populateAvailableLayouts() {
        QProcess proc;
        proc.start("setxkbmap", {"-query"});
        proc.waitForFinished(3000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        // Get available layouts from system
        QProcess listProc;
        listProc.start("localectl", {"list-xkb-models"});
        listProc.waitForFinished(3000);

        // Common layouts
        QStringList common = {"us", "gb", "de", "fr", "es", "it", "pt", "br", "ru", "jp", "kr",
                              "cn", "in", "se", "no", "dk", "fi", "nl", "be", "ch", "at", "pl",
                              "cz", "ro", "hu", "hr", "bg", "gr", "tr", "il", "ar", "th"};

        addLayoutCombo->clear();
        addLayoutCombo->addItems(common);
    }

    void loadCurrentLayouts() {
        QProcess proc;
        proc.start("setxkbmap", {"-query"});
        proc.waitForFinished(3000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        QRegularExpression re("layout:\\s+(.+)");
        QRegularExpressionMatch m = re.match(output);
        if (m.hasMatch()) {
            QStringList layouts = m.captured(1).trimmed().split(',', Qt::SkipEmptyParts);
            for (const auto &l : layouts) {
                layoutList->addItem(l.trimmed());
            }
        }

        // Load repeat rate
        QProcess repeat;
        repeat.start("xset", {"q"});
        repeat.waitForFinished(3000);
        QString repeatOut = QString::fromUtf8(repeat.readAllStandardOutput());

        QRegularExpression reRate("auto repeat delay:\\s+(\\d+)\\s+repeat rate:\\s+(\\d+)");
        QRegularExpressionMatch mRate = reRate.match(repeatOut);
        if (mRate.hasMatch()) {
            delaySpin->setValue(mRate.captured(1).toInt());
            rateSpin->setValue(mRate.captured(2).toInt());
        }
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString spinStyle() {
        return "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
               "border-radius: 6px; padding: 6px; }";
    }

    QString miniBtnStyle(const QString &color) {
        return QString("QPushButton { background-color: %1; color: #1e1e2e; border-radius: 4px; "
                       "padding: 6px 12px; font-weight: bold; font-size: 12px; border: none; }"
                       "QPushButton:hover { background-color: %1; opacity: 0.8; }").arg(color);
    }

    QString accentBtnStyle() {
        return "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
               "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
               "QPushButton:hover { background-color: #b4f9b8; }";
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 6px 10px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
            "border-right: 5px solid transparent; border-top: 6px solid #a6adc8; margin-right: 8px; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; "
            "selection-background-color: #313244; border: 1px solid #45475a; }"
            "QCheckBox { spacing: 8px; }"
            "QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid #313244; "
            "border-radius: 4px; background-color: #181825; }"
            "QCheckBox::indicator:checked { background-color: #a6e3a1; border-color: #a6e3a1; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );
    }

    QListWidget *layoutList;
    QComboBox *addLayoutCombo;
    QSpinBox *rateSpin;
    QSpinBox *delaySpin;
    QCheckBox *composeCheck;
    QComboBox *composeCombo;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    KeyboardGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
