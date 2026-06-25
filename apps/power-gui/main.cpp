#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QProcess>
#include <QStatusBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRegularExpression>

class PowerGui : public QMainWindow {
    Q_OBJECT

public:
    PowerGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Power Management");
        resize(500, 420);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(16);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Configure idle timeout, screen lock, and lid actions");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Idle Section ---
        auto *idleGroup = new QGroupBox("Idle Behavior (swayidle)");
        idleGroup->setStyleSheet(groupStyle());
        auto *idleLayout = new QVBoxLayout(idleGroup);

        auto *timeoutRow = new QHBoxLayout();
        auto *timeoutLabel = new QLabel("Screen blank after (seconds):");
        timeoutLabel->setStyleSheet("color: #a6adc8;");
        timeoutRow->addWidget(timeoutLabel);
        timeoutSpin = new QSpinBox();
        timeoutSpin->setRange(0, 3600);
        timeoutSpin->setValue(300);
        timeoutSpin->setSuffix("s");
        timeoutRow->addWidget(timeoutSpin);
        idleLayout->addLayout(timeoutRow);

        auto *lockRow = new QHBoxLayout();
        auto *lockLabel = new QLabel("Lock screen after (seconds):");
        lockLabel->setStyleSheet("color: #a6adc8;");
        lockRow->addWidget(lockLabel);
        lockSpin = new QSpinBox();
        lockSpin->setRange(0, 3600);
        lockSpin->setValue(600);
        lockSpin->setSuffix("s");
        lockRow->addWidget(lockSpin);
        idleLayout->addLayout(lockRow);

        layout->addWidget(idleGroup);

        // --- Lid Section ---
        auto *lidGroup = new QGroupBox("Lid Close Action");
        lidGroup->setStyleSheet(groupStyle());
        auto *lidLayout = new QVBoxLayout(lidGroup);

        lidCombo = new QComboBox();
        lidCombo->addItems({"Do nothing", "Suspend", "Hibernate", "Lock screen", "Power off"});
        lidCombo->setMinimumHeight(36);
        lidLayout->addWidget(lidCombo);

        layout->addWidget(lidGroup);

    // --- Power Profiles ---
    auto *profileGroup = new QGroupBox("Power Profile (power-profiles-daemon)");
    profileGroup->setStyleSheet(groupStyle());
    auto *profileLayout = new QVBoxLayout(profileGroup);

    auto *profileRow = new QHBoxLayout();
    auto *profileLabel = new QLabel("Profile:");
    profileLabel->setStyleSheet("color: #a6adc8;");
    profileRow->addWidget(profileLabel);
    profileCombo = new QComboBox();
    profileCombo->setMinimumHeight(36);
    profileRow->addWidget(profileCombo, 1);
    profileLayout->addLayout(profileRow);

    auto *profileBtnRow = new QHBoxLayout();
    auto *refreshProfilesBtn = new QPushButton("Detect Profiles");
    refreshProfilesBtn->setMinimumHeight(34);
    connect(refreshProfilesBtn, &QPushButton::clicked, this, &PowerGui::loadProfiles);
    profileBtnRow->addWidget(refreshProfilesBtn);

    auto *applyProfileBtn = new QPushButton("Apply Profile");
    applyProfileBtn->setMinimumHeight(34);
    connect(applyProfileBtn, &QPushButton::clicked, this, &PowerGui::applyProfile);
    profileBtnRow->addWidget(applyProfileBtn);

    profileLayout->addLayout(profileBtnRow);
    layout->addWidget(profileGroup);

    // --- Buttons ---
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto *loadBtn = new QPushButton("Load Current");
    loadBtn->setMinimumHeight(38);
    connect(loadBtn, &QPushButton::clicked, this, &PowerGui::loadConfig);
    btnRow->addWidget(loadBtn);

    auto *applyBtn = new QPushButton("Apply");
    applyBtn->setMinimumHeight(38);
    applyBtn->setStyleSheet(
        "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
        "QPushButton:hover { background-color: #b4f9b8; }"
    );
    connect(applyBtn, &QPushButton::clicked, this, &PowerGui::applyConfig);
    btnRow->addWidget(applyBtn);

    layout->addLayout(btnRow);
    layout->addStretch();

    statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
    applyStyle();
    loadConfig();
    loadProfiles();
    }

private slots:
    void loadProfiles() {
        profileCombo->clear();
        QProcess p;
        p.start("powerprofilesctl", {"list"});
        p.waitForFinished(3000);
        QString out = p.readAllStandardOutput();
        // Parse lines like "performance:*" or "  balanced:  "
        QRegularExpression re(R"(^\s*(\S+):)", QRegularExpression::MultilineOption);
        auto it = re.globalMatch(out);
        while (it.hasNext()) {
            auto m = it.next();
            QString name = m.captured(1);
            bool active = out.contains(name + ":*");
            profileCombo->addItem(name, active);
        }
        if (profileCombo->count() == 0) {
            profileCombo->addItem("power-profiles-daemon not available");
        }
    }

    void applyProfile() {
        QString profile = profileCombo->currentText();
        if (profile.isEmpty() || profile.contains("not available")) return;
        QProcess p;
        p.start("powerprofilesctl", {"set", profile});
        p.waitForFinished(3000);
        if (p.exitCode() == 0) {
            statusBar()->showMessage("Power profile set to: " + profile);
        } else {
            statusBar()->showMessage("Failed to set profile: " + p.readAllStandardError().trimmed());
        }
        loadProfiles();
    }

    void loadConfig() {
        QString configPath = QDir::homePath() + "/.config/labwc/rc.xml";
        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            statusBar()->showMessage("Cannot read " + configPath);
            return;
        }

        // Parse swayidle config if it exists
        QString swayIdlePath = QDir::homePath() + "/.config/swayidle/config";
        QFile swayFile(swayIdlePath);
        if (swayFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = swayFile.readAll();
            // Extract timeout values
            QRegularExpression timeoutRe("timeout\\s+(\\d+)\\s+.*");
            QRegularExpressionMatch match = timeoutRe.match(content);
            if (match.hasMatch()) timeoutSpin->setValue(match.captured(1).toInt());

            QRegularExpression lockRe("timeout\\s+(\\d+)\\s+.*swaylock");
            match = lockRe.match(content);
            if (match.hasMatch()) lockSpin->setValue(match.captured(1).toInt());
        }

        statusBar()->showMessage("Config loaded");
    }

    void applyConfig() {
        QString configDir = QDir::homePath() + "/.config/swayidle";
        QDir().mkpath(configDir);
        QString configPath = configDir + "/config";

        QFile file(configPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            statusBar()->showMessage("Cannot write " + configPath);
            return;
        }

        QTextStream out(&file);
        out << "# Generated by nv-power-gui\n";
        out << "timeout " << timeoutSpin->value() << " 'wlopm off'\n";
        out << "timeout " << lockSpin->value() << " 'swaylock -f'\n";
        out << "before-sleep 'swaylock -f'\n";
        out << "idleinhibit fullscreen\n";
        file.close();

        statusBar()->showMessage("Config written to " + configPath);
    }

private:
    QString groupStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 6px 10px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; border-right: 5px solid transparent; border-top: 6px solid #a6adc8; margin-right: 8px; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; selection-background-color: #313244; border: 1px solid #45475a; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
            "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 6px; }"
        );
    }

    QSpinBox *timeoutSpin;
    QSpinBox *lockSpin;
    QComboBox *lidCombo;
    QComboBox *profileCombo;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    int blankTimeout = 300;
    int lockTimeout = 600;
    bool doShow = false;
    bool doApply = false;
    bool hasBlank = false;
    bool hasLock = false;

    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--show") {
            doShow = true;
        } else if (arg == "--blank" && i + 1 < argc) {
            blankTimeout = QString(argv[++i]).toInt();
            hasBlank = true;
        } else if (arg == "--lock" && i + 1 < argc) {
            lockTimeout = QString(argv[++i]).toInt();
            hasLock = true;
        } else if (arg == "--apply") {
            doApply = true;
        }
    }

    if (doShow) {
        QString configPath = QDir::homePath() + "/.config/swayidle/config";
        QFile file(configPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            printf("%s", file.readAll().constData());
        } else {
            fprintf(stderr, "No swayidle config found at %s\n",
                    configPath.toLocal8Bit().constData());
        }
        return 0;
    }

    if (doApply) {
        QString configPath = QDir::homePath() + "/.config/swayidle/config";
        QFile readFile(configPath);
        if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = readFile.readAll();
            readFile.close();
            if (!hasBlank) {
                QRegularExpression re("timeout\\s+(\\d+)\\s+.*wlopm");
                QRegularExpressionMatch m = re.match(content);
                if (m.hasMatch()) blankTimeout = m.captured(1).toInt();
            }
            if (!hasLock) {
                QRegularExpression re("timeout\\s+(\\d+)\\s+.*swaylock");
                QRegularExpressionMatch m = re.match(content);
                if (m.hasMatch()) lockTimeout = m.captured(1).toInt();
            }
        }

        QString configDir = QDir::homePath() + "/.config/swayidle";
        QDir().mkpath(configDir);

        QFile file(configPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            fprintf(stderr, "Cannot write %s\n",
                    configPath.toLocal8Bit().constData());
            return 1;
        }

        QTextStream out(&file);
        out << "# Generated by nv-power-gui\n";
        out << "timeout " << blankTimeout << " 'wlopm off'\n";
        out << "timeout " << lockTimeout << " 'swaylock -f'\n";
        out << "before-sleep 'swaylock -f'\n";
        out << "idleinhibit fullscreen\n";
        file.close();

        printf("Config written to %s\n", configPath.toLocal8Bit().constData());
        return 0;
    }

    PowerGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
