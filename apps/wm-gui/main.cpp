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
#include <QTabWidget>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

class WmGui : public QMainWindow {
public:
    WmGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Window Manager Configurator");
        resize(500, 400);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(16, 16, 16, 16);

        auto *tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 20px; border: 1px solid #313244; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }"
        );

        // --- Config Tab ---
        auto *configTab = new QWidget();
        auto *layout = new QVBoxLayout(configTab);
        layout->setSpacing(16);
        layout->setContentsMargins(20, 20, 20, 20);

        auto *group = new QGroupBox("Window Manager Settings");
        group->setStyleSheet(
            "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; "
            "border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }");
        auto *groupLayout = new QVBoxLayout(group);

        auto *rowGaps = new QHBoxLayout();
        rowGaps->addWidget(new QLabel("Gaps"));
        rowGaps->addWidget(new QLineEdit());
        groupLayout->addLayout(rowGaps);

        auto *rowBorderThickness = new QHBoxLayout();
        rowBorderThickness->addWidget(new QLabel("Border Thickness"));
        rowBorderThickness->addWidget(new QLineEdit());
        groupLayout->addLayout(rowBorderThickness);

        auto *rowCornerRadius = new QHBoxLayout();
        rowCornerRadius->addWidget(new QLabel("Corner Radius"));
        rowCornerRadius->addWidget(new QLineEdit());
        groupLayout->addLayout(rowCornerRadius);

        layout->addWidget(group);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnSaveLayout = new QPushButton("Save Layout");
        btnLayout->addWidget(btnSaveLayout);
        layout->addLayout(btnLayout);

        tabs->addTab(configTab, "Config");

        // --- Health Check Tab ---
        auto *checker = new WmHealthChecker(this);
        auto *healthWidget = new HealthCheckWidget(checker);
        tabs->addTab(healthWidget, "Health Check");

        mainLayout->addWidget(tabs);

        statusBar()->showMessage("Ready");
        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        applyStyle();
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }");
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    WmGui gui;
    gui.show();
    return app.exec();
}
