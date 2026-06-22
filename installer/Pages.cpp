#include "Pages.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QGroupBox>
#include <QComboBox>
#include <QFormLayout>

// --- GlobalThemesPage ---
GlobalThemesPage::GlobalThemesPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *title = new QLabel("Global Themes");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);
    
    QLabel *desc = new QLabel("Apply a unified look across the entire desktop.");
    layout->addWidget(desc);
    layout->addSpacing(20);

    QMap<QString, GlobalTheme> themes = {
        {"Nord", {"Nord", "Nordic", "Papirus-Dark", "Nordzy-cursors", "nord", "Nordic"}},
        {"Dracula", {"Dracula", "Dracula", "Candy-icons", "Dracula-cursors", "dracula", "Dracula"}},
        {"Catppuccin", {"Catppuccin", "Catppuccin-Mocha", "Papirus-Dark", "Catppuccin-Mocha-Cursors", "catppuccin", "Catppuccin"}}
    };

    for (auto it = themes.begin(); it != themes.end(); ++it) {
        QPushButton *btn = new QPushButton("Apply " + it.key());
        btn->setMinimumHeight(40);
        QObject::connect(btn, &QPushButton::clicked, [theme = it.value()]() {
            applyGlobalTheme(theme);
        });
        layout->addWidget(btn);
    }
    
    layout->addStretch();
}

// --- ComponentThemesPage ---
ComponentThemesPage::ComponentThemesPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *title = new QLabel("Component Tweaks");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);
    
    QLabel *desc = new QLabel("Fine-tune individual components independently.");
    layout->addWidget(desc);
    layout->addSpacing(20);

    QGroupBox *groupBox = new QGroupBox("Individual Settings");
    QFormLayout *formLayout = new QFormLayout(groupBox);

    QComboBox *gtkCombo = new QComboBox();
    gtkCombo->addItems({"Nordic", "Dracula", "Catppuccin-Mocha"});
    formLayout->addRow("GTK Theme:", gtkCombo);

    QComboBox *iconCombo = new QComboBox();
    iconCombo->addItems({"Papirus-Dark", "Candy-icons", "Tela-circle"});
    formLayout->addRow("Icon Theme:", iconCombo);

    QComboBox *cursorCombo = new QComboBox();
    cursorCombo->addItems({"Nordzy-cursors", "Dracula-cursors"});
    formLayout->addRow("Cursor Theme:", cursorCombo);

    layout->addWidget(groupBox);

    QPushButton *applyBtn = new QPushButton("Apply Custom Settings");
    applyBtn->setMinimumHeight(40);
    layout->addWidget(applyBtn);

    layout->addStretch();
}

// --- WallpaperPage ---
WallpaperPage::WallpaperPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *title = new QLabel("Wallpaper Settings");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);

    QLabel *desc = new QLabel("Choose your background.");
    layout->addWidget(desc);
    layout->addSpacing(20);

    QPushButton *selectBtn = new QPushButton("Select Wallpaper...");
    selectBtn->setMinimumHeight(40);
    layout->addWidget(selectBtn);

    layout->addStretch();
}

// --- SystemPage ---
SystemPage::SystemPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QLabel *title = new QLabel("System Commands");
    title->setStyleSheet("font-size: 20px; font-weight: bold;");
    layout->addWidget(title);
    layout->addSpacing(20);

    QPushButton *reloadLabwc = new QPushButton("Reload Labwc (Window Manager)");
    reloadLabwc->setMinimumHeight(40);
    QObject::connect(reloadLabwc, &QPushButton::clicked, [](){
        executeCommand("labwc", {"-r"});
    });
    layout->addWidget(reloadLabwc);

    QPushButton *reloadPanel = new QPushButton("Restart LXQt Panel");
    reloadPanel->setMinimumHeight(40);
    QObject::connect(reloadPanel, &QPushButton::clicked, [](){
        executeCommand("pkill", {"-x", "lxqt-panel"});
        executeCommand("nohup", {"lxqt-panel", "&"});
    });
    layout->addWidget(reloadPanel);

    layout->addStretch();
}
