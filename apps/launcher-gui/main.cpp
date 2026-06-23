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

class LauncherGui : public QMainWindow {
public:
    LauncherGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("App Launcher Configurator");
        resize(400, 300);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *group = new QGroupBox("App Launcher Configurator Settings");
        auto *groupLayout = new QVBoxLayout(group);
        
        auto *rowLauncherWidth = new QHBoxLayout();
        rowLauncherWidth->addWidget(new QLabel("Launcher Width"));
        rowLauncherWidth->addWidget(new QLineEdit());
        groupLayout->addLayout(rowLauncherWidth);
        auto *rowOpacity = new QHBoxLayout();
        rowOpacity->addWidget(new QLabel("Opacity"));
        rowOpacity->addWidget(new QLineEdit());
        groupLayout->addLayout(rowOpacity);
        auto *rowIconSize = new QHBoxLayout();
        rowIconSize->addWidget(new QLabel("Icon Size"));
        rowIconSize->addWidget(new QLineEdit());
        groupLayout->addLayout(rowIconSize);


        mainLayout->addWidget(group);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnPreview = new QPushButton("Preview");
        btnLayout->addWidget(btnPreview);
        auto *btnSaveTheme = new QPushButton("Save Theme");
        btnLayout->addWidget(btnSaveTheme);

        mainLayout->addLayout(btnLayout);
        
        statusBar()->showMessage("Ready");
        applyStyle();
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; padding: 6px; border-radius: 4px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    LauncherGui gui;
    gui.show();
    return app.exec();
}
