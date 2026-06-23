#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGroupBox>
#include <QStatusBar>
#include <QProcess>
#include <QTextEdit>
#include <QMessageBox>
#include <QTimer>

class SyncGui : public QMainWindow {
    Q_OBJECT
public:
    SyncGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Dotfiles Git Sync");
        resize(400, 300);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *group = new QGroupBox("Dotfiles Git Sync Settings");
        auto *groupLayout = new QVBoxLayout(group);
        
        statusText = new QTextEdit();
        statusText->setReadOnly(true);
        statusText->setStyleSheet("background-color: #181825; border: 1px solid #313244; border-radius: 4px; padding: 4px; font-family: monospace;");
        groupLayout->addWidget(new QLabel("Git Status:"));
        groupLayout->addWidget(statusText, 1);

        auto *rowCommitMessage = new QHBoxLayout();
        rowCommitMessage->addWidget(new QLabel("Commit Message"));
        commitInput = new QLineEdit();
        commitInput->setText("Update dotfiles via Control Center");
        rowCommitMessage->addWidget(commitInput);
        groupLayout->addLayout(rowCommitMessage);

        mainLayout->addWidget(group);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnGitStatus = new QPushButton("Git Status");
        btnLayout->addWidget(btnGitStatus);
        auto *btnCommitPush = new QPushButton("Commit & Push");
        btnLayout->addWidget(btnCommitPush);
        auto *btnPullUpdates = new QPushButton("Pull Updates");
        btnLayout->addWidget(btnPullUpdates);
        mainLayout->addLayout(btnLayout);

        connect(btnGitStatus, &QPushButton::clicked, this, &SyncGui::runGitStatus);
        connect(btnCommitPush, &QPushButton::clicked, this, &SyncGui::runCommitPush);
        connect(btnPullUpdates, &QPushButton::clicked, this, &SyncGui::runPullUpdates);
        
        statusBar()->showMessage("Ready");
        applyStyle();

        // Run initial status check
        QTimer::singleShot(100, this, &SyncGui::runGitStatus);
    }

private slots:
    void runGitStatus() {
        statusBar()->showMessage("Checking git status...");
        QProcess p;
        p.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
        p.start("git", {"status", "-s"});
        p.waitForFinished();
        QString out = p.readAllStandardOutput();
        if (out.trimmed().isEmpty()) {
            statusText->setText("Working directory clean. No changes to sync.");
        } else {
            statusText->setText(out);
        }
        statusBar()->showMessage("Status updated", 3000);
    }

    void runCommitPush() {
        QString msg = commitInput->text();
        if (msg.isEmpty()) msg = "Update config";
        
        statusBar()->showMessage("Adding and committing changes...");
        QProcess pAdd;
        pAdd.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
        pAdd.start("git", {"add", "."});
        pAdd.waitForFinished();

        QProcess pCommit;
        pCommit.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
        pCommit.start("git", {"commit", "-m", msg});
        pCommit.waitForFinished();

        statusBar()->showMessage("Pushing to remote...");
        QProcess pPush;
        pPush.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
        pPush.start("git", {"push"});
        pPush.waitForFinished();

        QMessageBox::information(this, "Success", "Changes successfully pushed to remote!");
        runGitStatus();
    }

    void runPullUpdates() {
        statusBar()->showMessage("Pulling latest updates...");
        QProcess p;
        p.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
        p.start("git", {"pull"});
        p.waitForFinished();
        QString out = p.readAllStandardOutput();
        QMessageBox::information(this, "Pull Result", out.isEmpty() ? p.readAllStandardError() : out);
        runGitStatus();
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
            "QTextEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QTextEdit *statusText;
    QLineEdit *commitInput;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // CLI argument parsing
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--status") {
            QProcess p;
            p.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
            p.start("git", {"status", "-s"});
            if (p.waitForFinished()) {
                printf("%s", p.readAllStandardOutput().constData());
            }
            return 0;
        } else if (arg == "--pull") {
            QProcess p;
            p.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
            p.start("git", {"pull"});
            if (p.waitForFinished()) {
                printf("%s", p.readAllStandardOutput().constData());
            }
            return 0;
        } else if (arg == "--commit" && i + 1 < argc) {
            QString msg = argv[++i];
            
            QProcess pAdd;
            pAdd.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
            pAdd.start("git", {"add", "."});
            pAdd.waitForFinished();

            QProcess pCommit;
            pCommit.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
            pCommit.start("git", {"commit", "-m", msg});
            pCommit.waitForFinished();

            QProcess pPush;
            pPush.setWorkingDirectory("/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt");
            pPush.start("git", {"push"});
            if (pPush.waitForFinished()) {
                printf("Successfully committed and pushed.\n");
            }
            return 0;
        }
    }

    SyncGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
