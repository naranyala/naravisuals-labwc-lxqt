#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QStringList>

struct GlobalTheme {
    QString name;
    QString gtkTheme;
    QString iconTheme;
    QString cursorTheme;
    QString lxqtTheme;
    QString labwcTheme;
};

void executeCommand(const QString &program, const QStringList &arguments);
void applyGlobalTheme(const GlobalTheme &t);

#endif // THEMEMANAGER_H
