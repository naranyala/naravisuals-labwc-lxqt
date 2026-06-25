#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QProcess>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QGroupBox>

const QString WORKSPACE_DIR = "/media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt";

QString findScript(const QString &name) {
    QStringList searchDirs = {
        WORKSPACE_DIR,
        WORKSPACE_DIR + "/scripts",
        WORKSPACE_DIR + "/scripts/launchers",
        WORKSPACE_DIR + "/scripts/fixes",
        WORKSPACE_DIR + "/scripts/installers",
        WORKSPACE_DIR + "/scripts/themes",
        WORKSPACE_DIR + "/scripts/compositors",
        WORKSPACE_DIR + "/scripts/input",
        WORKSPACE_DIR + "/scripts/desktop",
        WORKSPACE_DIR + "/scripts/panel",
        WORKSPACE_DIR + "/scripts/apps-widgets",
        WORKSPACE_DIR + "/scripts/utils",
        WORKSPACE_DIR + "/scripts/dev",
        WORKSPACE_DIR + "/scripts/features",
    };
    for (const auto &dir : searchDirs) {
        QString path = dir + "/" + name;
        if (QFile::exists(path)) return path;
    }
    return {};
}

void runScript(const QString &scriptName) {
    QString path = findScript(scriptName);
    if (path.isEmpty()) {
        QMessageBox::warning(nullptr, "Error", "Script not found: " + scriptName);
        return;
    }
    QProcess::startDetached("bash", {path});
}

void runTerminalScript(const QString &scriptName) {
    QString path = findScript(scriptName);
    if (path.isEmpty()) {
        QMessageBox::warning(nullptr, "Error", "Script not found: " + scriptName);
        return;
    }
    // Launch in xterm so user can type sudo password if needed
    QString cmd = QString("xterm -bg '#1e1e2e' -fg '#cdd6f4' -T '%1' -e bash -c '\"%2\"; echo \"\"; echo \"Press Enter to close...\"; read'").arg(scriptName, path);
    QProcess::startDetached("bash", {"-c", cmd});
}

// --- Pages ---

class AppearancePage : public QWidget {
public:
    AppearancePage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        
        auto *sddmGroup = new QGroupBox("SDDM Login Manager");
        auto *sddmLayout = new QVBoxLayout(sddmGroup);
        sddmLayout->setSpacing(10);
        
        auto *themeLayout = new QHBoxLayout();
        themeLayout->addWidget(new QLabel("SDDM Theme:"));
        themeCombo = new QComboBox();
        QDir themesDir("/usr/share/sddm/themes");
        if (themesDir.exists()) themeCombo->addItems(themesDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
        themeLayout->addWidget(themeCombo);
        
        auto *applyThemeBtn = new QPushButton("Apply Theme");
        connect(applyThemeBtn, &QPushButton::clicked, this, [this](){
            QString selected = themeCombo->currentText();
            if (selected.isEmpty()) return;
            QFile file("/tmp/sddm-custom.conf");
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream(&file) << "[Theme]\nCurrent=" << selected << "\n";
                file.close();
                QProcess::execute("pkexec", {"mkdir", "-p", "/etc/sddm.conf.d/"});
                QProcess process;
                process.start("pkexec", {"cp", "/tmp/sddm-custom.conf", "/etc/sddm.conf.d/custom.conf"});
                process.waitForFinished();
                if (process.exitCode() == 0) QMessageBox::information(this, "Success", "SDDM theme updated!");
            }
        });
        themeLayout->addWidget(applyThemeBtn);
        sddmLayout->addLayout(themeLayout);
        layout->addWidget(sddmGroup);

        auto *themeGroup = new QGroupBox("System Appearance Installs");
        auto *tLayout = new QVBoxLayout(themeGroup);
        tLayout->setSpacing(10);
        
        auto *gtkBtn = new QPushButton("Fix GTK Window Controls");
        connect(gtkBtn, &QPushButton::clicked, [](){ runScript("fix-gtk-window-controls.sh"); });
        tLayout->addWidget(gtkBtn);

        auto *sysThemesBtn = new QPushButton("Install System Themes");
        connect(sysThemesBtn, &QPushButton::clicked, [](){ runTerminalScript("themes.sh"); });
        tLayout->addWidget(sysThemesBtn);

        auto *iconsBtn = new QPushButton("Install Icon Packs");
        connect(iconsBtn, &QPushButton::clicked, [](){ runTerminalScript("icons.sh"); });
        tLayout->addWidget(iconsBtn);

        auto *cursorsBtn = new QPushButton("Install Cursor Themes");
        connect(cursorsBtn, &QPushButton::clicked, [](){ runTerminalScript("cursors.sh"); });
        tLayout->addWidget(cursorsBtn);

        auto *fontsBtn = new QPushButton("Install Custom Fonts");
        connect(fontsBtn, &QPushButton::clicked, [](){ runTerminalScript("fonts.sh"); });
        tLayout->addWidget(fontsBtn);

        layout->addWidget(themeGroup);
        layout->addStretch();
    }
private:
    QComboBox *themeCombo;
};

class DesktopPage : public QWidget {
public:
    DesktopPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        
        auto *group = new QGroupBox("Desktop Environments & Window Managers");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *switchBtn = new QPushButton("Switch LXQt Compositor");
        connect(switchBtn, &QPushButton::clicked, [](){ runTerminalScript("switch-lxqt-compositor.sh"); });
        gLayout->addWidget(switchBtn);

        auto *lxqtThemeBtn = new QPushButton("Install LXQt Themes");
        connect(lxqtThemeBtn, &QPushButton::clicked, [](){ runTerminalScript("lxqt-themes.sh"); });
        gLayout->addWidget(lxqtThemeBtn);

        auto *labwcThemeBtn = new QPushButton("Install Labwc Themes");
        connect(labwcThemeBtn, &QPushButton::clicked, [](){ runTerminalScript("labwc-themes.sh"); });
        gLayout->addWidget(labwcThemeBtn);

        layout->addWidget(group);

        auto *panelGroup = new QGroupBox("Panel Management");
        auto *pLayout = new QVBoxLayout(panelGroup);
        pLayout->setSpacing(10);

        auto *resetPanelBtn = new QPushButton("Reset Panel to Stock");
        connect(resetPanelBtn, &QPushButton::clicked, [](){ runTerminalScript("reset-panel.sh"); });
        pLayout->addWidget(resetPanelBtn);

        layout->addWidget(panelGroup);
        layout->addStretch();
    }
};

class WallpaperPage : public QWidget {
public:
    WallpaperPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        auto *group = new QGroupBox("Wallpaper Management");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *sddmWallBtn = new QPushButton("Update SDDM Wallpaper");
        connect(sddmWallBtn, &QPushButton::clicked, [](){ runTerminalScript("update_sddm_wallpaper.sh"); });
        gLayout->addWidget(sddmWallBtn);

        auto *defaultWallBtn = new QPushButton("Set Default Desktop Wallpaper");
        connect(defaultWallBtn, &QPushButton::clicked, [](){ runTerminalScript("set_default_wallpaper.sh"); });
        gLayout->addWidget(defaultWallBtn);

        auto *wallpacksBtn = new QPushButton("Download Extra Wallpapers");
        connect(wallpacksBtn, &QPushButton::clicked, [](){ runTerminalScript("wallpapers.sh"); });
        gLayout->addWidget(wallpacksBtn);

        layout->addWidget(group);
        layout->addStretch();
    }
};

class AddonsPage : public QWidget {
public:
    AddonsPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        
        auto *group = new QGroupBox("Widgets & Applications");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *conkyBtn = new QPushButton("Install & Setup Conky");
        connect(conkyBtn, &QPushButton::clicked, [](){ runTerminalScript("conky.sh"); });
        gLayout->addWidget(conkyBtn);

        auto *neoBtn = new QPushButton("Install Neofetch");
        connect(neoBtn, &QPushButton::clicked, [](){ runTerminalScript("neofetch.sh"); });
        gLayout->addWidget(neoBtn);

        auto *emacsBtn = new QPushButton("Install Emacs Config");
        connect(emacsBtn, &QPushButton::clicked, [](){ runTerminalScript("emacs.sh"); });
        gLayout->addWidget(emacsBtn);

        layout->addWidget(group);

        auto *diskGroup = new QGroupBox("Disk Management");
        auto *dLayout = new QVBoxLayout(diskGroup);
        dLayout->setSpacing(10);

        auto *ntfsBtn = new QPushButton("NTFS Partition Manager");
        connect(ntfsBtn, &QPushButton::clicked, [](){
            QString path = WORKSPACE_DIR + "/apps/ntfs-gui/build/nv-ntfs-gui";
            if (QFile::exists(path)) {
                QProcess::startDetached(path);
            } else {
                QMessageBox::warning(nullptr, "Error", "NTFS GUI not built. Run: cd apps/ntfs-gui/build && ninja");
            }
        });
        dLayout->addWidget(ntfsBtn);

        layout->addWidget(diskGroup);

        auto *updateGroup = new QGroupBox("System Maintenance");
        auto *uLayout = new QVBoxLayout(updateGroup);
        auto *updatePanelBtn = new QPushButton("Update LXQt Panel from Source");
        connect(updatePanelBtn, &QPushButton::clicked, [](){ runTerminalScript("update_lxqt_panel.sh"); });
        uLayout->addWidget(updatePanelBtn);
        layout->addWidget(updateGroup);

        layout->addStretch();
    }
};

class InputPage : public QWidget {
public:
    InputPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        auto *group = new QGroupBox("Mouse & Touchpad Settings");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *scrollBtn = new QPushButton("Enable Natural Scrolling");
        connect(scrollBtn, &QPushButton::clicked, [](){ runScript("enable-natural-scrolling.sh"); });
        gLayout->addWidget(scrollBtn);

        auto *tapBtn = new QPushButton("Enable Touchpad Tap-to-Click");
        connect(tapBtn, &QPushButton::clicked, [](){ runScript("enable-touchpad-tap.sh"); });
        gLayout->addWidget(tapBtn);

        layout->addWidget(group);
        layout->addStretch();
    }
};

class CompositorPage : public QWidget {
public:
    CompositorPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);

        auto *group = new QGroupBox("Wayland Compositors");
        auto *gLayout = new QVBoxLayout(group);
        gLayout->setSpacing(10);

        auto *hyprlandBtn = new QPushButton("Install Hyprland");
        connect(hyprlandBtn, &QPushButton::clicked, [](){ runTerminalScript("features/install-compositor.sh hyprland"); });
        gLayout->addWidget(hyprlandBtn);

        auto *swayBtn = new QPushButton("Install Sway");
        connect(swayBtn, &QPushButton::clicked, [](){ runTerminalScript("features/install-compositor.sh sway"); });
        gLayout->addWidget(swayBtn);

        auto *wayfireBtn = new QPushButton("Install Wayfire");
        connect(wayfireBtn, &QPushButton::clicked, [](){ runTerminalScript("features/install-compositor.sh wayfire"); });
        gLayout->addWidget(wayfireBtn);

        layout->addWidget(group);

        auto *switchGroup = new QGroupBox("Switch Active Compositor");
        auto *sLayout = new QVBoxLayout(switchGroup);
        sLayout->setSpacing(10);

        auto *switchBtn = new QPushButton("Switch Compositor");
        connect(switchBtn, &QPushButton::clicked, [](){ runTerminalScript("switch-lxqt-compositor.sh"); });
        sLayout->addWidget(switchBtn);

        layout->addWidget(switchGroup);
        layout->addStretch();
    }
};

class ThemingPage : public QWidget {
public:
    ThemingPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);

        auto *kvantumGroup = new QGroupBox("Kvantum Theme Engine");
        auto *kLayout = new QVBoxLayout(kvantumGroup);
        kLayout->setSpacing(10);

        auto *kvantumBtn = new QPushButton("Install Kvantum + Qt6CT");
        connect(kvantumBtn, &QPushButton::clicked, [](){ runTerminalScript("features/install-kvantum.sh"); });
        kLayout->addWidget(kvantumBtn);

        layout->addWidget(kvantumGroup);

        auto *wallustGroup = new QGroupBox("Dynamic Theming (Wallust)");
        auto *wLayout = new QVBoxLayout(wallustGroup);
        wLayout->setSpacing(10);

        auto *wallustBtn = new QPushButton("Install Wallust");
        connect(wallustBtn, &QPushButton::clicked, [](){ runTerminalScript("features/wallust-setup.sh"); });
        wLayout->addWidget(wallustBtn);

        auto *applyBtn = new QPushButton("Apply Colors to Current Wallpaper");
        connect(applyBtn, &QPushButton::clicked, [](){ runTerminalScript("features/wallust-setup.sh --apply"); });
        wLayout->addWidget(applyBtn);

        layout->addWidget(wallustGroup);

        auto *panelGroup = new QGroupBox("Panel Styling");
        auto *pLayout = new QVBoxLayout(panelGroup);
        pLayout->setSpacing(10);

        auto *resetPanelBtn = new QPushButton("Reset Panel to Stock");
        connect(resetPanelBtn, &QPushButton::clicked, [](){ runTerminalScript("reset-panel.sh"); });
        pLayout->addWidget(resetPanelBtn);

        layout->addWidget(panelGroup);
        layout->addStretch();
    }
};

class SystemPage : public QWidget {
public:
    SystemPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);

        auto *portalGroup = new QGroupBox("Portals & Permissions");
        auto *ptLayout = new QVBoxLayout(portalGroup);
        ptLayout->setSpacing(10);

        auto *portalBtn = new QPushButton("Setup XDG Portals");
        connect(portalBtn, &QPushButton::clicked, [](){ runTerminalScript("features/setup-portals.sh"); });
        ptLayout->addWidget(portalBtn);

        auto *flatpakPermBtn = new QPushButton("List Flatpak Permissions");
        connect(flatpakPermBtn, &QPushButton::clicked, [](){ runTerminalScript("features/list-flatpak-permissions.sh"); });
        ptLayout->addWidget(flatpakPermBtn);

        layout->addWidget(portalGroup);

        auto *fedoraGroup = new QGroupBox("Fedora / RHEL Support");
        auto *fLayout = new QVBoxLayout(fedoraGroup);
        fLayout->setSpacing(10);

        auto *fedoraBtn = new QPushButton("Setup Fedora Repos");
        connect(fedoraBtn, &QPushButton::clicked, [](){ runTerminalScript("features/setup-fedora-repos.sh"); });
        fLayout->addWidget(fedoraBtn);

        layout->addWidget(fedoraGroup);

        auto *nightLightGroup = new QGroupBox("Night Light");
        auto *nLayout = new QVBoxLayout(nightLightGroup);
        nLayout->setSpacing(10);

        auto *nightOnBtn = new QPushButton("Enable Night Light");
        connect(nightOnBtn, &QPushButton::clicked, [](){ runTerminalScript("features/night-light.sh on"); });
        nLayout->addWidget(nightOnBtn);

        auto *nightOffBtn = new QPushButton("Disable Night Light");
        connect(nightOffBtn, &QPushButton::clicked, [](){ runTerminalScript("features/night-light.sh off"); });
        nLayout->addWidget(nightOffBtn);

        auto *nightStatusBtn = new QPushButton("Check Night Light Status");
        connect(nightStatusBtn, &QPushButton::clicked, [](){ runTerminalScript("features/night-light.sh status"); });
        nLayout->addWidget(nightStatusBtn);

        layout->addWidget(nightLightGroup);

        auto *maintenanceGroup = new QGroupBox("Maintenance");
        auto *mLayout = new QVBoxLayout(maintenanceGroup);
        mLayout->setSpacing(10);

        auto *updatePanelBtn = new QPushButton("Update LXQt Panel from Source");
        connect(updatePanelBtn, &QPushButton::clicked, [](){ runTerminalScript("update_lxqt_panel.sh"); });
        mLayout->addWidget(updatePanelBtn);

        layout->addWidget(maintenanceGroup);
        layout->addStretch();
    }
};

// --- Main Window ---

class ControlCenterWindow : public QMainWindow {
public:
    ControlCenterWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Naravisuals Control Center");
        resize(850, 600);

        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        auto *mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        sidebar = new QListWidget();
        sidebar->setFixedWidth(240);
        sidebar->addItem("Appearance");
        sidebar->addItem("Desktop & WM");
        sidebar->addItem("Wallpapers");
        sidebar->addItem("Addons & Apps");
        sidebar->addItem("Input Devices");
        sidebar->addItem("Compositors");
        sidebar->addItem("Theming");
        sidebar->addItem("System");
        
        sidebar->setStyleSheet(
            "QListWidget { background-color: #11111b; color: #a6adc8; border: none; font-size: 16px; }"
            "QListWidget::item { padding: 20px 15px; border-bottom: 1px solid #181825; }"
            "QListWidget::item:selected { background-color: #313244; color: #89b4fa; border-left: 4px solid #89b4fa; font-weight: bold; }"
            "QListWidget::item:hover:!selected { background-color: #1e1e2e; }"
        );

        pages = new QStackedWidget();
        pages->setContentsMargins(25, 25, 25, 25);
        pages->addWidget(new AppearancePage());
        pages->addWidget(new DesktopPage());
        pages->addWidget(new WallpaperPage());
        pages->addWidget(new AddonsPage());
        pages->addWidget(new InputPage());
        pages->addWidget(new CompositorPage());
        pages->addWidget(new ThemingPage());
        pages->addWidget(new SystemPage());

        mainLayout->addWidget(sidebar);
        mainLayout->addWidget(pages);

        connect(sidebar, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
        sidebar->setCurrentRow(0);
    }

private:
    QListWidget *sidebar;
    QStackedWidget *pages;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--run-script" && i + 1 < argc) {
            runScript(argv[++i]);
            return 0;
        } else if (arg == "--run-terminal-script" && i + 1 < argc) {
            runTerminalScript(argv[++i]);
            return 0;
        }
    }
    
    app.setStyleSheet(
        "QMainWindow { background-color: #1e1e2e; color: #cdd6f4; }"
        "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
        "QGroupBox { font-weight: bold; font-size: 16px; border: 1px solid #45475a; border-radius: 8px; margin-top: 20px; padding-top: 20px; padding-left: 10px; padding-right: 10px; padding-bottom: 15px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 5px; color: #89b4fa; }"
        "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
        "QPushButton:hover { background-color: #b4befe; }"
        "QPushButton:pressed { background-color: #74c7ec; }"
        "QComboBox { background-color: #313244; color: #cdd6f4; padding: 10px; border: 1px solid #45475a; border-radius: 6px; font-size: 14px; }"
        "QComboBox::drop-down { border: none; }"
        "QLabel { font-size: 14px; }"
        "QMessageBox { background-color: #1e1e2e; color: #cdd6f4; }"
        "QMessageBox QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 8px 15px; }"
        "QScrollBar:vertical { background: #11111b; width: 8px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: #313244; min-height: 20px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background: #45475a; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
    );
    
    ControlCenterWindow window;
    window.show();
    return app.exec();
}
