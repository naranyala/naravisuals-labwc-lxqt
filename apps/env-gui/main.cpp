#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--list") {
            QFile file(QDir::homePath() + "/.config/environment.d/99-labwc.conf");
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    QString line = in.readLine();
                    printf("%s\n", line.toUtf8().constData());
                }
            }
            return 0;
        }
    }

    EnvGui gui;
    gui.show();
    return app.exec();
}
