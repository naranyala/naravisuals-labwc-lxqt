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

class NotificationsGui : public QMainWindow {
public:
    NotificationsGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Notifications Manager");
        resize(400, 300);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *group = new QGroupBox("Notifications Manager Settings");
        auto *groupLayout = new QVBoxLayout(group);
        
        auto *rowTimeout = new QHBoxLayout();
        rowTimeout->addWidget(new QLabel("Timeout (s)"));
        rowTimeout->addWidget(new QLineEdit());
        groupLayout->addLayout(rowTimeout);
        auto *rowWidth = new QHBoxLayout();
        rowWidth->addWidget(new QLabel("Width"));
        rowWidth->addWidget(new QLineEdit());
        groupLayout->addLayout(rowWidth);
        auto *rowHeight = new QHBoxLayout();
        rowHeight->addWidget(new QLabel("Height"));
        rowHeight->addWidget(new QLineEdit());
        groupLayout->addLayout(rowHeight);
        auto *rowMargin = new QHBoxLayout();
        rowMargin->addWidget(new QLabel("Margin"));
        rowMargin->addWidget(new QLineEdit());
        groupLayout->addLayout(rowMargin);


        mainLayout->addWidget(group);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnTestNotification = new QPushButton("Test Notification");
        btnLayout->addWidget(btnTestNotification);
        auto *btnSaveConfig = new QPushButton("Save Config");
        btnLayout->addWidget(btnSaveConfig);

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
    NotificationsGui gui;
    gui.show();
    return app.exec();
}
