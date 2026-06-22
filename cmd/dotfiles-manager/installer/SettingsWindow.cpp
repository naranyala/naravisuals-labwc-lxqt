#include "SettingsWindow.h"
#include "Pages.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>

SettingsWindow::SettingsWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("NaraVisuals Control Center");
    resize(750, 500);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Sidebar
    QListWidget *sidebar = new QListWidget();
    sidebar->setFixedWidth(200);
    sidebar->setStyleSheet(
        "QListWidget { background-color: #2e3440; color: #eceff4; font-size: 14px; border: none; padding-top: 10px; }"
        "QListWidget::item { padding: 10px 15px; }"
        "QListWidget::item:selected { background-color: #4c566a; border-left: 4px solid #88c0d0; }"
    );

    sidebar->addItem("Global Themes");
    sidebar->addItem("Component Tweaks");
    sidebar->addItem("Wallpaper");
    sidebar->addItem("System");

    // Stacked Widget for Pages
    QStackedWidget *stackedWidget = new QStackedWidget();
    stackedWidget->addWidget(new GlobalThemesPage());
    stackedWidget->addWidget(new ComponentThemesPage());
    stackedWidget->addWidget(new WallpaperPage());
    stackedWidget->addWidget(new SystemPage());
    
    // Add padding to the pages area
    QWidget *pageContainer = new QWidget();
    QVBoxLayout *pageLayout = new QVBoxLayout(pageContainer);
    pageLayout->setContentsMargins(30, 30, 30, 30);
    pageLayout->addWidget(stackedWidget);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(pageContainer);

    // Link sidebar clicks to stacked widget pages
    QObject::connect(sidebar, &QListWidget::currentRowChanged, stackedWidget, &QStackedWidget::setCurrentIndex);
    
    // Select first item by default
    sidebar->setCurrentRow(0);
}
