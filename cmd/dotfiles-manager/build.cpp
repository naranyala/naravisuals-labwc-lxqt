#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>

namespace fs = std::filesystem;

struct Dotfile {
    std::string srcPath;
    std::string dstPath;
};

// Re-mapped to the new 'configs/' directory structure
std::vector<Dotfile> manifest = {
    {"configs/dotfiles/lxqt/session.conf", ".config/lxqt/session.conf"},
    {"configs/dotfiles/lxqt/panel.conf", ".config/lxqt/panel.conf"},
    {"configs/dotfiles/lxqt/lxqt.conf", ".config/lxqt/lxqt.conf"},
    {"configs/dotfiles/lxqt/lxqt-config.conf", ".config/lxqt/lxqt-config.conf"},
    {"configs/dotfiles/lxqt/lxqt-powermanagement.conf", ".config/lxqt/lxqt-powermanagement.conf"},
    {"configs/dotfiles/lxqt/lxqt-runner.conf", ".config/lxqt/lxqt-runner.conf"},
    {"configs/dotfiles/lxqt/lxqt-notificationd.conf", ".config/lxqt/lxqt-notificationd.conf"},
    {"configs/dotfiles/lxqt/globalkeyshortcuts.conf", ".config/lxqt/globalkeyshortcuts.conf"},
    {"configs/dotfiles/labwc/rc.xml", ".config/labwc/rc.xml"},
    {"configs/dotfiles/labwc/menu.xml", ".config/labwc/menu.xml"},
    {"configs/dotfiles/labwc/autostart", ".config/labwc/autostart"},
    {"configs/dotfiles/labwc/environment", ".config/labwc/environment"},
    {"configs/dotfiles/labwc/themerc", ".config/labwc/themerc-override"},
    {"configs/dotfiles/labwc/shutdown", ".config/labwc/shutdown"},
    {"configs/dotfiles/gtk-3.0/settings.ini", ".config/gtk-3.0/settings.ini"},
    {"configs/dotfiles/gtk-4.0/settings.ini", ".config/gtk-4.0/settings.ini"},
    {"configs/dotfiles/qt6ct/qt6ct.conf", ".config/qt6ct/qt6ct.conf"},
    {"configs/dotfiles/pcmanfm-qt/lxqt/settings.conf", ".config/pcmanfm-qt/lxqt/settings.conf"},
    {"configs/dotfiles/qterminal.org/qterminal.ini", ".config/qterminal.org/qterminal.ini"},
    {"configs/dotfiles/user-dirs.dirs", ".config/user-dirs.dirs"},
    {"configs/dotfiles/kanshi/config", ".config/kanshi/config"},
    {"configs/dotfiles/swaylock/config", ".config/swaylock/config"},
    {"configs/dotfiles/dunst/dunstrc", ".config/dunst/dunstrc"},
    {"configs/dotfiles/emacs/init.el", ".config/emacs/init.el"}
};

std::string getHomeDir() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : "";
}

void printUsage() {
    std::cout << "LXQt Labwc Dotfiles Installer (Native C++)\n\n"
              << "Usage:\n"
              << "  ./installer install    Install dotfiles to $HOME\n"
              << "  ./installer list       List all managed dotfiles and their destinations\n";
}

void installFiles() {
    std::string home = getHomeDir();
    if (home.empty()) {
        std::cerr << "Error: HOME environment variable not set.\n";
        return;
    }

    // Attempt to resolve the workspace path assuming the binary is run from the project root
    fs::path workspace = fs::current_path();

    int installed = 0, skipped = 0, failed = 0;

    for (const auto& df : manifest) {
        fs::path src = workspace / df.srcPath;
        fs::path dst = fs::path(home) / df.dstPath;

        if (!fs::exists(src)) {
            std::cerr << "  ERROR: Source file missing: " << src << "\n";
            failed++;
            continue;
        }

        fs::create_directories(dst.parent_path());

        try {
            fs::copy_options opts = fs::copy_options::overwrite_existing;
            fs::copy(src, dst, opts);
            
            // Replicate the Go installer's 0755 mode for scripts
            if (df.srcPath.find("autostart") != std::string::npos || df.srcPath.find("shutdown") != std::string::npos) {
                fs::permissions(dst, fs::perms::owner_all | fs::perms::group_read | fs::perms::others_read);
            }
            
            std::cout << "  INSTALLED " << dst << "\n";
            installed++;
        } catch (fs::filesystem_error& e) {
            std::cerr << "  ERROR installing " << dst << ": " << e.what() << "\n";
            failed++;
        }
    }

    std::cout << "\nSummary: " << installed << " installed, " << skipped << " skipped, " << failed << " failed\n";
    std::cout << "\nNote: System files need to be copied manually using sudo:\n";
    std::cout << "  sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/\n";
    std::cout << "  sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/\n";
}

void listFiles() {
    std::string home = getHomeDir();
    std::cout << "LXQt Labwc Dotfiles Manifest\n";
    std::cout << "============================\n\n";
    for (const auto& df : manifest) {
        std::cout << "  " << df.srcPath << " -> " << home << "/" << df.dstPath << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "install") {
        installFiles();
    } else if (cmd == "list") {
        listFiles();
    } else {
        printUsage();
        return 1;
    }

    return 0;
}
