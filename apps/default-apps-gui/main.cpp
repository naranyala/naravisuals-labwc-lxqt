#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QStatusBar>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>
#include <QMessageBox>
#include <QIcon>
#include <QProcess>
#include <QFileDialog>
#include <QMap>
#include <QGroupBox>
#include <QFrame>
#include <QScrollArea>
#include <algorithm>

struct DesktopApp {
    QString id;          // e.g. "firefox.desktop"
    QString name;        // e.g. "Firefox Web Browser"
    QString exec;
    QString icon;
    QStringList mimeTypes;
    QString filePath;
};

struct AppCategory {
    QString name;
    QString description;
    QStringList mimeTypes;
    QString iconName;
};

const QList<AppCategory> categories = {
    {"Web Browser", "Opens websites and HTML files", {"text/html", "x-scheme-handler/http", "x-scheme-handler/https"}, "browser"},
    {"File Manager", "Explores files and directories", {"inode/directory"}, "system-file-manager"},
    {"Text Editor", "Edits plain text files", {"text/plain"}, "text-editor"},
    {"Email Client", "Sends and receives emails", {"x-scheme-handler/mailto"}, "internet-mail"},
    {"Image Viewer", "Displays pictures and photos", {"image/png", "image/jpeg", "image/gif", "image/webp"}, "image-viewer"},
    {"Video Player", "Plays movies and video clips", {"video/mp4", "video/x-matroska", "video/x-msvideo", "video/quicktime"}, "video-player"},
    {"Audio Player", "Plays music and sound tracks", {"audio/mpeg", "audio/ogg", "audio/wav", "audio/flac"}, "audio-player"},
    {"PDF Reader", "Views PDF documents", {"application/pdf"}, "document-viewer"}
};

const QMap<QString, QStringList> fallbackPrefixes = {
    {"Web Browser", {"firefox", "chrome", "chromium", "brave", "opera", "epiphany", "falkon", "midori"}},
    {"File Manager", {"pcmanfm", "thunar", "nautilus", "dolphin", "nemo", "doublecmd", "gentoo"}},
    {"Text Editor", {"featherpad", "kate", "mousepad", "gedit", "leafpad", "pluma", "kwrite", "code", "emacs", "nvim"}},
    {"Email Client", {"thunderbird", "evolution", "kmail", "geary", "claws-mail"}},
    {"Image Viewer", {"lximage", "viewnior", "eog", "gwenview", "ristretto", "feh", "sxiv"}},
    {"Video Player", {"vlc", "mpv", "smplayer", "totem", "dragonplayer"}},
    {"Audio Player", {"audacious", "clementine", "rhythmbox", "lollypop", "strawberry", "cmus"}},
    {"PDF Reader", {"qpdfview", "evince", "okular", "zathura", "mupdf", "atril"}}
};

QList<DesktopApp> scanInstalledApps() {
    QList<DesktopApp> apps;
    QStringList searchDirs = {
        "/usr/share/applications",
        QDir::homePath() + "/.local/share/applications",
        "/var/lib/flatpak/exports/share/applications"
    };

    for (const auto &dirPath : searchDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        QFileInfoList list = dir.entryInfoList({"*.desktop"}, QDir::Files);
        for (const auto &fileInfo : list) {
            QString filePath = fileInfo.absoluteFilePath();
            QString id = fileInfo.fileName();

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QTextStream in(&file);
            bool inDesktopEntry = false;
            QString name, exec, icon, type;
            bool noDisplay = false;
            QStringList mimeTypes;

            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("[")) {
                    if (line == "[Desktop Entry]") {
                        inDesktopEntry = true;
                    } else {
                        inDesktopEntry = false;
                    }
                    continue;
                }

                if (!inDesktopEntry) continue;

                if (line.startsWith("Name=")) {
                    if (name.isEmpty()) {
                        name = line.mid(5);
                    }
                } else if (line.startsWith("Exec=")) {
                    exec = line.mid(5);
                } else if (line.startsWith("Icon=")) {
                    icon = line.mid(5);
                } else if (line.startsWith("Type=")) {
                    type = line.mid(5);
                } else if (line.startsWith("NoDisplay=")) {
                    noDisplay = (line.mid(10).toLower() == "true");
                } else if (line.startsWith("MimeType=")) {
                    QString mimesStr = line.mid(9);
                    mimeTypes = mimesStr.split(';', Qt::SkipEmptyParts);
                }
            }
            file.close();

            if (type == "Application" && !noDisplay && !name.isEmpty()) {
                bool duplicate = false;
                for (const auto &app : apps) {
                    if (app.id == id) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate) {
                    apps.append({id, name, exec, icon, mimeTypes, filePath});
                }
            }
        }
    }
    return apps;
}

class DefaultAppsGui : public QMainWindow {
    Q_OBJECT

public:
    DefaultAppsGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Default Applications Selector");
        resize(820, 600);

        mimeFile = QDir::homePath() + "/.config/mimeapps.list";
        installedApps = scanInstalledApps();

        setupUI();
        loadCurrentDefaults();
    }

private:
    void setupUI() {
        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(16);
        mainLayout->setContentsMargins(24, 24, 24, 24);

        // Header
        auto *header = new QLabel("Default Applications");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: #89b4fa;");
        mainLayout->addWidget(header);

        auto *subHeader = new QLabel("Choose your preferred applications for various categories of tasks.");
        subHeader->setStyleSheet("font-size: 13px; color: #a6adc8; margin-bottom: 8px;");
        subHeader->setWordWrap(true);
        mainLayout->addWidget(subHeader);

        // Scroll Area
        auto *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: transparent; }");

        auto *scrollWidget = new QWidget();
        scrollWidget->setObjectName("scrollWidget");
        scrollWidget->setStyleSheet("#scrollWidget { background-color: transparent; }");
        auto *scrollLayout = new QVBoxLayout(scrollWidget);
        scrollLayout->setSpacing(12);
        scrollLayout->setContentsMargins(0, 0, 0, 0);

        for (const auto &cat : categories) {
            auto *card = new QFrame();
            card->setObjectName("categoryCard");
            card->setStyleSheet(
                "QFrame#categoryCard { background-color: #181825; border: 1px solid #313244; border-radius: 8px; }"
                "QFrame#categoryCard:hover { border: 1px solid #45475a; }"
            );

            auto *cardLayout = new QHBoxLayout(card);
            cardLayout->setContentsMargins(16, 12, 16, 12);
            cardLayout->setSpacing(16);

            // Icon
            auto *iconLabel = new QLabel();
            iconLabel->setFixedSize(32, 32);
            QIcon catIcon = QIcon::fromTheme(cat.iconName);
            if (catIcon.isNull()) {
                catIcon = QIcon::fromTheme("application-x-executable");
            }
            if (!catIcon.isNull()) {
                iconLabel->setPixmap(catIcon.pixmap(32, 32));
            }
            cardLayout->addWidget(iconLabel);

            // Labels
            auto *textLayout = new QVBoxLayout();
            textLayout->setSpacing(2);

            auto *nameLabel = new QLabel(cat.name);
            nameLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #cdd6f4;");
            textLayout->addWidget(nameLabel);

            auto *descLabel = new QLabel(cat.description);
            descLabel->setStyleSheet("font-size: 12px; color: #a6adc8;");
            textLayout->addWidget(descLabel);

            cardLayout->addLayout(textLayout, 1);

            // ComboBox
            auto *combo = new QComboBox();
            combo->setMinimumWidth(260);
            combo->setMaximumWidth(360);
            combo->setStyleSheet(
                "QComboBox { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 6px 12px; font-size: 13px; }"
                "QComboBox:hover { border: 1px solid #89b4fa; }"
                "QComboBox::drop-down { border: none; width: 20px; }"
                "QComboBox QAbstractItemView { background-color: #1e1e2e; color: #cdd6f4; selection-background-color: #313244; selection-color: #89b4fa; border: 1px solid #45475a; }"
            );
            cardLayout->addWidget(combo);
            categoryCombos[cat.name] = combo;

            populateCategoryCombo(cat, combo);

            // Browse button
            auto *browseBtn = new QPushButton();
            browseBtn->setFixedSize(32, 32);
            browseBtn->setToolTip("Select custom .desktop file...");
            QIcon browseIcon = QIcon::fromTheme("folder-open");
            if (!browseIcon.isNull()) {
                browseBtn->setIcon(browseIcon);
            } else {
                browseBtn->setText("...");
            }
            browseBtn->setStyleSheet(
                "QPushButton { background-color: #313244; border: 1px solid #45475a; border-radius: 6px; color: #cdd6f4; }"
                "QPushButton:hover { background-color: #45475a; border: 1px solid #89b4fa; }"
            );
            connect(browseBtn, &QPushButton::clicked, this, [this, cat, combo]() {
                browseCustomApp(cat, combo);
            });
            cardLayout->addWidget(browseBtn);

            scrollLayout->addWidget(card);
        }

        scrollLayout->addStretch();
        scroll->setWidget(scrollWidget);
        mainLayout->addWidget(scroll, 1);

        // Buttons
        auto *btnLayout = new QHBoxLayout();
        btnLayout->setSpacing(12);

        auto *resetBtn = new QPushButton("Reset");
        resetBtn->setMinimumHeight(38);
        resetBtn->setMinimumWidth(100);
        resetBtn->setStyleSheet(
            "QPushButton { background-color: #f38ba8; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #f5a8c0; }"
        );
        connect(resetBtn, &QPushButton::clicked, this, &DefaultAppsGui::loadCurrentDefaults);
        btnLayout->addWidget(resetBtn);

        btnLayout->addStretch();

        auto *applyBtn = new QPushButton("Apply Changes");
        applyBtn->setMinimumHeight(38);
        applyBtn->setMinimumWidth(160);
        applyBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(applyBtn, &QPushButton::clicked, this, &DefaultAppsGui::applyChanges);
        btnLayout->addWidget(applyBtn);

        mainLayout->addLayout(btnLayout);

        // Status Bar
        statusBar()->setStyleSheet("background-color: #11111b; color: #a6adc8; border-top: 1px solid #313244;");

        // Styling
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 13px; }"
        );
    }

    void populateCategoryCombo(const AppCategory &cat, QComboBox *combo) {
        combo->clear();

        QList<DesktopApp> matches;
        QStringList matchedIds;

        for (const auto &app : installedApps) {
            bool matchesMime = false;
            for (const auto &mime : cat.mimeTypes) {
                if (app.mimeTypes.contains(mime)) {
                    matchesMime = true;
                    break;
                }
            }

            bool matchesPrefix = false;
            QString idLower = app.id.toLower();
            if (fallbackPrefixes.contains(cat.name)) {
                for (const auto &prefix : fallbackPrefixes[cat.name]) {
                    if (idLower.startsWith(prefix)) {
                        matchesPrefix = true;
                        break;
                    }
                }
            }

            if (matchesMime || matchesPrefix) {
                matches.append(app);
                matchedIds.append(app.id);
            }
        }

        std::sort(matches.begin(), matches.end(), [](const DesktopApp &a, const DesktopApp &b) {
            return a.name.localeAwareCompare(b.name) < 0;
        });

        for (const auto &app : matches) {
            QIcon appIcon = QIcon::fromTheme(app.icon);
            if (appIcon.isNull()) {
                appIcon = QIcon::fromTheme("application-x-executable");
            }
            combo->addItem(appIcon, app.name, app.id);
        }

        if (combo->count() == 0) {
            combo->addItem(QIcon::fromTheme("dialog-question"), "No applications found", "");
        }
    }

    void browseCustomApp(const AppCategory &cat, QComboBox *combo) {
        QString desktopPath = QFileDialog::getOpenFileName(
            this,
            "Select Application .desktop File",
            "/usr/share/applications",
            "Desktop Files (*.desktop)"
        );

        if (desktopPath.isEmpty()) return;

        QFileInfo fi(desktopPath);
        QString appId = fi.fileName();

        QFile file(desktopPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QTextStream in(&file);
        bool inDesktopEntry = false;
        QString name = appId;
        QString iconName = "application-x-executable";

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line == "[Desktop Entry]") {
                inDesktopEntry = true;
                continue;
            }
            if (line.startsWith("[") && line != "[Desktop Entry]") {
                inDesktopEntry = false;
                continue;
            }
            if (!inDesktopEntry) continue;

            if (line.startsWith("Name=")) {
                if (name == appId) {
                    name = line.mid(5);
                }
            } else if (line.startsWith("Icon=")) {
                iconName = line.mid(5);
            }
        }
        file.close();

        int index = combo->findData(appId);
        if (index == -1) {
            QIcon appIcon = QIcon::fromTheme(iconName);
            if (appIcon.isNull()) {
                appIcon = QIcon::fromTheme("application-x-executable");
            }
            combo->addItem(appIcon, name + " (Custom)", appId);
            index = combo->count() - 1;
        }
        combo->setCurrentIndex(index);
        statusBar()->showMessage("Selected custom app: " + name);
    }

    void loadCurrentDefaults() {
        QSettings settings(mimeFile, QSettings::IniFormat);
        settings.beginGroup("Default Applications");

        for (const auto &cat : categories) {
            QComboBox *combo = categoryCombos[cat.name];
            if (!combo) continue;

            QString primaryMime = cat.mimeTypes.first();
            QString currentVal = settings.value(primaryMime).toString();
            QString currentAppId = currentVal.split(';', Qt::SkipEmptyParts).value(0).trimmed();

            if (currentAppId.isEmpty()) {
                for (int i = 1; i < cat.mimeTypes.size(); ++i) {
                    currentVal = settings.value(cat.mimeTypes[i]).toString();
                    currentAppId = currentVal.split(';', Qt::SkipEmptyParts).value(0).trimmed();
                    if (!currentAppId.isEmpty()) break;
                }
            }

            if (!currentAppId.isEmpty()) {
                int index = combo->findData(currentAppId);
                if (index != -1) {
                    combo->setCurrentIndex(index);
                } else {
                    QString displayName = currentAppId;
                    QString iconName = "application-x-executable";

                    for (const auto &app : installedApps) {
                        if (app.id == currentAppId) {
                            displayName = app.name;
                            iconName = app.icon;
                            break;
                        }
                    }

                    QIcon appIcon = QIcon::fromTheme(iconName);
                    if (appIcon.isNull()) {
                        appIcon = QIcon::fromTheme("application-x-executable");
                    }
                    combo->addItem(appIcon, displayName + " (Current Default)", currentAppId);
                    combo->setCurrentIndex(combo->count() - 1);
                }
            }
        }

        settings.endGroup();
        statusBar()->showMessage("Loaded current default applications.");
    }

    void applyChanges() {
        QMap<QString, QString> selectedApps;
        QString browserAppId;

        for (const auto &cat : categories) {
            QComboBox *combo = categoryCombos[cat.name];
            if (!combo) continue;

            QString appId = combo->itemData(combo->currentIndex()).toString();
            if (appId.isEmpty()) continue;

            if (cat.name == "Web Browser") {
                browserAppId = appId;
            }

            for (const auto &mime : cat.mimeTypes) {
                selectedApps[mime] = appId;
            }
        }

        if (selectedApps.isEmpty()) {
            QMessageBox::information(this, "Info", "No default applications selected.");
            return;
        }

        // Read file contents except [Default Applications]
        QString beforeSection;
        QString afterSection;
        bool inDefault = false;
        bool foundDefault = false;

        QFile file(mimeFile);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.trimmed().startsWith("[Default Applications]")) {
                    inDefault = true;
                    foundDefault = true;
                    continue;
                }
                if (inDefault && line.trimmed().startsWith("[")) {
                    inDefault = false;
                }

                if (!inDefault) {
                    if (!foundDefault) beforeSection += line + "\n";
                    else afterSection += line + "\n";
                }
            }
            file.close();
        }

        // Load existing defaults first to merge them
        QMap<QString, QString> finalDefaults;
        QSettings settings(mimeFile, QSettings::IniFormat);
        settings.beginGroup("Default Applications");
        for (const QString &key : settings.childKeys()) {
            finalDefaults[key] = settings.value(key).toString();
        }
        settings.endGroup();

        // Merge our selections
        for (auto it = selectedApps.constBegin(); it != selectedApps.constEnd(); ++it) {
            finalDefaults[it.key()] = it.value();
        }

        // Create ~/.config directory if it doesn't exist
        QDir().mkpath(QFileInfo(mimeFile).absolutePath());

        // Save
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            if (!beforeSection.isEmpty()) out << beforeSection;
            out << "[Default Applications]\n";
            for (auto it = finalDefaults.constBegin(); it != finalDefaults.constEnd(); ++it) {
                out << it.key() << "=" << it.value() << "\n";
            }
            if (!afterSection.isEmpty()) out << afterSection;
            file.close();
        } else {
            QMessageBox::critical(this, "Error", "Failed to save defaults to " + mimeFile);
            return;
        }

        // Set default browser with xdg-settings
        if (!browserAppId.isEmpty()) {
            QProcess::startDetached("xdg-settings", {"set", "default-web-browser", browserAppId});
        }

        QMessageBox::information(this, "Success", "Default applications updated successfully!");
        loadCurrentDefaults();
    }

private:
    QString mimeFile;
    QList<DesktopApp> installedApps;
    QMap<QString, QComboBox*> categoryCombos;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("naravisuals");
    app.setApplicationName("default-apps-gui");

    DefaultAppsGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
