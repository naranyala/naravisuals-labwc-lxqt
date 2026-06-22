#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "==> Building NaraVisuals Control Center..." << std::endl;
    
    // We compile the files directly using g++ and pkg-config for Qt6.
    // Since we didn't use Q_OBJECT macros, we don't need to run 'moc' beforehand!
    const char* compileCmd = "g++ main.cpp SettingsWindow.cpp Pages.cpp ThemeManager.cpp "
                             "-o NaraVisualsThemeManager "
                             "$(pkg-config --cflags --libs Qt6Widgets Qt6Core Qt6Gui) "
                             "-fPIC";
    
    int result = std::system(compileCmd);
    
    if (result == 0) {
        std::cout << "==> Build successful! Binary generated: NaraVisualsThemeManager" << std::endl;
    } else {
        std::cerr << "==> Build failed!" << std::endl;
    }
    
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
