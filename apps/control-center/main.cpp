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

void runScript(const QString &scriptName) {
    QString path = WORKSPACE_DIR + "/" + scriptName;
    if (!QFile::exists(path)) path = WORKSPACE_DIR + "/scripts/" + scriptName;
    if (!QFile::exists(path)) {
        QMessageBox::warning(nullptr, "Error", "Script not found: " + scriptName);
        return;
    }
    QProcess::startDetached("bash", {path});
}

void runTerminalScript(const QString &scriptName) {
    QString path = WORKSPACE_DIR + "/" + scriptName;
    if (!QFile::exists(path)) path = WORKSPACE_DIR + "/scripts/" + scriptName;
    if (!QFile::exists(path)) {
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
        connect(scrollBtn, &QPushButton::clicked, [](){ runScript("enable-natural-scrollong.sh"); });
        gLayout->addWidget(scrollBtn);

        auto *tapBtn = new QPushButton("Enable Touchpad Tap-to-Click");
        connect(tapBtn, &QPushButton::clicked, [](){ runScript("enable-touchpad-tap.sh"); });
        gLayout->addWidget(tapBtn);

        layout->addWidget(group);
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
        sidebar->addItem("🎨 Appearance");
        sidebar->addItem("🖥️ Desktop & WM");
        sidebar->addItem("🖼️ Wallpapers");
        sidebar->addItem("⚙️ Addons & Apps");
        sidebar->addItem("🖱️ Input Devices");
        
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
    );
    
    ControlCenterWindow window;
    window.show();
    return app.exec();
}
