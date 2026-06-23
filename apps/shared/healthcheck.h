#ifndef HEALTHCHECK_H
#define HEALTHCHECK_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QRegularExpression>
#include <QTextStream>

struct HealthIssue {
    QString id;
    QString category;
    QString title;
    QString description;
    QString fixCommand;
    QString fixDescription;
    bool autoFixable;
    bool detected;
};

class HealthChecker : public QObject {
    Q_OBJECT

public:
    explicit HealthChecker(QObject *parent = nullptr) : QObject(parent) {}

    virtual QList<HealthIssue> runCheck() {
        issues.clear();
        checkFileExists("fontconfig-conf", "Config",
                        "Fontconfig user config",
                        QDir::homePath() + "/.config/fontconfig/fonts.conf",
                        "Deploy fontconfig config", true);
        checkFileExists("labwc-environment", "Config",
                        "Labwc environment file",
                        QDir::homePath() + "/.config/labwc/environment",
                        "", false);
        checkFileExists("labwc-rc", "Config",
                        "Labwc keybindings",
                        QDir::homePath() + "/.config/labwc/rc.xml",
                        "", false);
        checkFileExists("labwc-autostart", "Config",
                        "Labwc autostart",
                        QDir::homePath() + "/.config/labwc/autostart",
                        "", false);
        checkFileExists("lxqt-session", "Config",
                        "LXQt session config",
                        QDir::homePath() + "/.config/lxqt/session.conf",
                        "", false);
        checkFileExists("gtk3-settings", "Config",
                        "GTK3 settings",
                        QDir::homePath() + "/.config/gtk-3.0/settings.ini",
                        "", false);
        checkFileExists("qt6ct-conf", "Config",
                        "Qt6CT config",
                        QDir::homePath() + "/.config/qt6ct/qt6ct.conf",
                        "", false);
        return issues;
    }

    bool fixIssue(const QString &issueId) {
        for (auto &issue : issues) {
            if (issue.id == issueId && issue.autoFixable) {
                return executeFix(issue);
            }
        }
        return false;
    }

    QList<HealthIssue> issues;

protected:
    void addIssue(const QString &id, const QString &cat, const QString &title,
                  const QString &desc, const QString &fixCmd, const QString &fixDesc,
                  bool autoFix, bool detected) {
        issues.append({id, cat, title, desc, fixCmd, fixDesc, autoFix, detected});
    }

    void checkFileExists(const QString &id, const QString &cat, const QString &title,
                         const QString &path, const QString &fixDesc, bool autoFix) {
        bool exists = QFile::exists(path);
        QString desc = exists
            ? QString("Found: %1").arg(path)
            : QString("Missing: %1").arg(path);
        addIssue(id, cat, title, desc, "", fixDesc, autoFix, !exists);
    }

    void checkBinaryExists(const QString &id, const QString &cat, const QString &title,
                           const QString &binary, const QString &fixCmd, const QString &fixDesc) {
        QProcess proc;
        proc.start("which", {binary});
        proc.waitForFinished(3000);
        bool found = (proc.exitCode() == 0);
        QString desc = found
            ? QString("Found: %1").arg(QString(proc.readAllStandardOutput().trimmed()))
            : QString("Not found: %1").arg(binary);
        addIssue(id, cat, title, desc, fixCmd, fixDesc, !fixCmd.isEmpty(), !found);
    }

    void checkEnvVar(const QString &id, const QString &category,
                     const QString &varName, const QString &expectedValue,
                     const QString &envFile) {
        QFile file(envFile);
        QString content;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(file.readAll());
            file.close();
        }

        QRegularExpression re(QString("^%1=(.+)$").arg(varName), QRegularExpression::MultilineOption);
        QRegularExpressionMatch match = re.match(content);
        bool ok = match.hasMatch() && match.captured(1).trimmed() == expectedValue;

        QString desc = ok
            ? QString("%1=%2").arg(varName, expectedValue)
            : QString("%1 not set or wrong value").arg(varName);

        QString fixCmd;
        if (!ok && !envFile.isEmpty()) {
            fixCmd = QString("echo '%1=%2' >> '%3'").arg(varName, expectedValue, envFile);
        }

        addIssue(id, category, QString("Env: %1").arg(varName), desc,
                 fixCmd, QString("Set %1").arg(varName), !fixCmd.isEmpty(), !ok);
    }

    void checkServiceRunning(const QString &id, const QString &title,
                             const QString &service, bool userService = false) {
        // First check if the process is actually running
        QProcess proc;
        proc.start("pgrep", {"-f", service});
        proc.waitForFinished(3000);
        bool running = (proc.exitCode() == 0);

        if (running) {
            addIssue(id, "Services", title,
                     QString("%1 is running").arg(service),
                     "", "", false, false);
            return;
        }

        // Fall back to systemctl check
        QProcess sysctl;
        QStringList args = {"is-active"};
        if (userService) args << "--user";
        args << service;
        sysctl.start("systemctl", args);
        sysctl.waitForFinished(3000);
        bool active = (sysctl.exitCode() == 0);

        QString desc = active
            ? QString("%1 is active via systemd").arg(service)
            : QString("%1 is NOT running").arg(service);

        addIssue(id, "Services", title, desc,
                 QString("systemctl %1 start %2").arg(userService ? "--user" : "", service),
                 QString("Start %1").arg(service), true, !active);
    }

    bool executeFix(const HealthIssue &issue) {
        if (issue.fixCommand.isEmpty()) return false;

        QProcess proc;
        proc.start("bash", {"-c", issue.fixCommand});
        proc.waitForFinished(30000);
        return proc.exitCode() == 0;
    }

    static QString readFileContent(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();
        return content;
    }

    static bool writeFileContent(const QString &path, const QString &content) {
        QFile file(path);
        QDir().mkpath(QFileInfo(path).absolutePath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
        QTextStream out(&file);
        out << content;
        file.close();
        return true;
    }
};

// ---- Specialized Checkers ----

class EnvHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit EnvHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();
        QString envFile = QDir::homePath() + "/.config/labwc/environment";

        // Critical env vars for LXQt+labwc
        checkEnvVar("env-xdg-desktop", "Environment", "XDG_CURRENT_DESKTOP", "LXQt:labwc:wlroots", envFile);
        checkEnvVar("env-xdg-session", "Environment", "XDG_SESSION_TYPE", "wayland", envFile);
        checkEnvVar("env-qt-platform", "Environment", "QT_QPA_PLATFORMTHEME", "qt6ct", envFile);
        checkEnvVar("env-gtk-backend", "Environment", "GDK_BACKEND", "wayland,x11", envFile);
        checkEnvVar("env-moz-wayland", "Environment", "MOZ_ENABLE_WAYLAND", "1", envFile);
        checkEnvVar("env-electron-wayland", "Environment", "ELECTRON_ENABLE_WAYLAND", "1", envFile);
        checkEnvVar("env-cursor-theme", "Environment", "XCURSOR_THEME", "Bibata-Modern-Ice", envFile);
        checkEnvVar("env-cursor-size", "Environment", "XCURSOR_SIZE", "24", envFile);
        checkEnvVar("env-gtk-portal", "Environment", "GTK_USE_PORTAL", "1", envFile);

        // Font rendering vars
        checkEnvVar("env-font-hint", "Font Rendering", "GDK_FONTCONFIG_HINT", "1", envFile);
        checkEnvVar("env-font-render", "Font Rendering", "GDK_RENDERING", "image", envFile);
        checkEnvVar("env-pango-backend", "Font Rendering", "PANGOCAIRO_BACKEND", "fontconfig", envFile);

        return issues;
    }
};

class ServiceHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit ServiceHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();

        // Critical user services
        checkServiceRunning("svc-lxqt-panel", "LXQt Panel", "lxqt-panel", true);
        checkServiceRunning("svc-lxqt-notifd", "LXQt Notifications", "lxqt-notificationd", true);
        checkServiceRunning("svc-lxqt-policykit", "LXQt PolicyKit", "lxqt-policykit-agent", true);

        // Check if labwc is running
        checkBinaryExists("svc-labwc", "Services", "Labwc compositor", "labwc", "", "");

        // Check if key services are running
        checkBinaryExists("svc-pipewire", "Services", "PipeWire audio", "pipewire", "sudo apt install -y pipewire", "Install PipeWire");
        checkBinaryExists("svc-dbus", "Services", "D-Bus session", "dbus-daemon", "", "");

        return issues;
    }
};

class WmHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit WmHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();

        // Labwc config validation
        QString rcPath = QDir::homePath() + "/.config/labwc/rc.xml";
        if (QFile::exists(rcPath)) {
            QString content = readFileContent(rcPath);
            bool hasTheme = content.contains("<name>");
            bool hasKeybinds = content.contains("<keybind");
            bool hasDesktops = content.contains("<desktops>");

            addIssue("wm-rc-theme", "Config", "RC theme defined",
                     hasTheme ? "Theme section found" : "No theme defined in rc.xml",
                     "", "", false, !hasTheme);
            addIssue("wm-rc-keybinds", "Config", "Keybindings defined",
                     hasKeybinds ? "Keybindings found" : "No keybindings in rc.xml",
                     "", "", false, !hasKeybinds);
            addIssue("wm-rc-desktops", "Config", "Desktops defined",
                     hasDesktops ? "Desktops configured" : "No desktops in rc.xml",
                     "", "", false, !hasDesktops);

            // Check for duplicate keybinds
            QRegularExpression re("keybind key=\"([^\"]+)\"");
            QRegularExpressionMatchIterator it = re.globalMatch(content);
            QStringList keys;
            while (it.hasNext()) {
                QRegularExpressionMatch m = it.next();
                keys.append(m.captured(1));
            }
            QStringList dupes;
            for (int i = 0; i < keys.size(); i++) {
                for (int j = i + 1; j < keys.size(); j++) {
                    if (keys[i] == keys[j] && !dupes.contains(keys[i])) {
                        dupes.append(keys[i]);
                    }
                }
            }
            if (!dupes.isEmpty()) {
                addIssue("wm-rc-dupes", "Config", "Duplicate keybinds",
                         QString("Duplicate keys: %1").arg(dupes.join(", ")),
                         "", "", false, true);
            }
        }

        // Check theme consistency
        QString themercPath = QDir::homePath() + "/.config/labwc/themerc-override";
        if (!QFile::exists(themercPath)) {
            addIssue("wm-themerc", "Config", "Window theme override",
                     "themerc-override missing — using default theme",
                     "", "", false, true);
        }

        // Check binary
        checkBinaryExists("wm-labwc", "System", "Labwc installed", "labwc",
                          "sudo apt install -y labwc", "Install labwc");

        return issues;
    }
};

class ThemeHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit ThemeHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();

        // GTK theme consistency
        QString gtk3 = QDir::homePath() + "/.config/gtk-3.0/settings.ini";
        QString gtk4 = QDir::homePath() + "/.config/gtk-4.0/settings.ini";

        if (QFile::exists(gtk3) && QFile::exists(gtk4)) {
            QString c3 = readFileContent(gtk3);
            QString c4 = readFileContent(gtk4);

            QRegularExpression re3("gtk-theme-name=(.+)");
            QRegularExpression re4("gtk-theme-name=(.+)");
            QRegularExpressionMatch m3 = re3.match(c3);
            QRegularExpressionMatch m4 = re4.match(c4);

            if (m3.hasMatch() && m4.hasMatch()) {
                bool match = m3.captured(1).trimmed() == m4.captured(1).trimmed();
                addIssue("theme-gtk-match", "Consistency", "GTK3/GTK4 theme match",
                         match
                             ? QString("Both use: %1").arg(m3.captured(1).trimmed())
                             : QString("GTK3=%1, GTK4=%2").arg(m3.captured(1).trimmed(), m4.captured(1).trimmed()),
                         "", "", false, !match);
            }

            // Check cursor consistency
            QRegularExpression cre3("gtk-cursor-theme-name=(.+)");
            QRegularExpression cre4("gtk-cursor-theme-name=(.+)");
            QRegularExpressionMatch cm3 = cre3.match(c3);
            QRegularExpressionMatch cm4 = cre4.match(c4);

            if (cm3.hasMatch() && cm4.hasMatch()) {
                bool match = cm3.captured(1).trimmed() == cm4.captured(1).trimmed();
                addIssue("theme-cursor-match", "Consistency", "GTK3/GTK4 cursor match",
                         match
                             ? QString("Both use: %1").arg(cm3.captured(1).trimmed())
                             : QString("GTK3=%1, GTK4=%2").arg(cm3.captured(1).trimmed(), cm4.captured(1).trimmed()),
                         "", "", false, !match);
            }
        }

        // Check icon theme exists
        QString iconTheme;
        if (QFile::exists(gtk3)) {
            QRegularExpression re("gtk-icon-theme-name=(.+)");
            QRegularExpressionMatch m = re.match(readFileContent(gtk3));
            if (m.hasMatch()) iconTheme = m.captured(1).trimmed();
        }

        if (!iconTheme.isEmpty()) {
            QStringList searchPaths = {
                QDir::homePath() + "/.icons/" + iconTheme,
                QDir::homePath() + "/.local/share/icons/" + iconTheme,
                "/usr/share/icons/" + iconTheme
            };
            bool found = false;
            for (const QString &p : searchPaths) {
                if (QDir(p).exists()) { found = true; break; }
            }
            addIssue("theme-icons-exist", "System", "Icon theme installed",
                     found ? QString("Found: %1").arg(iconTheme) : QString("Missing: %1").arg(iconTheme),
                     "sudo apt install -y papirus-icon-theme", "Install Papirus icons", !found, !found);
        }

        // Check cursor theme exists
        QString cursorTheme;
        if (QFile::exists(gtk3)) {
            QRegularExpression re("gtk-cursor-theme-name=(.+)");
            QRegularExpressionMatch m = re.match(readFileContent(gtk3));
            if (m.hasMatch()) cursorTheme = m.captured(1).trimmed();
        }

        if (!cursorTheme.isEmpty()) {
            QStringList searchPaths = {
                QDir::homePath() + "/.icons/" + cursorTheme,
                "/usr/share/icons/" + cursorTheme
            };
            bool found = false;
            for (const QString &p : searchPaths) {
                if (QDir(p).exists()) { found = true; break; }
            }
            addIssue("theme-cursor-exist", "System", "Cursor theme installed",
                     found ? QString("Found: %1").arg(cursorTheme) : QString("Missing: %1").arg(cursorTheme),
                     "", "", false, !found);
        }

        // Check Kvantum
        QString kvantumConf = QDir::homePath() + "/.config/Kvantum/kvantum.conf";
        if (QFile::exists(kvantumConf)) {
            addIssue("theme-kvantum", "Config", "Kvantum config",
                     "Kvantum config found", "", "", false, false);
        }

        // Check Qt6CT
        QString qt6ctConf = QDir::homePath() + "/.config/qt6ct/qt6ct.conf";
        if (QFile::exists(qt6ctConf)) {
            QString content = readFileContent(qt6ctConf);
            QRegularExpression re("style=(.+)");
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                addIssue("theme-qt6ct-style", "Config", "Qt6CT style",
                         QString("Style: %1").arg(m.captured(1).trimmed()),
                         "", "", false, false);
            }
        }

        return issues;
    }
};

class NetworkHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit NetworkHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();

        // Check network connectivity
        QProcess proc;
        proc.start("ping", {"-c1", "-W3", "8.8.8.8"});
        proc.waitForFinished(5000);
        bool online = (proc.exitCode() == 0);
        addIssue("net-connectivity", "System", "Internet connectivity",
                 online ? "Connected to internet" : "No internet connection",
                 "", "", false, !online);

        // Check NetworkManager
        checkBinaryExists("net-nm", "System", "NetworkManager", "nmcli",
                          "sudo apt install -y network-manager", "Install NetworkManager");

        // Check nm-applet
        checkBinaryExists("net-nm-applet", "Tray", "Network tray applet", "nm-applet",
                          "sudo apt install -y network-manager-gnome", "Install nm-applet");

        // Check DNS resolution
        QProcess dns;
        dns.start("host", {"-W3", "google.com"});
        dns.waitForFinished(5000);
        bool dnsOk = (dns.exitCode() == 0);
        addIssue("net-dns", "System", "DNS resolution",
                 dnsOk ? "DNS working" : "DNS resolution failed",
                 "", "", false, !dnsOk);

        return issues;
    }
};

class AudioHealthChecker : public HealthChecker {
    Q_OBJECT
public:
    explicit AudioHealthChecker(QObject *parent = nullptr) : HealthChecker(parent) {}

    QList<HealthIssue> runCheck() override {
        issues.clear();

        checkBinaryExists("audio-pipewire", "System", "PipeWire", "pipewire",
                          "sudo apt install -y pipewire", "Install PipeWire");
        checkBinaryExists("audio-pulse", "System", "PulseAudio (PipeWire-Pulse)", "pactl",
                          "sudo apt install -y pipewire-pulse", "Install PipeWire-Pulse");
        checkBinaryExists("audio-wireplumber", "System", "WirePlumber", "wireplumber",
                          "sudo apt install -y wireplumber", "Install WirePlumber");
        checkBinaryExists("audio-playerctl", "Tools", "Media player control", "playerctl",
                          "sudo apt install -y playerctl", "Install playerctl");

        // Check if PipeWire is running
        QProcess proc;
        proc.start("pgrep", {"-x", "pipewire"});
        proc.waitForFinished(3000);
        bool running = (proc.exitCode() == 0);
        addIssue("audio-running", "System", "PipeWire process",
                 running ? "PipeWire is running" : "PipeWire is NOT running",
                 "systemctl --user start pipewire", "Start PipeWire", true, !running);

        return issues;
    }
};

#endif // HEALTHCHECK_H
