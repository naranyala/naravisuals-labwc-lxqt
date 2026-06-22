#include <QApplication>
#include "SettingsWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set a modern fusion style
    app.setStyle("Fusion");
    
    SettingsWindow window;
    window.show();
    return app.exec();
}
