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
#include <QScrollArea>
#include <QFrame>
#include <QMap>
#include <QTimer>

struct DisplayOutput {
    QString name;
    bool connected;
    bool enabled;
    QString currentMode;
    QString currentRefresh;
    QString orientation;
    QStringList modes;
};

class DisplayGui : public QMainWindow {
    Q_OBJECT

public:
    DisplayGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Display Manager");
        resize(600, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("Configure display resolution, refresh rate, and orientation");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        auto *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: #1e1e2e; border: none; }");

        displayContainer = new QWidget();
        displayLayout = new QVBoxLayout(displayContainer);
        displayLayout->setSpacing(12);
        scroll->setWidget(displayContainer);
        mainLayout->addWidget(scroll, 1);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &DisplayGui::refreshDisplays);
        btnRow->addWidget(refreshBtn);

        mainLayout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar()->showMessage("Ready");

        applyStyle();
        refreshDisplays();
    }

private slots:
    void refreshDisplays() {
        QLayoutItem *item;
        while ((item = displayLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        displays.clear();

        if (!parseWlrRandr()) {
            auto *noDisp = new QLabel("No displays detected.\nEnsure wlr-randr is installed.");
            noDisp->setStyleSheet("color: #a6adc8; padding: 40px; font-size: 16px;");
            noDisp->setAlignment(Qt::AlignCenter);
            displayLayout->addWidget(noDisp);
            return;
        }

        for (auto &disp : displays) {
            auto *card = new QFrame();
            card->setFrameShape(QFrame::StyledPanel);
            card->setStyleSheet(
                "QFrame { background-color: #181825; border: 1px solid #45475a; border-radius: 8px; padding: 12px; }"
            );
            auto *cardLayout = new QVBoxLayout(card);

            // Title
            auto *titleLabel = new QLabel(QString("%1 %2").arg(disp.name, disp.connected ? "" : "(disconnected)"));
            titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #89b4fa;");
            cardLayout->addWidget(titleLabel);

            if (!disp.connected) {
                auto *statusLabel = new QLabel("Not connected");
                statusLabel->setStyleSheet("color: #a6adc8; font-style: italic;");
                cardLayout->addWidget(statusLabel);
                displayLayout->addWidget(card);
                continue;
            }

            // Mode selector
            auto *modeRow = new QHBoxLayout();
            auto *modeLabel = new QLabel("Resolution & Refresh:");
            modeLabel->setStyleSheet("color: #a6adc8;");
            modeRow->addWidget(modeLabel);

            auto *modeCombo = new QComboBox();
            modeCombo->setMinimumHeight(32);
            for (const auto &mode : disp.modes) {
                modeCombo->addItem(mode);
            }
            int modeIdx = disp.modes.indexOf(disp.currentMode + " " + disp.currentRefresh);
            if (modeIdx >= 0) modeCombo->setCurrentIndex(modeIdx);
            connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, name = disp.name](int idx) { applyMode(name, idx); });
            modeRow->addWidget(modeCombo, 1);
            cardLayout->addLayout(modeRow);

            // Orientation
            auto *orientRow = new QHBoxLayout();
            auto *orientLabel = new QLabel("Orientation:");
            orientLabel->setStyleSheet("color: #a6adc8;");
            orientRow->addWidget(orientLabel);

            auto *orientCombo = new QComboBox();
            orientCombo->setMinimumHeight(32);
            orientCombo->addItems({"normal", "90", "180", "270", "flipped", "flipped-90", "flipped-180", "flipped-270"});
            int orientIdx = orientCombo->findText(disp.orientation);
            if (orientIdx >= 0) orientCombo->setCurrentIndex(orientIdx);
            connect(orientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, name = disp.name](int idx) { applyOrientation(name, idx); });
            orientRow->addWidget(orientCombo, 1);
            cardLayout->addLayout(orientRow);

            // Enable/disable
            auto *enableRow = new QHBoxLayout();
            enableRow->addStretch();
            auto *toggleBtn = new QPushButton(disp.enabled ? "Disable" : "Enable");
            toggleBtn->setMinimumHeight(32);
            toggleBtn->setStyleSheet(
                disp.enabled
                    ? "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 6px 16px; font-weight: bold; border: none; }"
                      "QPushButton:hover { background-color: #f5a0b8; }"
                    : "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 6px 16px; font-weight: bold; border: none; }"
                      "QPushButton:hover { background-color: #b4f9b8; }"
            );
            connect(toggleBtn, &QPushButton::clicked, this, [this, name = disp.name, enabled = disp.enabled]() {
                toggleOutput(name, !enabled);
            });
            enableRow->addWidget(toggleBtn);
            cardLayout->addLayout(enableRow);

            displayLayout->addWidget(card);
        }

        displayLayout->addStretch();
        statusBar()->showMessage(QString("Detected %1 display(s)").arg(displays.size()));
    }

private:
    bool parseWlrRandr() {
        QProcess p;
        p.start("wlr-randr");
        p.waitForFinished(5000);
        QString output = p.readAllStandardOutput();

        if (output.trimmed().isEmpty()) return false;

        DisplayOutput current;
        bool hasCurrent = false;

        for (const QString &line : output.split('\n')) {
            QString trimmed = line.trimmed();
            if (trimmed.isEmpty()) continue;

            // New output line: "DP-1 " or "HDMI-A-1 " or "eDP-1 " followed by state
            if (!line.startsWith(' ') && !line.startsWith('\t') && !trimmed.startsWith('*')) {
                if (hasCurrent) displays.append(current);

                current = DisplayOutput();
                current.name = trimmed.section(' ', 0, 0);
                current.connected = !trimmed.contains("disconnected");
                current.enabled = trimmed.contains("enabled");
                hasCurrent = true;
            } else if (trimmed.startsWith("Enabled:") || trimmed.contains("enabled")) {
                current.enabled = trimmed.contains("yes") || trimmed.contains("enabled");
            } else if (trimmed.contains("current") && (trimmed.contains("Hz") || trimmed.contains("x"))) {
                // Mode line like: "  1920x1080 px, 60.000000 Hz (preferred, current)"
                QString mode = trimmed.section(" px,", 0, 0).trimmed();
                QString refresh = trimmed.section("Hz", 0, 0).trimmed().section(',', -1).trimmed();
                if (trimmed.contains("current")) {
                    current.currentMode = mode;
                    current.currentRefresh = refresh;
                }
                current.modes.append(mode + " " + refresh);
            } else if (trimmed.startsWith("Transform:")) {
                QString orient = trimmed.section(':', 1).trimmed();
                current.orientation = orient;
            }
        }
        if (hasCurrent) displays.append(current);

        return !displays.isEmpty();
    }

    int findDisplay(const QString &name) const {
        for (int i = 0; i < displays.size(); ++i)
            if (displays[i].name == name) return i;
        return -1;
    }

    void applyMode(const QString &name, int modeIdx) {
        int idx = findDisplay(name);
        if (modeIdx < 0 || idx < 0) return;
        DisplayOutput &disp = displays[idx];
        if (modeIdx >= disp.modes.size()) return;

        QString modeStr = disp.modes[modeIdx];
        QString resolution = modeStr.section(' ', 0, 0);
        QString refresh = modeStr.section(' ', 1);

        QProcess::startDetached("wlr-randr", {"--output", name, "--mode", resolution + " " + refresh});
        statusBar()->showMessage(QString("Applied %1 %2 to %3").arg(resolution, refresh, name));
    }

    void applyOrientation(const QString &name, int orientIdx) {
        QStringList transforms = {"normal", "90", "180", "270", "flipped", "flipped-90", "flipped-180", "flipped-270"};
        if (orientIdx < 0 || orientIdx >= transforms.size()) return;

        QProcess::startDetached("wlr-randr", {"--output", name, "--transform", transforms[orientIdx]});
        statusBar()->showMessage(QString("Applied orientation %1 to %2").arg(transforms[orientIdx], name));
    }

    void toggleOutput(const QString &name, bool enable) {
        if (enable) {
            QProcess::startDetached("wlr-randr", {"--output", name, "--on"});
        } else {
            QProcess::startDetached("wlr-randr", {"--output", name, "--off"});
        }
        QTimer::singleShot(500, this, &DisplayGui::refreshDisplays);
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
        );
    }

    QWidget *displayContainer;
    QVBoxLayout *displayLayout;
    QList<DisplayOutput> displays;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--list") {
            QProcess p;
            p.start("wlr-randr");
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        } else if (arg == "--mode" && i + 2 < argc) {
            QString output = argv[++i];
            QString mode = argv[++i];
            QProcess p;
            p.start("wlr-randr", {"--output", output, "--mode", mode});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        } else if (arg == "--orientation" && i + 2 < argc) {
            QString output = argv[++i];
            QString orient = argv[++i];
            QProcess p;
            p.start("wlr-randr", {"--output", output, "--transform", orient});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        } else if (arg == "--enable" && i + 1 < argc) {
            QString output = argv[++i];
            QProcess p;
            p.start("wlr-randr", {"--output", output, "--on"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        } else if (arg == "--disable" && i + 1 < argc) {
            QString output = argv[++i];
            QProcess p;
            p.start("wlr-randr", {"--output", output, "--off"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
    }

    DisplayGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
