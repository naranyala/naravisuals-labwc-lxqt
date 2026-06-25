#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QProcess>
#include <QStatusBar>
#include <QTimer>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QRegularExpression>

struct ProcessInfo {
    QString pid;
    QString user;
    QString cpu;
    QString mem;
    QString vsz;
    QString rss;
    QString stat;
    QString start;
    QString time;
    QString command;
};

class ProcessGui : public QMainWindow {
    Q_OBJECT

public:
    ProcessGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Process Monitor");
        resize(850, 600);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Monitor and manage running processes");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        // Filter row
        auto *filterRow = new QHBoxLayout();
        filterRow->setSpacing(8);

        filterEdit = new QLineEdit();
        filterEdit->setPlaceholderText("Filter processes...");
        filterEdit->setMinimumHeight(32);
        filterRow->addWidget(filterEdit, 1);

        sortCombo = new QComboBox();
        sortCombo->addItems({"CPU%", "MEM%", "PID", "RSS", "COMMAND"});
        sortCombo->setMinimumHeight(32);
        sortRow = new QHBoxLayout();
        auto *sortLabel = new QLabel("Sort by:");
        sortLabel->setStyleSheet("color: #a6adc8;");
        sortRow->addWidget(sortLabel);
        sortRow->addWidget(sortCombo);
        filterRow->addLayout(sortRow);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(32);
        connect(refreshBtn, &QPushButton::clicked, this, &ProcessGui::refreshProcesses);
        filterRow->addWidget(refreshBtn);

        mainLayout->addLayout(filterRow);

        // Process table
        table = new QTableWidget();
        table->setColumnCount(7);
        table->setHorizontalHeaderLabels({"PID", "USER", "CPU%", "MEM%", "RSS", "STATE", "COMMAND"});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->verticalHeader()->setVisible(false);
        table->setSortingEnabled(true);
        table->horizontalHeader()->setStretchLastSection(true);
        table->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget { background-color: #1e1e2e; color: #cdd6f4; gridline-color: #313244; "
            "border: 1px solid #45475a; border-radius: 6px; }"
            "QTableWidget::item { padding: 4px; }"
            "QTableWidget::item:selected { background-color: #313244; color: #89b4fa; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; padding: 6px; "
            "border: 1px solid #313244; font-weight: bold; }"
        );
        mainLayout->addWidget(table);

        // Button row
        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(8);

        killBtn = new QPushButton("Kill Process");
        killBtn->setMinimumHeight(36);
        killBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #f5a0b8; }"
        );
        connect(killBtn, &QPushButton::clicked, this, &ProcessGui::killProcess);
        btnRow->addWidget(killBtn);

        sigtermBtn = new QPushButton("SIGTERM");
        sigtermBtn->setMinimumHeight(36);
        sigtermBtn->setStyleSheet(
            "QPushButton { background-color: #fab387; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #fbc0a0; }"
        );
        connect(sigtermBtn, &QPushButton::clicked, this, &ProcessGui::sigtermProcess);
        btnRow->addWidget(sigtermBtn);

        btnRow->addStretch();

        countLabel = new QLabel("Processes: 0");
        countLabel->setStyleSheet("color: #a6adc8; font-size: 13px;");
        btnRow->addWidget(countLabel);

        mainLayout->addLayout(btnRow);

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;"
        );
        statusBar()->showMessage("Ready");

        applyStyle();

        // Auto-refresh timer
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ProcessGui::refreshProcesses);
        timer->start(3000);

        connect(filterEdit, &QLineEdit::textChanged, this, &ProcessGui::applyFilter);
        connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { refreshProcesses(); });

        refreshProcesses();
    }

private slots:
    void refreshProcesses() {
        table->setSortingEnabled(false);
        table->setRowCount(0);

        QString sortFlag;
        switch (sortCombo->currentIndex()) {
            case 0: sortFlag = "--sort=-pcpu"; break;
            case 1: sortFlag = "--sort=-pmem"; break;
            case 2: sortFlag = "--sort=pid"; break;
            case 3: sortFlag = "--sort=-rss"; break;
            case 4: sortFlag = "--sort=args"; break;
            default: sortFlag = "--sort=-pcpu"; break;
        }

        QProcess proc;
        proc.start("ps", {"aux", sortFlag});
        proc.waitForFinished(3000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        int totalProcesses = 0;

        for (int i = 1; i < lines.size(); i++) {
            QStringList parts = lines[i].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 11) continue;

            ProcessInfo pi;
            pi.user = parts[0];
            pi.pid = parts[1];
            pi.cpu = parts[2];
            pi.mem = parts[3];
            pi.vsz = parts[4];
            pi.rss = parts[5];
            pi.stat = parts[7];
            pi.time = parts[9];
            pi.command = parts.mid(10).join(' ');

            if (!filterText.isEmpty() &&
                !pi.command.contains(filterText, Qt::CaseInsensitive) &&
                !pi.pid.contains(filterText) &&
                !pi.user.contains(filterText, Qt::CaseInsensitive)) {
                continue;
            }

            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(pi.pid));
            table->setItem(row, 1, new QTableWidgetItem(pi.user));
            table->setItem(row, 2, new QTableWidgetItem(pi.cpu));
            table->setItem(row, 3, new QTableWidgetItem(pi.mem));
            table->setItem(row, 4, new QTableWidgetItem(formatRss(pi.rss)));
            table->setItem(row, 5, new QTableWidgetItem(pi.stat));
            table->setItem(row, 6, new QTableWidgetItem(pi.command));
            totalProcesses++;
        }

        table->setSortingEnabled(true);
        countLabel->setText(QString("Processes: %1").arg(totalProcesses));
        statusBar()->showMessage(QString("Refreshed — %1 processes").arg(totalProcesses));
    }

    void killProcess() {
        auto *item = table->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Select a process to kill.");
            return;
        }
        int row = item->row();
        QString pid = table->item(row, 0)->text();
        QString cmd = table->item(row, 6)->text();

        auto reply = QMessageBox::question(this, "Kill Process",
            QString("Kill process %1?\nCommand: %2").arg(pid, cmd.left(80)),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QProcess::execute("kill", {"-9", pid});
            statusBar()->showMessage("Killed PID " + pid);
            QTimer::singleShot(500, this, &ProcessGui::refreshProcesses);
        }
    }

    void sigtermProcess() {
        auto *item = table->currentItem();
        if (!item) {
            QMessageBox::warning(this, "No Selection", "Select a process to signal.");
            return;
        }
        int row = item->row();
        QString pid = table->item(row, 0)->text();

        auto reply = QMessageBox::question(this, "SIGTERM Process",
            QString("Send SIGTERM to PID %1?").arg(pid),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QProcess::execute("kill", {pid});
            statusBar()->showMessage("Sent SIGTERM to PID " + pid);
            QTimer::singleShot(500, this, &ProcessGui::refreshProcesses);
        }
    }

    void applyFilter() {
        filterText = filterEdit->text();
        refreshProcesses();
    }

private:
    QString formatRss(const QString &rss) {
        bool ok;
        long kb = rss.toLong(&ok);
        if (!ok) return rss;
        if (kb >= 1048576) return QString::number(kb / 1048576.0, 'f', 1) + " GB";
        if (kb >= 1024) return QString::number(kb / 1024.0, 'f', 1) + " MB";
        return rss + " KB";
    }

    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
            "QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 6px 10px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
            "border-right: 5px solid transparent; border-top: 6px solid #a6adc8; margin-right: 8px; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; "
            "selection-background-color: #313244; border: 1px solid #45475a; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QTableWidget *table;
    QLineEdit *filterEdit;
    QComboBox *sortCombo;
    QPushButton *killBtn;
    QPushButton *sigtermBtn;
    QLabel *countLabel;
    QTimer *timer;
    QString filterText;
    QWidget *sortRow;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ProcessGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
