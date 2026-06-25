#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

namespace fs = std::filesystem;

struct Dotfile {
    std::string srcPath;
    std::string dstPath;
};

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
    {"configs/dotfiles/labwc/toggle-redshift.sh", ".config/labwc/toggle-redshift.sh"},
    {"configs/dotfiles/gtk-3.0/settings.ini", ".config/gtk-3.0/settings.ini"},
    {"configs/dotfiles/gtk-4.0/settings.ini", ".config/gtk-4.0/settings.ini"},
    {"configs/dotfiles/qt6ct/qt6ct.conf", ".config/qt6ct/qt6ct.conf"},
    {"configs/dotfiles/pcmanfm-qt/lxqt/settings.conf", ".config/pcmanfm-qt/lxqt/settings.conf"},
    {"configs/dotfiles/qterminal.org/qterminal.ini", ".config/qterminal.org/qterminal.ini"},
    {"configs/dotfiles/user-dirs.dirs", ".config/user-dirs.dirs"},
    {"configs/dotfiles/fontconfig/fonts.conf", ".config/fontconfig/fonts.conf"},
    {"configs/dotfiles/kanshi/config", ".config/kanshi/config"},
    {"configs/dotfiles/swaylock/config", ".config/swaylock/config"},
    {"configs/dotfiles/dunst/dunstrc", ".config/dunst/dunstrc"},
    {"configs/dotfiles/cliphist/config", ".config/cliphist/config"},
    {"configs/dotfiles/wob/config", ".config/wob/config"},
    {"configs/dotfiles/rofi/config.rasi", ".config/rofi/config.rasi"},
    {"configs/dotfiles/redshift/redshift.conf", ".config/redshift/redshift.conf"},
    {"configs/dotfiles/xdg-desktop-portal-wlr/config", ".config/xdg-desktop-portal-wlr/config"},
    {"configs/dotfiles/lxqt/lxqt-panel.qss", ".config/lxqt/lxqt-panel.qss"},
    {"configs/dotfiles/lxqt/panel-stock.conf", ".config/lxqt/panel-stock.conf"},
    {"configs/dotfiles/emacs/init.el", ".config/emacs/init.el"},
    {"configs/compositors/hyprland/hyprland.conf", ".config/hypr/hyprland.conf"},
    {"configs/compositors/sway/config", ".config/sway/config"},
    {"configs/compositors/wayfire/wayfire.ini", ".config/wayfire/wayfire.ini"}
};

std::string getHomeDir() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : "";
}

std::string getTimestamp() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", t);
    return std::string(buf);
}

void printUsage() {
    std::cout << "Naravisuals Dotfiles Installer (C++)\n\n"
              << "Usage:\n"
              << "  ./lxqt-dotfiles install         Install dotfiles (backs up existing)\n"
              << "  ./lxqt-dotfiles list            List all managed dotfiles\n"
              << "  ./lxqt-dotfiles restore         Restore from latest backup\n"
              << "  ./lxqt-dotfiles validate        Check manifest vs installed files\n"
              << "\nOptions:\n"
              << "  --force          Overwrite without backup\n"
              << "  --dry-run        Show what would be done\n";
}

fs::path getBackupDir() {
    std::string home = getHomeDir();
    std::string ts = getTimestamp();
    return fs::path(home) / ".config" / "backups" / "naravisuals" / ts;
}

void backupFile(const fs::path& dst, const fs::path& backupDir) {
    if (!fs::exists(dst)) return;
    fs::path rel = dst.lexically_relative(fs::path(getHomeDir()) / ".config");
    fs::path backupPath = backupDir / rel;
    fs::create_directories(backupPath.parent_path());
    fs::copy_file(dst, backupPath, fs::copy_options::overwrite_existing);
}

void installFiles(bool force, bool dryRun) {
    std::string home = getHomeDir();
    if (home.empty()) {
        std::cerr << "Error: HOME environment variable not set.\n";
        return;
    }

    fs::path workspace = fs::current_path();
    fs::path backupDir = getBackupDir();
    bool backedUp = false;

    int installed = 0, skipped = 0, failed = 0, backedUpCount = 0;

    for (const auto& df : manifest) {
        fs::path src = workspace / df.srcPath;
        fs::path dst = fs::path(home) / df.dstPath;

        if (!fs::exists(src)) {
            std::cerr << "  WARN: Source missing: " << df.srcPath << "\n";
            failed++;
            continue;
        }

        if (dryRun) {
            if (fs::exists(dst)) {
                std::cout << "  would update: " << df.dstPath << "\n";
            } else {
                std::cout << "  would create: " << df.dstPath << "\n";
            }
            installed++;
            continue;
        }

        if (fs::exists(dst) && !force) {
            skipped++;
            continue;
        }

        fs::create_directories(dst.parent_path());

        try {
            if (fs::exists(dst) && force) {
                backupFile(dst, backupDir);
                backedUp = true;
                backedUpCount++;
            }

            fs::copy(src, dst, fs::copy_options::overwrite_existing);

            if (df.srcPath.find("autostart") != std::string::npos ||
                df.srcPath.find("shutdown") != std::string::npos) {
                fs::permissions(dst, fs::perms::owner_all | fs::perms::group_read |
                               fs::perms::group_exec | fs::perms::others_read | fs::perms::others_exec);
            }

            std::cout << "  INSTALLED " << df.dstPath << "\n";
            installed++;
        } catch (fs::filesystem_error& e) {
            std::cerr << "  ERROR: " << df.dstPath << ": " << e.what() << "\n";
            failed++;
        }
    }

    std::cout << "\nSummary: " << installed << " installed, " << skipped
              << " skipped, " << backedUpCount << " backed up, " << failed << " failed\n";

    if (backedUp) {
        std::cout << "\nBackups saved to: " << backupDir << "\n";
        std::cout << "Restore with: ./lxqt-dotfiles restore\n";
    }

    std::cout << "\nNote: System files need sudo:\n"
              << "  sudo cp configs/dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/\n"
              << "  sudo cp configs/dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/\n";
}

void restoreFiles() {
    std::string home = getHomeDir();
    fs::path backupBase = fs::path(home) / ".config" / "backups" / "naravisuals";

    if (!fs::exists(backupBase)) {
        std::cerr << "No backups found in " << backupBase << "\n";
        return;
    }

    // Find latest backup directory
    fs::path latest;
    for (const auto& entry : fs::directory_iterator(backupBase)) {
        if (entry.is_directory()) {
            if (latest.empty() || entry.path().filename() > latest.filename()) {
                latest = entry.path();
            }
        }
    }

    if (latest.empty()) {
        std::cerr << "No backup directories found\n";
        return;
    }

    std::cout << "Restoring from: " << latest << "\n";

    int restored = 0;
    for (const auto& entry : fs::recursive_directory_iterator(latest)) {
        if (entry.is_regular_file()) {
            fs::path rel = entry.path().lexically_relative(latest);
            fs::path dst = fs::path(home) / "config" / rel;

            fs::create_directories(dst.parent_path());
            fs::copy_file(entry.path(), dst, fs::copy_options::overwrite_existing);
            std::cout << "  RESTORED " << rel << "\n";
            restored++;
        }
    }

    std::cout << "\nRestored " << restored << " files\n";
}

void listFiles() {
    std::string home = getHomeDir();
    std::cout << "Naravisuals Dotfiles Manifest\n";
    std::cout << "=============================\n\n";
    for (const auto& df : manifest) {
        fs::path dst = fs::path(home) / df.dstPath;
        std::string status = fs::exists(dst) ? "OK" : "MISSING";
        std::cout << "  [" << status << "] " << df.srcPath << "\n"
                  << "         -> " << df.dstPath << "\n";
    }
}

void validateFiles() {
    std::string home = getHomeDir();
    fs::path workspace = fs::current_path();
    int ok = 0, missing = 0, sourceMissing = 0;

    for (const auto& df : manifest) {
        fs::path src = workspace / df.srcPath;
        fs::path dst = fs::path(home) / df.dstPath;

        if (!fs::exists(src)) {
            std::cout << "  WARN  Source missing: " << df.srcPath << "\n";
            sourceMissing++;
            continue;
        }

        if (!fs::exists(dst)) {
            std::cout << "  FAIL  Not installed: " << df.dstPath << "\n";
            missing++;
        } else {
            ok++;
        }
    }

    std::cout << "\nValidation: " << ok << " ok, " << missing
              << " not installed, " << sourceMissing << " source missing\n";

    if (missing > 0) {
        std::cout << "\nRun: ./lxqt-dotfiles install\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    bool force = false;
    bool dryRun = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--force") force = true;
        if (arg == "--dry-run") dryRun = true;
    }

    if (cmd == "install") {
        installFiles(force, dryRun);
    } else if (cmd == "list") {
        listFiles();
    } else if (cmd == "restore") {
        restoreFiles();
    } else if (cmd == "validate") {
        validateFiles();
    } else {
        printUsage();
        return 1;
    }

    return 0;
}
