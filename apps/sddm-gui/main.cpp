#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

class SddmConfigurator : public QWidget {
    Q_OBJECT

public:
    SddmConfigurator(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("SDDM Customizer");
        resize(400, 150);

        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(15);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *title = new QLabel("<b>Customize SDDM Login Manager</b>");
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);

        auto *themeLayout = new QHBoxLayout();
        themeLayout->addWidget(new QLabel("Select Theme:"));
        themeCombo = new QComboBox();
        populateThemes();
        themeLayout->addWidget(themeCombo);
        layout->addLayout(themeLayout);

        auto *saveBtn = new QPushButton("Apply Theme");
        saveBtn->setMinimumHeight(40);
        layout->addWidget(saveBtn);

        connect(saveBtn, &QPushButton::clicked, this, &SddmConfigurator::saveConfig);
    }

private:
    QComboBox *themeCombo;

    void populateThemes() {
        QDir themesDir("/usr/share/sddm/themes");
        if (themesDir.exists()) {
            QStringList themes = themesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            themeCombo->addItems(themes);
        } else {
            themeCombo->addItem("No themes found");
            themeCombo->setEnabled(false);
        }
    }

    void saveConfig() {
        QString selectedTheme = themeCombo->currentText();
        if (selectedTheme.isEmpty() || selectedTheme == "No themes found") return;

        // Create a temporary config file
        QString tempFile = "/tmp/sddm-custom.conf";
        QFile file(tempFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Theme]\n";
            out << "Current=" << selectedTheme << "\n";
            file.close();

            // Make sure the directory exists before copying
            QProcess::execute("pkexec", {"mkdir", "-p", "/etc/sddm.conf.d/"});

            // Use pkexec to copy it to /etc/sddm.conf.d/
            QProcess process;
            process.start("pkexec", {"cp", tempFile, "/etc/sddm.conf.d/custom.conf"});
            process.waitForFinished();

            if (process.exitCode() == 0) {
                QMessageBox::information(this, "Success", "SDDM theme successfully updated to: " + selectedTheme + "\nIt will take effect on your next login.");
            } else {
                QMessageBox::critical(this, "Error", "Failed to update SDDM config. Did you provide the correct password?");
            }
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Add a basic stylesheet for aesthetics
    app.setStyleSheet("QWidget { font-family: sans-serif; font-size: 14px; }"
                      "QPushButton { background-color: #3b82f6; color: white; border-radius: 5px; font-weight: bold; }"
                      "QPushButton:hover { background-color: #2563eb; }"
                      "QComboBox { padding: 5px; border: 1px solid #ccc; border-radius: 4px; }");

    SddmConfigurator window;
    window.show();
    return app.exec();
}

#include "main.moc"
