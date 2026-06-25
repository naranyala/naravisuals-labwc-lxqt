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
#include <QSlider>
#include <QScrollArea>
#include <QFrame>
#include <QInputDialog>

struct InputDevice {
    QString name;
    QString type;
    QString node;
};

class InputGui : public QMainWindow {
    Q_OBJECT

public:
    InputGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Input Devices Manager");
        resize(560, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Configure touchpad, mouse, and keyboard settings");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        auto *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: #1e1e2e; border: none; }");

        auto *container = new QWidget();
        contentLayout = new QVBoxLayout(container);
        contentLayout->setSpacing(12);
        scroll->setWidget(container);
        mainLayout->addWidget(scroll, 1);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &InputGui::scanDevices);
        btnRow->addWidget(refreshBtn);
        mainLayout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
        scanDevices();
    }

private slots:
    void scanDevices() {
        QLayoutItem *item;
        while ((item = contentLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        QProcess p;
        p.start("libinput", {"list-devices"});
        p.waitForFinished(5000);
        QString output = p.readAllStandardOutput();

        if (output.trimmed().isEmpty()) {
            auto *lbl = new QLabel("No input devices found.\nEnsure libinput is installed.");
            lbl->setStyleSheet("color: #a6adc8; padding: 40px; font-size: 16px;");
            lbl->setAlignment(Qt::AlignCenter);
            contentLayout->addWidget(lbl);
            return;
        }

        QStringList blocks = output.split("\n\n", Qt::SkipEmptyParts);
        for (const QString &block : blocks) {
            QStringList lines = block.split('\n', Qt::SkipEmptyParts);
            if (lines.isEmpty()) continue;

            InputDevice dev;
            dev.name = lines[0].trimmed().split("  ").last().trimmed();
            dev.type = "unknown";
            dev.node = "/dev/input/event?";

            for (const QString &line : lines) {
                QString t = line.trimmed();
                if (t.startsWith("Capabilities:")) {
                    if (t.contains("pointer")) dev.type = "pointer";
                    else if (t.contains("keyboard")) dev.type = "keyboard";
                    else if (t.contains("tablet")) dev.type = "tablet";
                }
                if (t.startsWith("Device:")) dev.name = t.section(':', 1).trimmed();
                if (t.startsWith("/dev/input")) dev.node = t.trimmed();
            }

            auto *card = createDeviceCard(dev);
            contentLayout->addWidget(card);
        }
        contentLayout->addStretch();
        statusBar()->showMessage("Scanned input devices");
    }

private:
    QFrame *createDeviceCard(const InputDevice &dev) {
        auto *card = new QFrame();
        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(
            "QFrame { background-color: #181825; border: 1px solid #45475a; border-radius: 8px; padding: 12px; }"
        );
        auto *layout = new QVBoxLayout(card);

        auto *titleLabel = new QLabel(QString("%1 (%2)").arg(dev.name, dev.type));
        titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #89b4fa;");
        layout->addWidget(titleLabel);

        auto *nodeLabel = new QLabel(dev.node);
        nodeLabel->setStyleSheet("color: #a6adc8; font-size: 12px;");
        layout->addWidget(nodeLabel);

        if (dev.type == "pointer") {
            auto *natScroll = new QCheckBox("Natural Scrolling");
            natScroll->setStyleSheet("QCheckBox { color: #cdd6f4; }");
            connect(natScroll, &QCheckBox::toggled, this, [this, dev](bool on) {
                QProcess::startDetached("libinput", {"set-click-finger", dev.node, "1"});
            });
            layout->addWidget(natScroll);

            auto *speedRow = new QHBoxLayout();
            auto *speedLabel = new QLabel("Speed:");
            speedLabel->setStyleSheet("color: #a6adc8;");
            speedRow->addWidget(speedLabel);
            auto *speedSlider = new QSlider(Qt::Horizontal);
            speedSlider->setRange(-100, 100);
            speedSlider->setValue(0);
            speedSlider->setStyleSheet(
                "QSlider::groove:horizontal { height: 6px; background: #313244; border-radius: 3px; }"
                "QSlider::handle:horizontal { background: #89b4fa; width: 16px; height: 16px; margin: -5px 0; border-radius: 8px; }"
                "QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 3px; }"
            );
            speedRow->addWidget(speedSlider, 1);
            layout->addLayout(speedRow);
        } else if (dev.type == "tablet" || dev.type == "touch") {
            auto *mapBtn = new QPushButton("Map to output...");
            mapBtn->setStyleSheet(
                "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
                "padding: 8px; font-weight: bold; border: none; }"
            );
            connect(mapBtn, &QPushButton::clicked, this, [this, dev]() {
                // Get available outputs via wlr-randr or kscreen
                QProcess p;
                p.start("wlr-randr", {"--dry-run"});
                p.waitForFinished(2000);
                QString out = p.readAllStandardOutput();
                // Try simpler approach: prompt user for output name
                bool ok;
                QString output = QInputDialog::getText(nullptr, "Map Touch/Tablet",
                    "Output name (e.g. eDP-1, HDMI-A-1):\n"
                    "Run 'wlr-randr' in terminal to list available outputs.",
                    QLineEdit::Normal, "", &ok);
                if (ok && !output.isEmpty()) {
                    QProcess::startDetached("wlr-randr", {"--output", output, "--pos", "0,0"});
                    QProcess::startDetached("libinput", {"set-output", dev.node, output});
                }
            });
            layout->addWidget(mapBtn);

            auto *calBtn = new QPushButton("Calibrate");
            calBtn->setStyleSheet(
                "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
                "padding: 8px; font-weight: bold; border: none; }"
            );
            connect(calBtn, &QPushButton::clicked, this, [this, dev]() {
                // Launch libinput measure touchpad for calibration
                QProcess::startDetached("libinput", {"debug-gui", "--device", dev.node});
            });
            layout->addWidget(calBtn);
        }

        return card;
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QVBoxLayout *contentLayout;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--list") {
            QProcess p;
            p.start("libinput", {"list-devices"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
        if (arg == "--enable-tap" && i + 1 < argc) {
            QString dev = argv[++i];
            QProcess p;
            p.start("libinput", {"tap-set-enabled", dev, "enabled"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return p.exitCode();
        }
        if (arg == "--disable-tap" && i + 1 < argc) {
            QString dev = argv[++i];
            QProcess p;
            p.start("libinput", {"tap-set-enabled", dev, "disabled"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return p.exitCode();
        }
        if (arg == "--enable-natScroll" && i + 1 < argc) {
            QString dev = argv[++i];
            QProcess p;
            p.start("libinput", {"scroll-set-natural", dev, "enabled"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return p.exitCode();
        }
        if (arg == "--disable-natScroll" && i + 1 < argc) {
            QString dev = argv[++i];
            QProcess p;
            p.start("libinput", {"scroll-set-natural", dev, "disabled"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return p.exitCode();
        }
    }
    InputGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
