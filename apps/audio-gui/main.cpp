#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QGroupBox>
#include <QProcess>
#include <QStatusBar>
#include <QCheckBox>
#include <QScrollArea>
#include <QFrame>
#include <QTimer>
#include <QRegularExpression>
#include <QTabWidget>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

struct AudioDevice {
    QString id;
    QString name;
    bool isDefault;
    int volume;
    bool muted;
};

class AudioDeviceWidget : public QFrame {
    Q_OBJECT
public:
    AudioDeviceWidget(const AudioDevice &dev, bool isInput, const QString &backendCmd, QWidget *parent = nullptr)
        : QFrame(parent), m_dev(dev), m_isInput(isInput), m_backendCmd(backendCmd) {
        
        setObjectName("AudioDeviceWidget");
        setStyleSheet("#AudioDeviceWidget { border: 1px solid #45475a; border-radius: 8px; background-color: #1e1e2e; margin-bottom: 6px; }");
        
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(14, 14, 14, 14);
        layout->setSpacing(10);

        // Top row: Name and Default toggle
        auto *topRow = new QHBoxLayout();
        auto *nameLabel = new QLabel(dev.name);
        nameLabel->setWordWrap(true);
        nameLabel->setStyleSheet("color: #cdd6f4; font-weight: bold; font-size: 14px;");
        topRow->addWidget(nameLabel, 1);

        defaultBtn = new QPushButton(dev.isDefault ? "★ Default" : "Set Default");
        defaultBtn->setCheckable(true);
        defaultBtn->setChecked(dev.isDefault);
        if (dev.isDefault) {
            defaultBtn->setStyleSheet("QPushButton { background-color: #a6e3a1; color: #1e1e2e; border: none; padding: 6px 12px; border-radius: 6px; font-weight: bold; }");
            defaultBtn->setEnabled(false); // Already default
        } else {
            defaultBtn->setStyleSheet("QPushButton { background-color: #313244; color: #cdd6f4; border: none; padding: 6px 12px; border-radius: 6px; } QPushButton:hover { background-color: #45475a; }");
        }
        connect(defaultBtn, &QPushButton::clicked, this, &AudioDeviceWidget::onDefaultClicked);
        topRow->addWidget(defaultBtn);
        layout->addLayout(topRow);

        // Bottom row: Mute, Slider, Value Label
        auto *botRow = new QHBoxLayout();
        muteBox = new QCheckBox("Mute");
        muteBox->setChecked(dev.muted);
        muteBox->setStyleSheet("QCheckBox { color: #a6adc8; } QCheckBox::indicator { width: 18px; height: 18px; }");
        connect(muteBox, &QCheckBox::toggled, this, &AudioDeviceWidget::onMuteToggled);
        botRow->addWidget(muteBox);

        volumeSlider = new QSlider(Qt::Horizontal);
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(dev.volume);
        volumeSlider->setStyleSheet(
            "QSlider::groove:horizontal { height: 6px; background: #313244; border-radius: 3px; }"
            "QSlider::handle:horizontal { background: #89b4fa; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px; }"
            "QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 3px; }"
        );
        connect(volumeSlider, &QSlider::valueChanged, this, &AudioDeviceWidget::onVolumeChanged);
        botRow->addWidget(volumeSlider, 1);

        volLabel = new QLabel(QString::number(dev.volume) + "%");
        volLabel->setFixedWidth(45);
        volLabel->setStyleSheet("color: #cdd6f4;");
        botRow->addWidget(volLabel);
        
        layout->addLayout(botRow);
    }

signals:
    void requestRefresh();

private slots:
    void onDefaultClicked() {
        if (m_isInput) {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-default", m_dev.id});
            else QProcess::startDetached("pactl", {"set-default-source", m_dev.id});
        } else {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-default", m_dev.id});
            else QProcess::startDetached("pactl", {"set-default-sink", m_dev.id});
        }
        QTimer::singleShot(500, this, [this](){ emit requestRefresh(); });
    }

    void onMuteToggled(bool muted) {
        if (m_isInput) {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-mute", m_dev.id, muted ? "1" : "0"});
            else QProcess::startDetached("pactl", {"set-source-mute", m_dev.id, muted ? "1" : "0"});
        } else {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-mute", m_dev.id, muted ? "1" : "0"});
            else QProcess::startDetached("pactl", {"set-sink-mute", m_dev.id, muted ? "1" : "0"});
        }
    }

    void onVolumeChanged(int vol) {
        volLabel->setText(QString::number(vol) + "%");
        double v = vol / 100.0;
        if (m_isInput) {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-volume", m_dev.id, QString::number(v, 'f', 2)});
            else QProcess::startDetached("pactl", {"set-source-volume", m_dev.id, QString("%1%").arg(vol)});
        } else {
            if (m_backendCmd == "wpctl") QProcess::startDetached("wpctl", {"set-volume", m_dev.id, QString::number(v, 'f', 2)});
            else QProcess::startDetached("pactl", {"set-sink-volume", m_dev.id, QString("%1%").arg(vol)});
        }
    }

private:
    AudioDevice m_dev;
    bool m_isInput;
    QString m_backendCmd;

    QPushButton *defaultBtn;
    QCheckBox *muteBox;
    QSlider *volumeSlider;
    QLabel *volLabel;
};

class AudioGui : public QMainWindow {
    Q_OBJECT

public:
    AudioGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Audio Manager");
        resize(600, 700);

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

        // --- Audio Tab ---
        auto *audioTab = new QWidget();
        auto *mainLayout = new QVBoxLayout(audioTab);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        detectBackend();

        // --- Devices Scroll Area ---
        auto *scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; } QWidget#scrollContent { background-color: transparent; }");
        
        auto *scrollContent = new QWidget(scrollArea);
        scrollContent->setObjectName("scrollContent");
        
        scrollLayout = new QVBoxLayout(scrollContent);
        scrollLayout->setSpacing(8);
        scrollLayout->setContentsMargins(0, 0, 10, 0); // Leave some margin for scrollbar
        
        scrollArea->setWidget(scrollContent);
        mainLayout->addWidget(scrollArea, 1);

        // --- Driver Info Section ---
        auto *driverGroup = new QGroupBox("Driver Info");
        driverGroup->setStyleSheet(
            "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
        );
        auto *driverLayout = new QVBoxLayout(driverGroup);

        auto *restartBtn = new QPushButton("Restart Audio Service");
        restartBtn->setMinimumHeight(38);
        connect(restartBtn, &QPushButton::clicked, this, &AudioGui::restartAudioService);
        driverLayout->addWidget(restartBtn);

        mainLayout->addWidget(driverGroup);

        // --- Bottom Row ---
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        refreshBtn->setFixedWidth(120);
        connect(refreshBtn, &QPushButton::clicked, this, &AudioGui::refreshDevices);
        btnRow->addWidget(refreshBtn);

        mainLayout->addLayout(btnRow);

        tabs->addTab(audioTab, "Audio");

        // --- Health Check Tab ---
        auto *checker = new AudioHealthChecker(this);
        auto *healthWidget = new HealthCheckWidget(checker);
        tabs->addTab(healthWidget, "Health Check");

        outerLayout->addWidget(tabs);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar()->showMessage("Backend: " + backend);

        applyStyle();
        refreshDevices();
    }

private slots:
    void detectBackend() {
        QProcess p;
        p.start("which", {"wpctl"});
        p.waitForFinished(3000);
        if (p.exitCode() == 0) {
            backend = "PipeWire (wpctl)";
            backendCmd = "wpctl";
        } else {
            p.start("which", {"pactl"});
            p.waitForFinished(3000);
            if (p.exitCode() == 0) {
                backend = "PulseAudio (pactl)";
                backendCmd = "pactl";
            } else {
                backend = "None found";
                backendCmd = "";
            }
        }
    }

    void refreshDevices() {
        if (backendCmd.isEmpty()) return;

        outputDevices.clear();
        inputDevices.clear();

        if (backendCmd == "wpctl") {
            refreshWpctl();
        } else {
            refreshPactl();
        }

        // Rebuild UI
        QLayoutItem *item;
        while ((item = scrollLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }

        auto *outLabel = new QLabel("Output Devices");
        outLabel->setStyleSheet("color: #89b4fa; font-weight: bold; font-size: 16px; margin-top: 10px; margin-bottom: 5px;");
        scrollLayout->addWidget(outLabel);

        for (const auto &dev : outputDevices) {
            auto *w = new AudioDeviceWidget(dev, false, backendCmd);
            connect(w, &AudioDeviceWidget::requestRefresh, this, &AudioGui::refreshDevices);
            scrollLayout->addWidget(w);
        }

        auto *inLabel = new QLabel("Input Devices");
        inLabel->setStyleSheet("color: #89b4fa; font-weight: bold; font-size: 16px; margin-top: 20px; margin-bottom: 5px;");
        scrollLayout->addWidget(inLabel);

        for (const auto &dev : inputDevices) {
            auto *w = new AudioDeviceWidget(dev, true, backendCmd);
            connect(w, &AudioDeviceWidget::requestRefresh, this, &AudioGui::refreshDevices);
            scrollLayout->addWidget(w);
        }

        scrollLayout->addStretch();

        statusBar()->showMessage(QString("Backend: %1 | %2 outputs, %3 inputs")
            .arg(backend).arg(outputDevices.size()).arg(inputDevices.size()));
    }

    void refreshWpctl() {
        QProcess p;
        p.start("wpctl", {"status"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        bool inSinks = false, inSources = false;

        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.contains("Sinks:")) { inSinks = true; inSources = false; continue; }
            if (trimmed.contains("Sources:")) { inSources = true; inSinks = false; continue; }
            if (trimmed.contains("Streams:") || trimmed.contains("─ Filters") || trimmed.contains("─ Devices") || trimmed.startsWith("Video") || trimmed.startsWith("Settings")) {
                inSinks = false; inSources = false;
                continue;
            }

            if ((inSinks || inSources) && (trimmed.startsWith("├") || trimmed.startsWith("└") || trimmed.startsWith("│") || trimmed.at(0).isDigit() || trimmed.startsWith("*"))) {
                bool isDefault = trimmed.contains("*");
                QString clean = trimmed;
                clean.remove(QRegularExpression("^[│├└\\s\\*]+"));
                
                int dotIdx = clean.indexOf('.');
                if (dotIdx == -1) continue;
                
                QString idStr = clean.left(dotIdx).trimmed();
                if (idStr.isEmpty()) continue;
                
                QString rest = clean.mid(dotIdx + 1).trimmed();
                int volIdx = rest.lastIndexOf("[vol:");
                QString name;
                double vol = 1.0;
                bool muted = false;
                
                if (volIdx != -1) {
                    name = rest.left(volIdx).trimmed();
                    QString volStr = rest.mid(volIdx);
                    if (volStr.contains("MUTED")) muted = true;
                    QRegularExpression rx("vol:\\s*([0-9.]+)");
                    QRegularExpressionMatch match = rx.match(volStr);
                    if (match.hasMatch()) {
                        vol = match.captured(1).toDouble();
                    }
                } else {
                    name = rest;
                }

                if (name.isEmpty()) continue;

                AudioDevice dev;
                dev.id = idStr;
                dev.name = name;
                dev.isDefault = isDefault;
                dev.volume = static_cast<int>(vol * 100);
                dev.muted = muted;

                if (inSinks) outputDevices.append(dev);
                else inputDevices.append(dev);
            }
        }
    }

    void refreshPactl() {
        QProcess p;
        p.start("pactl", {"list", "sinks", "short"});
        p.waitForFinished(5000);
        QString out = p.readAllStandardOutput();

        QProcess p2;
        p2.start("pactl", {"get-default-sink"});
        p2.waitForFinished(3000);
        QString defaultSink = p2.readAllStandardOutput().trimmed();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('\t');
            if (parts.size() < 2) continue;

            AudioDevice dev;
            dev.id = parts[0];
            dev.name = parts[1];
            dev.isDefault = (dev.name == defaultSink || dev.id == defaultSink);
            dev.volume = 100;
            dev.muted = false;
            outputDevices.append(dev);
        }

        p.start("pactl", {"list", "sources", "short"});
        p.waitForFinished(5000);
        out = p.readAllStandardOutput();

        QProcess p3;
        p3.start("pactl", {"get-default-source"});
        p3.waitForFinished(3000);
        QString defaultSource = p3.readAllStandardOutput().trimmed();

        for (const QString &line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('\t');
            if (parts.size() < 2) continue;

            AudioDevice dev;
            dev.id = parts[0];
            dev.name = parts[1];
            dev.isDefault = (dev.name == defaultSource || dev.id == defaultSource);
            dev.volume = 100;
            dev.muted = false;
            inputDevices.append(dev);
        }

        // Fetch volumes for outputs
        for (int i = 0; i < outputDevices.size(); ++i) {
            QProcess pVol;
            pVol.start("pactl", {"get-sink-volume", outputDevices[i].id});
            pVol.waitForFinished(1000);
            QString vOut = pVol.readAllStandardOutput().trimmed();
            outputDevices[i].volume = vOut.section(' ', 4, 4).remove('%').toInt();

            QProcess pMute;
            pMute.start("pactl", {"get-sink-mute", outputDevices[i].id});
            pMute.waitForFinished(1000);
            QString muteOut = pMute.readAllStandardOutput().trimmed();
            outputDevices[i].muted = muteOut.contains("yes");
        }

        // Fetch volumes for inputs
        for (int i = 0; i < inputDevices.size(); ++i) {
            QProcess pVol;
            pVol.start("pactl", {"get-source-volume", inputDevices[i].id});
            pVol.waitForFinished(1000);
            QString vOut = pVol.readAllStandardOutput().trimmed();
            inputDevices[i].volume = vOut.section(' ', 4, 4).remove('%').toInt();

            QProcess pMute;
            pMute.start("pactl", {"get-source-mute", inputDevices[i].id});
            pMute.waitForFinished(1000);
            QString muteOut = pMute.readAllStandardOutput().trimmed();
            inputDevices[i].muted = muteOut.contains("yes");
        }
    }

    void restartAudioService() {
        if (backendCmd == "wpctl") {
            QProcess::startDetached("systemctl", {"--user", "restart", "pipewire", "pipewire-pulse", "wireplumber"});
        } else {
            QProcess::startDetached("systemctl", {"--user", "restart", "pulseaudio"});
        }
        statusBar()->showMessage("Restarted Audio Service...");
        QTimer::singleShot(2000, this, &AudioGui::refreshDevices);
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #181825; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QString backend;
    QString backendCmd;
    QVBoxLayout *scrollLayout;
    QList<AudioDevice> outputDevices;
    QList<AudioDevice> inputDevices;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--list") {
            QProcess p;
            p.start("wpctl", {"status"});
            if (p.waitForFinished(3000) && p.exitCode() == 0) {
                printf("%s", p.readAllStandardOutput().constData());
            } else {
                p.start("pactl", {"list", "sinks", "short"});
                if (p.waitForFinished(3000)) {
                    printf("%s", p.readAllStandardOutput().constData());
                }
            }
            return 0;
        } else if (arg == "--set-output" && i + 1 < argc) {
            QString id = argv[++i];
            QProcess p;
            p.start("wpctl", {"set-default", id});
            p.waitForFinished(3000);
            return 0;
        } else if (arg == "--set-input" && i + 1 < argc) {
            QString id = argv[++i];
            QProcess p;
            p.start("wpctl", {"set-default", id});
            p.waitForFinished(3000);
            return 0;
        } else if (arg == "--volume" && i + 1 < argc) {
            int vol = QString(argv[++i]).toInt();
            if (vol < 0) vol = 0;
            if (vol > 100) vol = 100;
            double level = vol / 100.0;
            QProcess p;
            p.start("wpctl", {"set-volume", "@DEFAULT_SINK@", QString::number(level, 'f', 2)});
            p.waitForFinished(3000);
            return 0;
        } else if (arg == "--mute") {
            QProcess p;
            p.start("wpctl", {"set-mute", "@DEFAULT_SINK@", "1"});
            p.waitForFinished(3000);
            return 0;
        } else if (arg == "--unmute") {
            QProcess p;
            p.start("wpctl", {"set-mute", "@DEFAULT_SINK@", "0"});
            p.waitForFinished(3000);
            return 0;
        }
    }

    AudioGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
