#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QProcess>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QTimer>
#include <QTime>

class NightlightGui : public QMainWindow {
    Q_OBJECT

public:
    NightlightGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Night Light");
        resize(480, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Reduce blue light for comfortable nighttime use");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        // Enable toggle
        auto *enableGroup = new QGroupBox("Night Light");
        enableGroup->setStyleSheet(groupBoxStyle());
        auto *enableLayout = new QVBoxLayout(enableGroup);

        enableCheck = new QCheckBox("Enable night light");
        enableCheck->setMinimumHeight(30);
        connect(enableCheck, &QCheckBox::toggled, this, &NightlightGui::onEnableToggled);
        enableLayout->addWidget(enableCheck);

        statusLabel = new QLabel("Status: Inactive");
        statusLabel->setStyleSheet("color: #a6adc8; font-size: 13px; margin-top: 5px;");
        enableLayout->addWidget(statusLabel);

        mainLayout->addWidget(enableGroup);

        // Temperature
        auto *tempGroup = new QGroupBox("Color Temperature");
        tempGroup->setStyleSheet(groupBoxStyle());
        auto *tempLayout = new QVBoxLayout(tempGroup);
        tempLayout->setSpacing(10);

        auto *nightTempRow = new QHBoxLayout();
        nightTempRow->addWidget(new QLabel("Night:"));
        nightTempSpin = new QSpinBox();
        nightTempSpin->setRange(1000, 5000);
        nightTempSpin->setValue(3500);
        nightTempSpin->setSuffix(" K");
        nightTempSpin->setSingleStep(100);
        nightTempSpin->setMinimumHeight(32);
        nightTempSpin->setStyleSheet(spinStyle());
        connect(nightTempSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NightlightGui::updateSliderFromSpin);
        nightTempRow->addWidget(nightTempSpin);

        auto *dayTempRow = new QHBoxLayout();
        dayTempRow->addWidget(new QLabel("Day:"));
        dayTempSpin = new QSpinBox();
        dayTempSpin->setRange(3500, 7500);
        dayTempSpin->setValue(6500);
        dayTempSpin->setSuffix(" K");
        dayTempSpin->setSingleStep(100);
        dayTempSpin->setMinimumHeight(32);
        dayTempSpin->setStyleSheet(spinStyle());
        dayTempRow->addWidget(dayTempSpin);

        tempLayout->addLayout(nightTempRow);
        tempLayout->addLayout(dayTempRow);

        tempSlider = new QSlider(Qt::Horizontal);
        tempSlider->setRange(1000, 7500);
        tempSlider->setValue(3500);
        tempSlider->setTickPosition(QSlider::TicksBelow);
        tempSlider->setTickInterval(500);
        tempSlider->setMinimumHeight(30);
        connect(tempSlider, &QSlider::valueChanged, this, &NightlightGui::onSliderChanged);

        auto *sliderLabels = new QHBoxLayout();
        sliderLabels->addWidget(new QLabel("Warm (1000K)"));
        sliderLabels->addStretch();
        sliderLabels->addWidget(new QLabel("Cool (7500K)"));

        tempLayout->addWidget(tempSlider);
        tempLayout->addLayout(sliderLabels);

        mainLayout->addWidget(tempGroup);

        // Schedule
        auto *scheduleGroup = new QGroupBox("Schedule");
        scheduleGroup->setStyleSheet(groupBoxStyle());
        auto *scheduleForm = new QFormLayout(scheduleGroup);
        scheduleForm->setSpacing(8);
        scheduleForm->setContentsMargins(10, 20, 10, 10);

        auto *startRow = new QHBoxLayout();
        startHourSpin = new QSpinBox();
        startHourSpin->setRange(0, 23);
        startHourSpin->setValue(20);
        startHourSpin->setSuffix(":00");
        startHourSpin->setMinimumHeight(32);
        startHourSpin->setStyleSheet(spinStyle());
        startRow->addWidget(startHourSpin);
        startRow->addStretch();
        scheduleForm->addRow("Starts at:", startRow);

        auto *endRow = new QHBoxLayout();
        endHourSpin = new QSpinBox();
        endHourSpin->setRange(0, 23);
        endHourSpin->setValue(7);
        endHourSpin->setSuffix(":00");
        endHourSpin->setMinimumHeight(32);
        endHourSpin->setStyleSheet(spinStyle());
        endRow->addWidget(endHourSpin);
        endRow->addStretch();
        scheduleForm->addRow("Ends at:", endRow);

        mainLayout->addWidget(scheduleGroup);

        // Apply button
        auto *applyBtn = new QPushButton("Apply Settings");
        applyBtn->setMinimumHeight(40);
        applyBtn->setStyleSheet(accentBtnStyle());
        connect(applyBtn, &QPushButton::clicked, this, &NightlightGui::applySettings);
        mainLayout->addWidget(applyBtn);

        mainLayout->addStretch();

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;"
        );

        applyStyle();
        loadSettings();
    }

private slots:
    void onSliderChanged(int value) {
        nightTempSpin->setValue(value);
    }

    void updateSliderFromSpin(int value) {
        tempSlider->setValue(value);
    }

    void onEnableToggled(bool checked) {
        if (checked) {
            statusLabel->setText("Status: Active");
            statusLabel->setStyleSheet("color: #a6e3a1; font-size: 13px; margin-top: 5px;");
        } else {
            statusLabel->setText("Status: Inactive");
            statusLabel->setStyleSheet("color: #f38ba8; font-size: 13px; margin-top: 5px;");
        }
    }

    void applySettings() {
        int nightTemp = nightTempSpin->value();
        int dayTemp = dayTempSpin->value();
        bool enabled = enableCheck->isChecked();
        int startHour = startHourSpin->value();
        int endHour = endHourSpin->value();

        // Save to labwc environment
        QString envPath = QDir::homePath() + "/.config/labwc/environment";
        QFile envFile(envPath);
        QString envContent;
        if (envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            envContent = QString(envFile.readAll());
            envFile.close();
        }

        // Remove old wlsunset entries
        envContent.replace(QRegularExpression("\n# Night light.*\n(wlsunset.*)"), "");

        if (enabled) {
            QString lat = "51.5"; // Default, user should customize
            QString lon = "-0.12";
            QString wlsunsetCmd = QString("wlsunset -l %1 -L %2 -t %3 -T %4 &")
                .arg(lat, lon).arg(nightTemp).arg(dayTemp);
            envContent += "\n# Night light\n" + wlsunsetCmd + "\n";
        }

        if (envFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            envFile.write(envContent.toUtf8());
            envFile.close();
        }

        // Also create/update wlsunset systemd user service
        QString serviceDir = QDir::homePath() + "/.config/systemd/user";
        QDir().mkpath(serviceDir);
        QString serviceFile = serviceDir + "/wlsunset.service";

        if (enabled) {
            QString service = QString(
                "[Unit]\n"
                "Description=Night Light (wlsunset)\n"
                "After=graphical-session.target\n\n"
                "[Service]\n"
                "ExecStart=/usr/bin/wlsunset -l 51.5 -L -0.12 -t %1 -T %2\n"
                "Restart=on-failure\n\n"
                "[Install]\n"
                "WantedBy=default.target\n"
            ).arg(nightTemp).arg(dayTemp);

            QFile svc(serviceFile);
            if (svc.open(QIODevice::WriteOnly | QIODevice::Text)) {
                svc.write(service.toUtf8());
                svc.close();
            }

            QProcess::execute("systemctl", {"--user", "daemon-reload"});
            QProcess::execute("systemctl", {"--user", "enable", "wlsunset"});
            QProcess::execute("systemctl", {"--user", "start", "wlsunset"});
        } else {
            QProcess::execute("systemctl", {"--user", "stop", "wlsunset"});
            QProcess::execute("systemctl", {"--user", "disable", "wlsunset"});
            QFile::remove(serviceFile);
        }

        statusBar()->showMessage(enabled ?
            QString("Night light enabled: %1K night, %2K day").arg(nightTemp).arg(dayTemp) :
            "Night light disabled");
    }

    void loadSettings() {
        QString serviceFile = QDir::homePath() + "/.config/systemd/user/wlsunset.service";
        if (QFile::exists(serviceFile)) {
            enableCheck->setChecked(true);

            QFile f(serviceFile);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString(f.readAll());
                f.close();

                QRegularExpression reTemp("-t (\\d+) -T (\\d+)");
                QRegularExpressionMatch m = reTemp.match(content);
                if (m.hasMatch()) {
                    nightTempSpin->setValue(m.captured(1).toInt());
                    dayTempSpin->setValue(m.captured(2).toInt());
                    tempSlider->setValue(m.captured(1).toInt());
                }
            }
        }
    }

private:
    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString spinStyle() {
        return "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
               "border-radius: 6px; padding: 6px; }";
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
            "QCheckBox { spacing: 8px; }"
            "QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid #313244; "
            "border-radius: 4px; background-color: #181825; }"
            "QCheckBox::indicator:checked { background-color: #a6e3a1; border-color: #a6e3a1; }"
            "QSlider::groove:horizontal { height: 6px; background: #313244; border-radius: 3px; }"
            "QSlider::handle:horizontal { width: 18px; height: 18px; margin: -6px 0; "
            "background: #89b4fa; border-radius: 9px; }"
            "QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 3px; }"
        );
    }

    QCheckBox *enableCheck;
    QLabel *statusLabel;
    QSpinBox *nightTempSpin;
    QSpinBox *dayTempSpin;
    QSlider *tempSlider;
    QSpinBox *startHourSpin;
    QSpinBox *endHourSpin;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    NightlightGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
