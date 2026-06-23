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

class LockscreenGui : public QMainWindow {
public:
    LockscreenGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Lock Screen & Idle");
        resize(400, 300);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        auto *group = new QGroupBox("Lock Screen & Idle Settings");
        auto *groupLayout = new QVBoxLayout(group);
        
        auto *rowDimTime = new QHBoxLayout();
        rowDimTime->addWidget(new QLabel("Dim Time (min)"));
        rowDimTime->addWidget(new QLineEdit());
        groupLayout->addLayout(rowDimTime);
        auto *rowLockTime = new QHBoxLayout();
        rowLockTime->addWidget(new QLabel("Lock Time (min)"));
        rowLockTime->addWidget(new QLineEdit());
        groupLayout->addLayout(rowLockTime);
        auto *rowSuspendTime = new QHBoxLayout();
        rowSuspendTime->addWidget(new QLabel("Suspend Time (min)"));
        rowSuspendTime->addWidget(new QLineEdit());
        groupLayout->addLayout(rowSuspendTime);
        auto *rowRingColor = new QHBoxLayout();
        rowRingColor->addWidget(new QLabel("Ring Color"));
        rowRingColor->addWidget(new QLineEdit());
        groupLayout->addLayout(rowRingColor);


        mainLayout->addWidget(group);

        auto *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        auto *btnTestLock = new QPushButton("Test Lock");
        btnLayout->addWidget(btnTestLock);
        auto *btnSaveSettings = new QPushButton("Save Settings");
        btnLayout->addWidget(btnSaveSettings);

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
    LockscreenGui gui;
    gui.show();
    return app.exec();
}
