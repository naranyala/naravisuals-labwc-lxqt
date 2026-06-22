#include "ThemeManager.h"
#include <QProcess>
#include <QDebug>
#include <QDir>

void executeCommand(const QString &program, const QStringList &arguments) {
    QProcess process;
    process.start(program, arguments);
    process.waitForFinished(-1);
}

void applyGlobalTheme(const GlobalTheme &t) {
    QString home = QDir::homePath();
    qDebug() << "Applying theme:" << t.name;

    // 1. Apply GTK Theme
    QString gtkSettings = home + "/.config/gtk-3.0/settings.ini";
    executeCommand("sed", {"-i", "s/gtk-theme-name=.*/gtk-theme-name=" + t.gtkTheme + "/", gtkSettings});
    executeCommand("sed", {"-i", "s/gtk-icon-theme-name=.*/gtk-icon-theme-name=" + t.iconTheme + "/", gtkSettings});
    executeCommand("sed", {"-i", "s/gtk-cursor-theme-name=.*/gtk-cursor-theme-name=" + t.cursorTheme + "/", gtkSettings});

    // 2. Apply LXQt Theme
    QString lxqtConf = home + "/.config/lxqt/lxqt.conf";
    executeCommand("sed", {"-i", "s/^theme=.*/theme=" + t.lxqtTheme + "/", lxqtConf});
    executeCommand("sed", {"-i", "s/^icon_theme=.*/icon_theme=" + t.iconTheme + "/", lxqtConf});

    // 3. Apply Labwc Theme
    QString labwcRC = home + "/.config/labwc/rc.xml";
    executeCommand("sed", {"-i", "s|<name>.*</name>|<name>" + t.labwcTheme + "</name>|", labwcRC});

    // 4. Reload components
    executeCommand("labwc", {"-r"});
    executeCommand("pkill", {"-x", "lxqt-panel"});
    executeCommand("nohup", {"lxqt-panel", "&"});
}
