#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QStatusBar>
#include <QCheckBox>

class LogGui : public QMainWindow {
    Q_OBJECT

public:
    LogGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("System Log Viewer");
        resize(750, 520);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *layout = new QVBoxLayout(central);
        layout->setSpacing(10);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *header = new QLabel("View and filter journalctl logs");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        layout->addWidget(header);

        // --- Filter bar ---
        auto *filterRow = new QHBoxLayout();

        priorityCombo = new QComboBox();
        priorityCombo->addItems({"All priorities", "emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"});
        priorityCombo->setMinimumHeight(32);
        connect(priorityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LogGui::loadLogs);
        filterRow->addWidget(priorityCombo);

        auto *searchLabel = new QLabel("Filter:");
        searchLabel->setStyleSheet("color: #a6adc8;");
        filterRow->addWidget(searchLabel);

        searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("grep pattern...");
        searchEdit->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 6px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(searchEdit, &QLineEdit::returnPressed, this, &LogGui::loadLogs);
        filterRow->addWidget(searchEdit, 1);

        layout->addLayout(filterRow);

        // --- Log display ---
        logDisplay = new QPlainTextEdit();
        logDisplay->setReadOnly(true);
        logDisplay->setLineWrapMode(QPlainTextEdit::NoWrap);
        logDisplay->setStyleSheet(
            "QPlainTextEdit { background-color: #11111b; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; font-family: monospace; font-size: 13px; padding: 8px; }"
        );
        layout->addWidget(logDisplay, 1);

        // --- Buttons ---
        auto *btnRow = new QHBoxLayout();

        auto *liveCheck = new QCheckBox("Follow (live)");
        liveCheck->setStyleSheet("QCheckBox { color: #a6adc8; }");
        connect(liveCheck, &QCheckBox::toggled, this, [this](bool on) {
            if (on) startLiveLog();
            else stopLiveLog();
        });
        btnRow->addWidget(liveCheck);

        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &LogGui::loadLogs);
        btnRow->addWidget(refreshBtn);

        layout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
        loadLogs();
    }

private slots:
    void loadLogs() {
        QStringList args;
        args << "-n" << "500" << "--no-pager";

        int prioIdx = priorityCombo->currentIndex();
        if (prioIdx > 0) {
            QStringList priorities = {"emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"};
            args << "-p" << priorities[prioIdx - 1];
        }

        QString filter = searchEdit->text().trimmed();
        if (!filter.isEmpty()) {
            args << "-g" << filter;
        }

        QProcess p;
        p.start("journalctl", args);
        p.waitForFinished(10000);
        QString output = p.readAllStandardOutput();

        logDisplay->setPlainText(output);

        // Scroll to bottom
        QTextCursor cursor = logDisplay->textCursor();
        cursor.movePosition(QTextCursor::End);
        logDisplay->setTextCursor(cursor);

        statusBar()->showMessage(QString("Showing last %1 lines").arg(output.count('\n')));
    }

    void startLiveLog() {
        if (liveProc) return;
        liveProc = new QProcess(this);
        QStringList args;
        args << "-f" << "-n" << "0" << "--no-pager";
        int prioIdx = priorityCombo->currentIndex();
        if (prioIdx > 0) {
            QStringList priorities = {"emerg", "alert", "crit", "err", "warning", "notice", "info", "debug"};
            args << "-p" << priorities[prioIdx - 1];
        }
        connect(liveProc, &QProcess::readyReadStandardOutput, this, [this]() {
            logDisplay->appendPlainText(QString::fromUtf8(liveProc->readAllStandardOutput()));
            QTextCursor cursor = logDisplay->textCursor();
            cursor.movePosition(QTextCursor::End);
            logDisplay->setTextCursor(cursor);
        });
        liveProc->start("journalctl", args);
    }

    void stopLiveLog() {
        if (liveProc) {
            liveProc->kill();
            liveProc->deleteLater();
            liveProc = nullptr;
        }
    }

private:
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

    QComboBox *priorityCombo;
    QLineEdit *searchEdit;
    QPlainTextEdit *logDisplay;
    QProcess *liveProc = nullptr;
};

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        QString arg(argv[i]);
        if (arg == "--last") {
            QApplication app(argc, argv);
            int count = 50;
            if (i + 1 < argc && !QString(argv[i + 1]).startsWith("--")) {
                count = QString(argv[++i]).toInt();
            }
            QProcess p;
            p.start("journalctl", {"-n", QString::number(count), "--no-pager"});
            p.waitForFinished(5000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
        if (arg == "--follow") {
            QApplication app(argc, argv);
            QProcess p;
            p.setProcessChannelMode(QProcess::ForwardedChannels);
            p.start("journalctl", {"-f", "--no-pager"});
            printf("Following logs (Ctrl+C to stop)...\n");
            p.waitForFinished(-1);
            return 0;
        }
        if (arg == "--filter" && i + 1 < argc) {
            QApplication app(argc, argv);
            QString pattern = argv[++i];
            QProcess p;
            p.start("journalctl", {"--no-pager", "-g", pattern});
            p.waitForFinished(10000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
        if (arg == "--priority" && i + 1 < argc) {
            QApplication app(argc, argv);
            QString level = argv[++i];
            QProcess p;
            p.start("journalctl", {"--no-pager", "-p", level});
            p.waitForFinished(10000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
        if (arg == "--since" && i + 1 < argc) {
            QApplication app(argc, argv);
            QString time = argv[++i];
            QProcess p;
            p.start("journalctl", {"--no-pager", "--since", time});
            p.waitForFinished(10000);
            printf("%s", p.readAllStandardOutput().constData());
            return 0;
        }
    }
    QApplication app(argc, argv);
    LogGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
