#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QProcess>
#include <QStatusBar>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QGridLayout>
#include <QScrollArea>
#include <QRegularExpression>
#include <QFrame>
#include <QListWidget>
#include <QStringListModel>
#include <QSplitter>
#include <QMap>
#include <QFontDialog>
#include <QInputDialog>
#include <QFileInfo>
#include <QTimer>
#include "../shared/healthcheck.h"
#include "../shared/healthcheckwidget.h"

// ============================================================
// Tab 1: Window Decorations (labwc/openbox themes + border/gaps)
// ============================================================
class WmDecorationTab : public QWidget {
    Q_OBJECT
public:
    explicit WmDecorationTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        scanThemes();
    }

signals:
    void changed();

private slots:
    void scanThemes() {
        wmThemeCombo->clear();
        QStringList paths = {
            QDir::homePath() + "/.themes",
            "/usr/share/themes"
        };
        for (const auto &path : paths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (QDir(path + "/" + sub + "/openbox-3").exists()) {
                    if (wmThemeCombo->findText(sub) < 0)
                        wmThemeCombo->addItem(sub);
                }
            }
        }

        // Detect current
        QString rcPath = QDir::homePath() + "/.config/labwc/rc.xml";
        QFile f(rcPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString(f.readAll());
            f.close();
            QRegularExpression re("<name>(.+)</name>");
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                int idx = wmThemeCombo->findText(m.captured(1).trimmed());
                if (idx >= 0) wmThemeCombo->setCurrentIndex(idx);
            }
            // Load gaps/border
            auto extract = [&](const QString &pattern) -> QString {
                QRegularExpression re(pattern);
                QRegularExpressionMatch mm = re.match(content);
                return mm.hasMatch() ? mm.captured(1).trimmed() : "";
            };
            gapSizeEdit->setText(extract("<gap>(\\d+)</gap>"));
            borderEdit->setText(extract("<width>(\\d+)</width>"));
            cornerEdit->setText(extract("<cornerRadius>(\\d+)</cornerRadius>"));
        }
        statusBar->showMessage(QString("Found %1 window themes").arg(wmThemeCombo->count()), 3000);
    }

    void applyWmTheme() {
        QString rcPath = QDir::homePath() + "/.config/labwc/rc.xml";
        QFile f(rcPath);
        if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot read " + rcPath);
            return;
        }
        QString content = QString(f.readAll());

        QString theme = wmThemeCombo->currentText();
        content.replace(QRegularExpression("<name>[^<]*</name>"), "<name>" + theme + "</name>");

        QString gap = gapSizeEdit->text();
        if (!gap.isEmpty())
            content.replace(QRegularExpression("<gap>\\d*</gap>"), "<gap>" + gap + "</gap>");

        QString border = borderEdit->text();
        if (!border.isEmpty())
            content.replace(QRegularExpression("<width>\\d*</width>"), "<width>" + border + "</width>");

        QString corner = cornerEdit->text();
        if (!corner.isEmpty())
            content.replace(QRegularExpression("<cornerRadius>\\d*</cornerRadius>"), "<cornerRadius>" + corner + "</cornerRadius>");

        f.resize(0);
        f.write(content.toUtf8());
        f.close();
        statusBar->showMessage("WM theme applied. Restart labwc to see changes.", 5000);
        emit changed();
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Window decoration themes for labwc (Openbox-compatible)");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // Theme selection
        auto *themeGroup = new QGroupBox("Window Decoration Theme");
        themeGroup->setStyleSheet(groupBoxStyle());
        auto *themeLayout = new QVBoxLayout(themeGroup);
        themeLayout->setSpacing(8);

        wmThemeCombo = new QComboBox();
        wmThemeCombo->setMinimumHeight(36);
        themeLayout->addWidget(wmThemeCombo);

        auto *themeBtnRow = new QHBoxLayout();
        auto *applyWmBtn = new QPushButton("Apply Theme");
        applyWmBtn->setMinimumHeight(36);
        connect(applyWmBtn, &QPushButton::clicked, this, &WmDecorationTab::applyWmTheme);
        themeBtnRow->addWidget(applyWmBtn);

        auto *refreshWmBtn = new QPushButton("Refresh");
        refreshWmBtn->setMinimumHeight(36);
        connect(refreshWmBtn, &QPushButton::clicked, this, &WmDecorationTab::scanThemes);
        themeBtnRow->addWidget(refreshWmBtn);
        themeBtnRow->addStretch();
        themeLayout->addLayout(themeBtnRow);
        layout->addWidget(themeGroup);

        // Window settings
        auto *settingsGroup = new QGroupBox("Window Layout Settings");
        settingsGroup->setStyleSheet(groupBoxStyle());
        auto *settingsForm = new QFormLayout(settingsGroup);
        settingsForm->setSpacing(8);
        settingsForm->setContentsMargins(10, 20, 10, 10);

        gapSizeEdit = new QLineEdit();
        gapSizeEdit->setPlaceholderText("e.g. 6");
        settingsForm->addRow("Gap Size (px):", gapSizeEdit);

        borderEdit = new QLineEdit();
        borderEdit->setPlaceholderText("e.g. 2");
        settingsForm->addRow("Border Width (px):", borderEdit);

        cornerEdit = new QLineEdit();
        cornerEdit->setPlaceholderText("e.g. 0");
        settingsForm->addRow("Corner Radius (px):", cornerEdit);

        auto *applySettingsBtn = new QPushButton("Apply Window Settings");
        applySettingsBtn->setMinimumHeight(36);
        applySettingsBtn->setStyleSheet(accentBtnStyle());
        connect(applySettingsBtn, &QPushButton::clicked, this, &WmDecorationTab::applyWmTheme);
        settingsForm->addRow("", applySettingsBtn);

        layout->addWidget(settingsGroup);
        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString accentBtnStyle() {
        return "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
               "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
               "QPushButton:hover { background-color: #b4f9b8; }";
    }

    QComboBox *wmThemeCombo;
    QLineEdit *gapSizeEdit;
    QLineEdit *borderEdit;
    QLineEdit *cornerEdit;
    QStatusBar *statusBar;
};

// ============================================================
// Tab 2: Qt Theme
// ============================================================
class QtThemeTab : public QWidget {
    Q_OBJECT
public:
    explicit QtThemeTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        scanQtThemes();
    }

signals:
    void changed();

private slots:
    void scanQtThemes() {
        qtStyleCombo->clear();

        QStringList paths = {
            QDir::homePath() + "/.config/Kvantum",
            "/usr/share/Kvantum",
            "/usr/share/themes"
        };
        QSet<QString> seen;
        for (const auto &path : paths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (!seen.contains(sub)) {
                    qtStyleCombo->addItem(sub);
                    seen.insert(sub);
                }
            }
        }
        qtStyleCombo->addItems({"Fusion", "Windows", "Classic"});

        // Detect current from qt6ct.conf
        QString confPath = QDir::homePath() + "/.config/qt6ct/qt6ct.conf";
        QFile f(confPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString(f.readAll());
            f.close();
            QRegularExpression re("style=(.+)");
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                int idx = qtStyleCombo->findText(m.captured(1).trimmed());
                if (idx >= 0) qtStyleCombo->setCurrentIndex(idx);
            }
            // Detect icon theme
            QRegularExpression reIcon("icon_theme=(.+)");
            QRegularExpressionMatch mIcon = reIcon.match(content);
            if (mIcon.hasMatch()) {
                int idx = iconThemeCombo->findText(mIcon.captured(1).trimmed());
                if (idx >= 0) iconThemeCombo->setCurrentIndex(idx);
            }
        }
        statusBar->showMessage(QString("Found %1 Qt themes").arg(qtStyleCombo->count()), 3000);
    }

    void applyQtTheme() {
        QString confPath = QDir::homePath() + "/.config/qt6ct/qt6ct.conf";
        QFile f(confPath);
        QString content;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(f.readAll());
            f.close();
        }
        if (content.isEmpty()) {
            content = "[Appearance]\nstyle=Fusion\nicon_theme=breeze-dark\n";
        }

        content.replace(QRegularExpression("style=.+"), "style=" + qtStyleCombo->currentText());
        content.replace(QRegularExpression("icon_theme=.+"), "icon_theme=" + iconThemeCombo->currentText());

        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
            statusBar->showMessage("Qt theme set. Restart Qt apps to apply.", 5000);
            emit changed();
        }
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Qt widget style via Qt6CT / Kvantum");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // Style group
        auto *styleGroup = new QGroupBox("Qt Style");
        styleGroup->setStyleSheet(groupBoxStyle());
        auto *styleLayout = new QVBoxLayout(styleGroup);
        styleLayout->setSpacing(8);

        qtStyleCombo = new QComboBox();
        qtStyleCombo->setMinimumHeight(36);
        styleLayout->addWidget(qtStyleCombo);

        auto *applyBtn = new QPushButton("Apply Qt Style");
        applyBtn->setMinimumHeight(36);
        connect(applyBtn, &QPushButton::clicked, this, &QtThemeTab::applyQtTheme);
        styleLayout->addWidget(applyBtn);

        layout->addWidget(styleGroup);

        // Icon theme
        auto *iconGroup = new QGroupBox("Qt Icon Theme (overrides GTK setting for Qt apps)");
        iconGroup->setStyleSheet(groupBoxStyle());
        auto *iconLayout = new QVBoxLayout(iconGroup);
        iconLayout->setSpacing(8);

        iconThemeCombo = new QComboBox();
        iconThemeCombo->setMinimumHeight(36);
        iconLayout->addWidget(iconThemeCombo);

        QSet<QString> seen;
        QStringList iconPaths = {"/usr/share/icons", QDir::homePath() + "/.icons"};
        for (const auto &path : iconPaths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (QFile::exists(path + "/" + sub + "/index.theme") && !seen.contains(sub)) {
                    iconThemeCombo->addItem(sub);
                    seen.insert(sub);
                }
            }
        }

        auto *applyIconBtn = new QPushButton("Apply Icon Theme");
        applyIconBtn->setMinimumHeight(36);
        connect(applyIconBtn, &QPushButton::clicked, this, &QtThemeTab::applyQtTheme);
        iconLayout->addWidget(applyIconBtn);

        layout->addWidget(iconGroup);
        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QComboBox *qtStyleCombo;
    QComboBox *iconThemeCombo;
    QStatusBar *statusBar;
};

// ============================================================
// Tab 3: GTK Theme
// ============================================================
class GtkThemeTab : public QWidget {
    Q_OBJECT
public:
    explicit GtkThemeTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        scanGtkThemes();
    }

signals:
    void changed();

private slots:
    void scanGtkThemes() {
        gtkCombo->clear();
        cursorCombo->clear();

        QStringList themePaths = {"/usr/share/themes", QDir::homePath() + "/.themes"};
        for (const auto &path : themePaths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if ((QDir(path + "/" + sub + "/gtk-3.0").exists() ||
                     QDir(path + "/" + sub + "/gtk-4.0").exists()) &&
                    gtkCombo->findText(sub) < 0) {
                    gtkCombo->addItem(sub);
                }
            }
        }

        QStringList cursorPaths = {"/usr/share/icons", QDir::homePath() + "/.icons"};
        for (const auto &path : cursorPaths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (sub.endsWith("_cursors", Qt::CaseInsensitive) ||
                    QDir(path + "/" + sub + "/cursors").exists()) {
                    if (cursorCombo->findText(sub) < 0)
                        cursorCombo->addItem(sub);
                }
            }
        }

        // Detect current
        QString gtk3Path = QDir::homePath() + "/.config/gtk-3.0/settings.ini";
        QFile f(gtk3Path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString(f.readAll());
            f.close();

            QRegularExpression re("gtk-theme-name=(.+)");
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                int idx = gtkCombo->findText(m.captured(1).trimmed());
                if (idx >= 0) gtkCombo->setCurrentIndex(idx);
            }

            QRegularExpression reCursor("gtk-cursor-theme-name=(.+)");
            QRegularExpressionMatch mCursor = reCursor.match(content);
            if (mCursor.hasMatch()) {
                int idx = cursorCombo->findText(mCursor.captured(1).trimmed());
                if (idx >= 0) cursorCombo->setCurrentIndex(idx);
            }

            QRegularExpression reFont("gtk-font-name=(.+)");
            QRegularExpressionMatch mFont = reFont.match(content);
            if (mFont.hasMatch()) {
                currentFont = mFont.captured(1).trimmed();
            }
        }
        statusBar->showMessage(QString("Found %1 GTK, %2 cursor themes")
            .arg(gtkCombo->count()).arg(cursorCombo->count()), 3000);
    }

    void applyGtkTheme() {
        QString theme = gtkCombo->currentText();
        QProcess::execute("gsettings", {"set", "org.gnome.desktop.interface", "gtk-theme", theme});
        statusBar->showMessage("GTK theme set to: " + theme, 3000);
        emit changed();
    }

    void applyCursorTheme() {
        QString theme = cursorCombo->currentText();
        QProcess::execute("gsettings", {"set", "org.gnome.desktop.interface", "cursor-theme", theme});

        // Update GTK3 settings.ini
        QString gtk3Path = QDir::homePath() + "/.config/gtk-3.0/settings.ini";
        QFile f(gtk3Path);
        QString content;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(f.readAll());
            f.close();
        }
        if (content.contains("gtk-cursor-theme-name")) {
            content.replace(QRegularExpression("gtk-cursor-theme-name=.+"), "gtk-cursor-theme-name=" + theme);
        } else {
            content += "\ngtk-cursor-theme-name=" + theme + "\n";
        }
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
        }

        statusBar->showMessage("Cursor theme set to: " + theme + " (may need restart)", 3000);
        emit changed();
    }

    void applyFontSetting() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, QFont("Noto Sans", 10), this, "Choose GTK Interface Font");
        if (!ok) return;

        currentFont = font.family() + ", " + QString::number(font.pointSize());
        QProcess::execute("gsettings", {"set", "org.gnome.desktop.interface", "font-name", currentFont});

        // Update GTK3 settings.ini
        QString gtk3Path = QDir::homePath() + "/.config/gtk-3.0/settings.ini";
        QFile f(gtk3Path);
        QString content;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(f.readAll());
            f.close();
        }
        if (content.contains("gtk-font-name")) {
            content.replace(QRegularExpression("gtk-font-name=.+"), "gtk-font-name=" + currentFont);
        } else {
            content += "\ngtk-font-name=" + currentFont + "\n";
        }
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
        }

        fontLabel->setText("Current: " + currentFont);
        statusBar->showMessage("GTK font set to: " + currentFont, 3000);
        emit changed();
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("GTK theme, cursor, and font settings");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // GTK theme
        auto *gtkGroup = new QGroupBox("GTK Theme (3 & 4)");
        gtkGroup->setStyleSheet(groupBoxStyle());
        auto *gtkLayout = new QVBoxLayout(gtkGroup);
        gtkLayout->setSpacing(8);

        gtkCombo = new QComboBox();
        gtkCombo->setMinimumHeight(36);
        gtkLayout->addWidget(gtkCombo);

        auto *gtkBtnRow = new QHBoxLayout();
        auto *applyGtkBtn = new QPushButton("Apply GTK Theme");
        applyGtkBtn->setMinimumHeight(36);
        connect(applyGtkBtn, &QPushButton::clicked, this, &GtkThemeTab::applyGtkTheme);
        gtkBtnRow->addWidget(applyGtkBtn);

        auto *refreshGtkBtn = new QPushButton("Refresh");
        refreshGtkBtn->setMinimumHeight(36);
        connect(refreshGtkBtn, &QPushButton::clicked, this, &GtkThemeTab::scanGtkThemes);
        gtkBtnRow->addWidget(refreshGtkBtn);
        gtkBtnRow->addStretch();
        gtkLayout->addLayout(gtkBtnRow);
        layout->addWidget(gtkGroup);

        // Cursor theme
        auto *cursorGroup = new QGroupBox("Cursor Theme");
        cursorGroup->setStyleSheet(groupBoxStyle());
        auto *cursorLayout = new QVBoxLayout(cursorGroup);
        cursorLayout->setSpacing(8);

        cursorCombo = new QComboBox();
        cursorCombo->setMinimumHeight(36);
        cursorLayout->addWidget(cursorCombo);

        auto *applyCursorBtn = new QPushButton("Apply Cursor Theme");
        applyCursorBtn->setMinimumHeight(36);
        connect(applyCursorBtn, &QPushButton::clicked, this, &GtkThemeTab::applyCursorTheme);
        cursorLayout->addWidget(applyCursorBtn);
        layout->addWidget(cursorGroup);

        // Font (quick access)
        auto *fontGroup = new QGroupBox("GTK Interface Font");
        fontGroup->setStyleSheet(groupBoxStyle());
        auto *fontLayout = new QVBoxLayout(fontGroup);
        fontLayout->setSpacing(8);

        fontLabel = new QLabel("Current: " + currentFont);
        fontLabel->setStyleSheet("color: #a6adc8; font-size: 13px;");
        fontLayout->addWidget(fontLabel);

        auto *chooseFontBtn = new QPushButton("Choose GTK Font...");
        chooseFontBtn->setMinimumHeight(36);
        connect(chooseFontBtn, &QPushButton::clicked, this, &GtkThemeTab::applyFontSetting);
        fontLayout->addWidget(chooseFontBtn);
        layout->addWidget(fontGroup);

        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QComboBox *gtkCombo;
    QComboBox *cursorCombo;
    QLabel *fontLabel;
    QString currentFont{"Noto Sans, 10"};
    QStatusBar *statusBar;
};

// ============================================================
// Tab 4: Icons & Cursors
// ============================================================
class IconCursorTab : public QWidget {
    Q_OBJECT
public:
    explicit IconCursorTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        scanIconThemes();
    }

signals:
    void changed();

private slots:
    void scanIconThemes() {
        iconThemeCombo->clear();

        QStringList paths = {"/usr/share/icons", QDir::homePath() + "/.icons"};
        for (const auto &path : paths) {
            QDir dir(path);
            for (const auto &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (QFile::exists(path + "/" + sub + "/index.theme") && iconThemeCombo->findText(sub) < 0) {
                    iconThemeCombo->addItem(sub);
                }
            }
        }

        // Detect current from gsettings
        QProcess proc;
        proc.start("gsettings", {"get", "org.gnome.desktop.interface", "icon-theme"});
        proc.waitForFinished(3000);
        QString current = QString::fromUtf8(proc.readAllStandardOutput().trimmed()).remove('\'');
        int idx = iconThemeCombo->findText(current);
        if (idx >= 0) iconThemeCombo->setCurrentIndex(idx);

        statusBar->showMessage(QString("Found %1 icon themes").arg(iconThemeCombo->count()), 3000);
    }

    void applyIconTheme() {
        QString theme = iconThemeCombo->currentText();
        QProcess::execute("gsettings", {"set", "org.gnome.desktop.interface", "icon-theme", theme});

        // Update LXQt configs
        auto updateLxqtConf = [&](const QString &path, const QString &key) {
            QFile f(path);
            if (!f.exists()) return;
            if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) return;
            QString content = QString(f.readAll());
            if (content.contains(key)) {
                content.replace(QRegularExpression(key + "=.+"), key + "=" + theme);
            } else {
                content += "\n" + key + "=" + theme + "\n";
            }
            f.resize(0);
            f.write(content.toUtf8());
            f.close();
        };

        updateLxqtConf(QDir::homePath() + "/.config/lxqt/lxqt.conf", "iconTheme");
        updateLxqtConf(QDir::homePath() + "/.config/lxqt/session.conf", "icon_theme");

        statusBar->showMessage("Icon theme set to: " + theme, 3000);
        emit changed();
    }

    void applySize() {
        int size = sizeSpin->value();
        QProcess::execute("gsettings", {"set", "org.gnome.desktop.interface", "cursor-size", QString::number(size)});

        QString envPath = QDir::homePath() + "/.config/labwc/environment";
        QFile f(envPath);
        QString content;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            content = QString(f.readAll());
            f.close();
        }
        if (content.contains("XCURSOR_SIZE")) {
            content.replace(QRegularExpression("XCURSOR_SIZE=\\d+"), "XCURSOR_SIZE=" + QString::number(size));
        } else {
            content += "\nXCURSOR_SIZE=" + QString::number(size) + "\n";
        }
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
        }
        statusBar->showMessage("Cursor size set to " + QString::number(size), 3000);
        emit changed();
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("System-wide icon and cursor theme settings");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // Icon theme
        auto *iconGroup = new QGroupBox("Icon Theme");
        iconGroup->setStyleSheet(groupBoxStyle());
        auto *iconLayout = new QVBoxLayout(iconGroup);
        iconLayout->setSpacing(8);

        iconThemeCombo = new QComboBox();
        iconThemeCombo->setMinimumHeight(36);
        iconLayout->addWidget(iconThemeCombo);

        auto *iconBtnRow = new QHBoxLayout();
        auto *applyIconBtn = new QPushButton("Apply Icon Theme");
        applyIconBtn->setMinimumHeight(36);
        connect(applyIconBtn, &QPushButton::clicked, this, &IconCursorTab::applyIconTheme);
        iconBtnRow->addWidget(applyIconBtn);

        auto *refreshIconBtn = new QPushButton("Refresh");
        refreshIconBtn->setMinimumHeight(36);
        connect(refreshIconBtn, &QPushButton::clicked, this, &IconCursorTab::scanIconThemes);
        iconBtnRow->addWidget(refreshIconBtn);
        iconBtnRow->addStretch();
        iconLayout->addLayout(iconBtnRow);
        layout->addWidget(iconGroup);

        // Cursor size
        auto *sizeGroup = new QGroupBox("Cursor Size");
        sizeGroup->setStyleSheet(groupBoxStyle());
        auto *sizeLayout = new QVBoxLayout(sizeGroup);
        sizeLayout->setSpacing(8);

        auto *sizeRow = new QHBoxLayout();
        sizeRow->addWidget(new QLabel("Size:"));
        sizeSpin = new QSpinBox();
        sizeSpin->setRange(16, 96);
        sizeSpin->setValue(24);
        sizeSpin->setMinimumHeight(36);
        sizeSpin->setStyleSheet(
            "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 6px; }");
        sizeRow->addWidget(sizeSpin);
        sizeRow->addStretch();
        sizeLayout->addLayout(sizeRow);

        auto *applySizeBtn = new QPushButton("Apply Cursor Size");
        applySizeBtn->setMinimumHeight(36);
        applySizeBtn->setStyleSheet(accentBtnStyle());
        connect(applySizeBtn, &QPushButton::clicked, this, &IconCursorTab::applySize);
        sizeLayout->addWidget(applySizeBtn);
        layout->addWidget(sizeGroup);

        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString accentBtnStyle() {
        return "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
               "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
               "QPushButton:hover { background-color: #b4f9b8; }";
    }

    QComboBox *iconThemeCombo;
    QSpinBox *sizeSpin;
    QStatusBar *statusBar;
};

// ============================================================
// Tab 5: Panel
// ============================================================
class PanelTab : public QWidget {
    Q_OBJECT
public:
    explicit PanelTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        loadPanelConfig();
    }

signals:
    void changed();

private slots:
    void loadPanelConfig() {
        QString panelPath = QDir::homePath() + "/.config/lxqt/panel.conf";
        QFile f(panelPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QString content = QString(f.readAll());
        f.close();

        auto extract = [&](const QString &section, const QString &key) -> QString {
            int secStart = content.indexOf("[" + section + "]");
            if (secStart < 0) return "";
            int secEnd = content.indexOf("\n[", secStart + 1);
            if (secEnd < 0) secEnd = content.length();
            QString sec = content.mid(secStart, secEnd - secStart);
            QRegularExpression re(key + "=(.+)");
            QRegularExpressionMatch m = re.match(sec);
            return m.hasMatch() ? m.captured(1).trimmed() : "";
        };

        QString panelSize = extract("panel1", "panelsize");
        if (!panelSize.isEmpty()) panelSizeSpin->setValue(panelSize.toInt());

        QString iconSize = extract("panel1", "iconsize");
        if (!iconSize.isEmpty()) iconSizeSpin->setValue(iconSize.toInt());

        QString position = extract("panel1", "position");
        if (position == "Top") positionCombo->setCurrentIndex(1);
        else if (position == "Left") positionCombo->setCurrentIndex(2);
        else if (position == "Right") positionCombo->setCurrentIndex(3);
        else positionCombo->setCurrentIndex(0); // Bottom

        QString opacity = extract("panel1", "opacity");
        if (!opacity.isEmpty()) {
            opacitySpin->setValue(static_cast<int>(opacity.toDouble() * 100));
        }

        // Load plugins
        QString plugins = extract("panel1", "plugins");
        pluginList->clear();
        if (!plugins.isEmpty()) {
            pluginList->addItems(plugins.split(",", Qt::SkipEmptyParts));
        }
    }

    void savePanelConfig() {
        QString panelPath = QDir::homePath() + "/.config/lxqt/panel.conf";
        QFile f(panelPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot read " + panelPath);
            return;
        }
        QString content = QString(f.readAll());
        f.close();

        auto setVal = [&](const QString &section, const QString &key, const QString &value) {
            QString marker = "[" + section + "]";
            int secStart = content.indexOf(marker);
            if (secStart < 0) {
                content += "\n" + marker + "\n" + key + "=" + value + "\n";
                return;
            }
            int secEnd = content.indexOf("\n[", secStart + 1);
            if (secEnd < 0) secEnd = content.length();
            QString sec = content.mid(secStart, secEnd - secStart);
            QRegularExpression re(key + "=.+");
            if (sec.contains(re)) {
                content.replace(secStart, secEnd - secStart,
                    sec.replace(re, key + "=" + value));
            } else {
                content.insert(secEnd, key + "=" + value + "\n");
            }
        };

        QString pos = positionCombo->currentText();
        setVal("panel1", "position", pos);
        setVal("panel1", "panelsize", QString::number(panelSizeSpin->value()));
        setVal("panel1", "iconsize", QString::number(iconSizeSpin->value()));
        setVal("panel1", "opacity", QString::number(opacitySpin->value() / 100.0, 'f', 2));

        // Rebuild plugins list
        QStringList plugins;
        for (int i = 0; i < pluginList->count(); i++)
            plugins << pluginList->item(i)->text();
        QStringList allPlugins = {"fancymenu", "desktopswitch", "quicklaunch", "taskbar",
                                  "spacer", "statusnotifier", "tray", "volume", "worldclock",
                                  "showdesktop", "clock", "mainmenu"};
        // Add any default plugins not in list but remove unused ones
        // For simplicity, set the explicit list
        setVal("panel1", "plugins", plugins.join(","));

        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(content.toUtf8());
            f.close();
            statusBar->showMessage("Panel config saved. Restart panel to apply.", 5000);
            emit changed();
        }
    }

    void addPlugin() {
        QStringList available = {"fancymenu", "desktopswitch", "quicklaunch", "taskbar",
                                "spacer", "statusnotifier", "tray", "volume", "worldclock",
                                "showdesktop", "clock", "mainmenu"};
        QSet<QString> existing;
        for (int i = 0; i < pluginList->count(); i++)
            existing.insert(pluginList->item(i)->text());

        for (int i = available.size() - 1; i >= 0; i--) {
            if (existing.contains(available[i]))
                available.removeAt(i);
        }

        if (available.isEmpty()) {
            QMessageBox::information(this, "All Added", "All plugins are already in the list.");
            return;
        }

        bool ok;
        QString chosen = QInputDialog::getItem(this, "Add Plugin",
            "Select plugin to add:", available, 0, false, &ok);
        if (ok && !chosen.isEmpty()) {
            pluginList->addItem(chosen);
        }
    }

    void removePlugin() {
        auto *item = pluginList->currentItem();
        if (item) delete pluginList->takeItem(pluginList->row(item));
    }

    void movePluginUp() {
        int row = pluginList->currentRow();
        if (row > 0) {
            auto *item = pluginList->takeItem(row);
            pluginList->insertItem(row - 1, item);
            pluginList->setCurrentRow(row - 1);
        }
    }

    void movePluginDown() {
        int row = pluginList->currentRow();
        if (row >= 0 && row < pluginList->count() - 1) {
            auto *item = pluginList->takeItem(row);
            pluginList->insertItem(row + 1, item);
            pluginList->setCurrentRow(row + 1);
        }
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("LXQt Panel configuration");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // Panel position & size
        auto *panelGroup = new QGroupBox("Panel Layout");
        panelGroup->setStyleSheet(groupBoxStyle());
        auto *panelForm = new QFormLayout(panelGroup);
        panelForm->setSpacing(8);
        panelForm->setContentsMargins(10, 20, 10, 10);

        positionCombo = new QComboBox();
        positionCombo->addItems({"Bottom", "Top", "Left", "Right"});
        positionCombo->setMinimumHeight(34);
        panelForm->addRow("Position:", positionCombo);

        panelSizeSpin = new QSpinBox();
        panelSizeSpin->setRange(24, 128);
        panelSizeSpin->setValue(48);
        panelSizeSpin->setSuffix(" px");
        panelSizeSpin->setMinimumHeight(34);
        panelSizeSpin->setStyleSheet(spinStyle());
        panelForm->addRow("Panel Size:", panelSizeSpin);

        iconSizeSpin = new QSpinBox();
        iconSizeSpin->setRange(16, 64);
        iconSizeSpin->setValue(32);
        iconSizeSpin->setSuffix(" px");
        iconSizeSpin->setMinimumHeight(34);
        iconSizeSpin->setStyleSheet(spinStyle());
        panelForm->addRow("Icon Size:", iconSizeSpin);

        opacitySpin = new QSpinBox();
        opacitySpin->setRange(10, 100);
        opacitySpin->setValue(90);
        opacitySpin->setSuffix(" %");
        opacitySpin->setMinimumHeight(34);
        opacitySpin->setStyleSheet(spinStyle());
        panelForm->addRow("Opacity:", opacitySpin);

        layout->addWidget(panelGroup);

        // Plugins
        auto *pluginGroup = new QGroupBox("Panel Plugins (order: top to bottom/left to right)");
        pluginGroup->setStyleSheet(groupBoxStyle());
        auto *pluginLayout = new QVBoxLayout(pluginGroup);
        pluginLayout->setSpacing(8);

        pluginList = new QListWidget();
        pluginList->setMinimumHeight(150);
        pluginList->setStyleSheet(
            "QListWidget { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; }"
            "QListWidget::item { padding: 6px; }"
            "QListWidget::item:selected { background-color: #313244; color: #89b4fa; }");
        pluginLayout->addWidget(pluginList);

        auto *pluginBtnRow = new QHBoxLayout();
        pluginBtnRow->setSpacing(6);

        auto *addPluginBtn = new QPushButton("+ Add");
        addPluginBtn->setMinimumHeight(32);
        addPluginBtn->setStyleSheet(miniBtnStyle("#a6e3a1"));
        connect(addPluginBtn, &QPushButton::clicked, this, &PanelTab::addPlugin);
        pluginBtnRow->addWidget(addPluginBtn);

        auto *removePluginBtn = new QPushButton("- Remove");
        removePluginBtn->setMinimumHeight(32);
        removePluginBtn->setStyleSheet(miniBtnStyle("#f38ba8"));
        connect(removePluginBtn, &QPushButton::clicked, this, &PanelTab::removePlugin);
        pluginBtnRow->addWidget(removePluginBtn);

        pluginBtnRow->addStretch();

        auto *upBtn = new QPushButton("Up");
        upBtn->setMinimumHeight(32);
        upBtn->setStyleSheet(miniBtnStyle("#89b4fa"));
        connect(upBtn, &QPushButton::clicked, this, &PanelTab::movePluginUp);
        pluginBtnRow->addWidget(upBtn);

        auto *downBtn = new QPushButton("Down");
        downBtn->setMinimumHeight(32);
        downBtn->setStyleSheet(miniBtnStyle("#89b4fa"));
        connect(downBtn, &QPushButton::clicked, this, &PanelTab::movePluginDown);
        pluginBtnRow->addWidget(downBtn);

        pluginLayout->addLayout(pluginBtnRow);
        layout->addWidget(pluginGroup);

        auto *savePanelBtn = new QPushButton("Save Panel Configuration");
        savePanelBtn->setMinimumHeight(38);
        savePanelBtn->setStyleSheet(saveBtnStyle());
        connect(savePanelBtn, &QPushButton::clicked, this, &PanelTab::savePanelConfig);
        layout->addWidget(savePanelBtn);

        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString spinStyle() {
        return "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
               "border-radius: 6px; padding: 6px; }";
    }

    QString miniBtnStyle(const QString &color) {
        return QString("QPushButton { background-color: %1; color: #1e1e2e; border-radius: 4px; "
                       "padding: 6px 12px; font-weight: bold; font-size: 12px; border: none; }"
                       "QPushButton:hover { background-color: %1; opacity: 0.8; }").arg(color);
    }

    QString saveBtnStyle() {
        return "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
               "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
               "QPushButton:hover { background-color: #b4f9b8; }";
    }

    QComboBox *positionCombo;
    QSpinBox *panelSizeSpin;
    QSpinBox *iconSizeSpin;
    QSpinBox *opacitySpin;
    QListWidget *pluginList;
    QStatusBar *statusBar;
};

// ============================================================
// Tab 6: Fonts
// ============================================================
class FontThemeTab : public QWidget {
    Q_OBJECT
public:
    explicit FontThemeTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        loadFontSettings();
    }

signals:
    void changed();

private slots:
    void loadFontSettings() {
        // Detect from LXQt config
        QString lxqtPath = QDir::homePath() + "/.config/lxqt/lxqt.conf";
        QFile f(lxqtPath);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString(f.readAll());
            f.close();

            QRegularExpression re("font=(.+),(-?\\d+)");
            QRegularExpressionMatch m = re.match(content);
            if (m.hasMatch()) {
                uiFontCombo->setCurrentText(m.captured(1).trimmed());
                uiFontSizeSpin->setValue(m.captured(2).toInt());
            }

            QRegularExpression reFixed("fixedFont=(.+),(-?\\d+)");
            QRegularExpressionMatch mFixed = reFixed.match(content);
            if (mFixed.hasMatch()) {
                fixedFontCombo->setCurrentText(mFixed.captured(1).trimmed());
                fixedFontSizeSpin->setValue(mFixed.captured(2).toInt());
            }
        }

        // Detect from labwc rc.xml
        QString rcPath = QDir::homePath() + "/.config/labwc/rc.xml";
        QFile frc(rcPath);
        if (frc.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString(frc.readAll());
            frc.close();

            QRegularExpression reWm("<font place=\"ActiveWindow\">(.+),(\\d+)</font>");
            QRegularExpressionMatch mWm = reWm.match(content);
            if (mWm.hasMatch()) {
                wmFontCombo->setCurrentText(mWm.captured(1).trimmed());
                wmFontSizeSpin->setValue(mWm.captured(2).toInt());
            }
        }

        // Populate available font families
        populateFontFamilies();
    }

    void populateFontFamilies() {
        QProcess proc;
        proc.start("fc-list", {"--format=%{family}\\n"});
        proc.waitForFinished(5000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());
        QStringList families;
        for (const auto &line : output.split('\n', Qt::SkipEmptyParts)) {
            for (const auto &f : line.split(',')) {
                QString name = f.trimmed();
                if (!name.isEmpty() && !families.contains(name))
                    families.append(name);
            }
        }
        families.sort();

        uiFontCombo->clear();
        fixedFontCombo->clear();
        wmFontCombo->clear();

        uiFontCombo->addItems(families);
        fixedFontCombo->addItems(families);
        wmFontCombo->addItems(families);

        // Re-set current values
        QStringList currentUis, currentFixed, currentWm;
        loadFontSettings();
    }

    void applyFonts() {
        QString uiFont = uiFontCombo->currentText();
        int uiSize = uiFontSizeSpin->value();
        QString fixedFont = fixedFontCombo->currentText();
        int fixedSize = fixedFontSizeSpin->value();
        QString wmFont = wmFontCombo->currentText();
        int wmSize = wmFontSizeSpin->value();

        // Update LXQt configs
        auto updateLxqtConf = [&](const QString &path) {
            QFile f(path);
            if (!f.exists()) return;
            if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) return;
            QString content = QString(f.readAll());
            content.replace(QRegularExpression("font=[^\\n]*"),
                QString("font=%1,%2,-1,5,50,0,0,0,0,0").arg(uiFont).arg(uiSize));
            content.replace(QRegularExpression("fixedFont=[^\\n]*"),
                QString("fixedFont=%1,%2,-1,5,50,0,0,0,0,0").arg(fixedFont).arg(fixedSize));
            f.resize(0);
            f.write(content.toUtf8());
            f.close();
        };

        updateLxqtConf(QDir::homePath() + "/.config/lxqt/lxqt.conf");
        updateLxqtConf(QDir::homePath() + "/.config/lxqt/session.conf");

        // Update GTK settings
        auto updateGtkConf = [&](const QString &path) {
            QFile f(path);
            if (!f.exists()) return;
            if (!f.open(QIODevice::ReadWrite | QIODevice::Text)) return;
            QString content = QString(f.readAll());
            content.replace(QRegularExpression("gtk-font-name=.+"),
                QString("gtk-font-name=%1, %2").arg(uiFont).arg(uiSize));
            content.replace(QRegularExpression("gtk-monospace-font-name=.+"),
                QString("gtk-monospace-font-name=%1, %2").arg(fixedFont).arg(fixedSize));
            f.resize(0);
            f.write(content.toUtf8());
            f.close();
        };

        updateGtkConf(QDir::homePath() + "/.config/gtk-3.0/settings.ini");
        updateGtkConf(QDir::homePath() + "/.config/gtk-4.0/settings.ini");

        // Update labwc rc.xml
        QString rcPath = QDir::homePath() + "/.config/labwc/rc.xml";
        QFile frc(rcPath);
        if (frc.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString content = QString(frc.readAll());
            content.replace(QRegularExpression("<font place=\"ActiveWindow\">[^<]+</font>"),
                QString("<font place=\"ActiveWindow\">%1,%2</font>").arg(wmFont).arg(wmSize));
            content.replace(QRegularExpression("<font place=\"MenuItem\">[^<]+</font>"),
                QString("<font place=\"MenuItem\">%1,%2</font>").arg(uiFont).arg(uiSize));
            frc.resize(0);
            frc.write(content.toUtf8());
            frc.close();
        }

        // Update Qt6CT
        QString qt6ctPath = QDir::homePath() + "/.config/qt6ct/qt6ct.conf";
        QFile fq(qt6ctPath);
        if (fq.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QString content = QString(fq.readAll());
            content.replace(QRegularExpression("general=@Variant\\([^)]*\\)"),
                QString("general=@Variant(\\0\\0\\0\\0\\0\\0\\0\\0)"));
            fq.resize(0);
            fq.write(content.toUtf8());
            fq.close();
        }

        statusBar->showMessage("Font settings applied. Restart apps to see changes.", 5000);
        emit changed();
    }

    void chooseUiFont() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, QFont(uiFontCombo->currentText(), uiFontSizeSpin->value()),
                                          this, "Choose UI Font");
        if (!ok) return;
        uiFontCombo->setCurrentText(font.family());
        uiFontSizeSpin->setValue(font.pointSize());
    }

    void chooseFixedFont() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, QFont(fixedFontCombo->currentText(), fixedFontSizeSpin->value()),
                                          this, "Choose Fixed/Monospace Font");
        if (!ok) return;
        fixedFontCombo->setCurrentText(font.family());
        fixedFontSizeSpin->setValue(font.pointSize());
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("Font settings for LXQt, GTK, and labwc window titles");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // UI font
        auto *uiGroup = new QGroupBox("Interface Font (LXQt, GTK, menus)");
        uiGroup->setStyleSheet(groupBoxStyle());
        auto *uiForm = new QFormLayout(uiGroup);
        uiForm->setSpacing(8);
        uiForm->setContentsMargins(10, 20, 10, 10);

        uiFontCombo = new QComboBox();
        uiFontCombo->setEditable(true);
        uiFontCombo->setMinimumHeight(34);
        uiFontCombo->setStyleSheet(comboStyle());
        uiForm->addRow("Font Family:", uiFontCombo);

        auto *uiSizeRow = new QHBoxLayout();
        uiFontSizeSpin = new QSpinBox();
        uiFontSizeSpin->setRange(6, 36);
        uiFontSizeSpin->setValue(10);
        uiFontSizeSpin->setMinimumHeight(34);
        uiFontSizeSpin->setStyleSheet(spinStyle());
        uiSizeRow->addWidget(uiFontSizeSpin);
        uiSizeRow->addWidget(new QLabel("pt"));
        uiSizeRow->addStretch();
        auto *chooseUiBtn = new QPushButton("Browse...");
        chooseUiBtn->setMinimumHeight(34);
        chooseUiBtn->setStyleSheet(miniBtnStyle("#89b4fa"));
        connect(chooseUiBtn, &QPushButton::clicked, this, &FontThemeTab::chooseUiFont);
        uiSizeRow->addWidget(chooseUiBtn);
        uiForm->addRow("Font Size:", uiSizeRow);
        layout->addWidget(uiGroup);

        // Fixed font
        auto *fixedGroup = new QGroupBox("Fixed/Monospace Font (terminals, code editors)");
        fixedGroup->setStyleSheet(groupBoxStyle());
        auto *fixedForm = new QFormLayout(fixedGroup);
        fixedForm->setSpacing(8);
        fixedForm->setContentsMargins(10, 20, 10, 10);

        fixedFontCombo = new QComboBox();
        fixedFontCombo->setEditable(true);
        fixedFontCombo->setMinimumHeight(34);
        fixedFontCombo->setStyleSheet(comboStyle());
        fixedForm->addRow("Font Family:", fixedFontCombo);

        auto *fixedSizeRow = new QHBoxLayout();
        fixedFontSizeSpin = new QSpinBox();
        fixedFontSizeSpin->setRange(6, 36);
        fixedFontSizeSpin->setValue(10);
        fixedFontSizeSpin->setMinimumHeight(34);
        fixedFontSizeSpin->setStyleSheet(spinStyle());
        fixedSizeRow->addWidget(fixedFontSizeSpin);
        fixedSizeRow->addWidget(new QLabel("pt"));
        fixedSizeRow->addStretch();
        auto *chooseFixedBtn = new QPushButton("Browse...");
        chooseFixedBtn->setMinimumHeight(34);
        chooseFixedBtn->setStyleSheet(miniBtnStyle("#89b4fa"));
        connect(chooseFixedBtn, &QPushButton::clicked, this, &FontThemeTab::chooseFixedFont);
        fixedSizeRow->addWidget(chooseFixedBtn);
        fixedForm->addRow("Font Size:", fixedSizeRow);
        layout->addWidget(fixedGroup);

        // Window title font
        auto *wmGroup = new QGroupBox("Window Title Font (labwc active window)");
        wmGroup->setStyleSheet(groupBoxStyle());
        auto *wmForm = new QFormLayout(wmGroup);
        wmForm->setSpacing(8);
        wmForm->setContentsMargins(10, 20, 10, 10);

        wmFontCombo = new QComboBox();
        wmFontCombo->setEditable(true);
        wmFontCombo->setMinimumHeight(34);
        wmFontCombo->setStyleSheet(comboStyle());
        wmForm->addRow("Font Family:", wmFontCombo);

        wmFontSizeSpin = new QSpinBox();
        wmFontSizeSpin->setRange(6, 36);
        wmFontSizeSpin->setValue(10);
        wmFontSizeSpin->setMinimumHeight(34);
        wmFontSizeSpin->setStyleSheet(spinStyle());
        wmForm->addRow("Font Size:", wmFontSizeSpin);
        layout->addWidget(wmGroup);

        auto *applyFontBtn = new QPushButton("Apply Font Settings to All Configs");
        applyFontBtn->setMinimumHeight(38);
        applyFontBtn->setStyleSheet(saveBtnStyle());
        connect(applyFontBtn, &QPushButton::clicked, this, &FontThemeTab::applyFonts);
        layout->addWidget(applyFontBtn);

        layout->addStretch();
    }

    QString groupBoxStyle() {
        return "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 14px; "
               "border: 1px solid #45475a; border-radius: 8px; margin-top: 10px; padding-top: 18px; }"
               "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }";
    }

    QString comboStyle() {
        return "QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
               "border-radius: 6px; padding: 6px; }"
               "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; }";
    }

    QString spinStyle() {
        return "QSpinBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
               "border-radius: 6px; padding: 6px; }";
    }

    QString miniBtnStyle(const QString &color) {
        return QString("QPushButton { background-color: %1; color: #1e1e2e; border-radius: 4px; "
                       "padding: 6px 12px; font-weight: bold; font-size: 12px; border: none; }").arg(color);
    }

    QString saveBtnStyle() {
        return "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
               "padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
               "QPushButton:hover { background-color: #b4f9b8; }";
    }

    QComboBox *uiFontCombo;
    QSpinBox *uiFontSizeSpin;
    QComboBox *fixedFontCombo;
    QSpinBox *fixedFontSizeSpin;
    QComboBox *wmFontCombo;
    QSpinBox *wmFontSizeSpin;
    QStatusBar *statusBar;
};

// ============================================================
// Tab 7: Global Themes
// ============================================================
class GlobalThemesTab : public QWidget {
    Q_OBJECT
public:
    explicit GlobalThemesTab(QStatusBar *sb, QWidget *parent = nullptr)
        : QWidget(parent), statusBar(sb) {
        setupUI();
        scanThemes();
        loadCurrentTheme();
    }

signals:
    void changed();

private slots:
    void scanThemes() {
        themeList->clear();
        themeCards.clear();

        // Scan configs/themes/ directory
        QString themesDir = QApplication::applicationDirPath() + "/../../../configs/themes";
        if (!QDir(themesDir).exists()) {
            // Try relative to common install paths
            themesDir = QDir::homePath() + "/.local/share/naravisuals/configs/themes";
        }
        if (!QDir(themesDir).exists()) {
            themesDir = QApplication::applicationDirPath() + "/configs/themes";
        }

        QDir dir(themesDir);
        if (!dir.exists()) {
            statusBar->showMessage("Theme directory not found: " + themesDir, 5000);
            return;
        }

        QStringList files = dir.entryList(QStringList() << "*.conf", QDir::Files, QDir::Name);
        for (const auto &file : files) {
            QString name = file.chopped(5); // remove .conf
            ThemeCard card;
            card.name = name;
            card.filePath = dir.absoluteFilePath(file);
            parseThemeProfile(card);
            themeCards.append(card);

            auto *item = new QListWidgetItem();
            item->setData(Qt::UserRole, themeCards.size() - 1);
            item->setSizeHint(QSize(0, 72));
            themeList->addItem(item);
        }

        statusBar->showMessage(QString("Found %1 global themes").arg(themeCards.size()), 3000);
    }

    void loadCurrentTheme() {
        QString stateFile = QDir::homePath() + "/.config/lxqt-rice/current-theme";
        QFile f(stateFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString current = QString::fromUtf8(f.readAll()).trimmed();
            f.close();
            currentThemeLabel->setText("Active: " + current);
            // Select in list
            for (int i = 0; i < themeCards.size(); i++) {
                if (themeCards[i].name == current) {
                    themeList->setCurrentRow(i);
                    showThemeDetails(i);
                    break;
                }
            }
        }
    }

    void onThemeSelected(int row) {
        if (row >= 0 && row < themeCards.size()) {
            showThemeDetails(row);
        }
    }

    void applySelectedTheme() {
        int row = themeList->currentRow();
        if (row < 0 || row >= themeCards.size()) {
            QMessageBox::information(this, "No Selection", "Select a theme from the list first.");
            return;
        }

        const ThemeCard &card = themeCards[row];

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Apply Theme",
            "Apply global theme \"" + card.name + "\"?\n\n"
            "This will:\n"
            "  - Set GTK theme: " + card.gtkTheme + "\n"
            "  - Set WM theme: " + card.wmTheme + "\n"
            "  - Set Qt style: " + card.qtStyle + "\n"
            "  - Set icons: " + card.iconTheme + "\n"
            "  - Set cursor: " + card.cursorTheme + "\n"
            "  - Update panel opacity and colors\n\n"
            "A backup of current configs will be created.",
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes) return;

        // Run apply-theme.sh
        QProcess proc;
        QString script = QApplication::applicationDirPath() + "/../../../apply-theme.sh";
        if (!QFile::exists(script)) {
            script = QDir::homePath() + "/.local/share/naravisuals/apply-theme.sh";
        }

        if (!QFile::exists(script)) {
            QMessageBox::critical(this, "Error", "apply-theme.sh not found.\n"
                "Expected at: " + QApplication::applicationDirPath() + "/../../../apply-theme.sh");
            return;
        }

        proc.start("bash", {script, card.name});
        proc.waitForFinished(10000);

        if (proc.exitCode() == 0) {
            QString output = QString::fromUtf8(proc.readAllStandardOutput());
            currentThemeLabel->setText("Active: " + card.name);
            statusBar->showMessage("Theme \"" + card.name + "\" applied. Restart labwc for full effect.", 5000);
            emit changed();
        } else {
            QString err = QString::fromUtf8(proc.readAllStandardError());
            QMessageBox::warning(this, "Theme Apply Failed",
                "Failed to apply theme:\n" + err);
        }
    }

    void previewTheme() {
        int row = themeList->currentRow();
        if (row < 0 || row >= themeCards.size()) return;

        const ThemeCard &card = themeCards[row];

        QString info;
        info += "<h3 style=\"color:#89b4fa\">" + card.metaName + "</h3>";
        info += "<p style=\"color:#a6adc8\">" + card.description + "</p>";
        info += "<table style=\"color:#cdd6f4;font-size:13px\">";
        info += "<tr><td><b>GTK Theme:</b></td><td>" + card.gtkTheme + "</td></tr>";
        info += "<tr><td><b>WM Theme:</b></td><td>" + card.wmTheme + "</td></tr>";
        info += "<tr><td><b>Qt Style:</b></td><td>" + card.qtStyle + "</td></tr>";
        info += "<tr><td><b>Qt Palette:</b></td><td>" + card.qtPalette + "</td></tr>";
        info += "<tr><td><b>Icons:</b></td><td>" + card.iconTheme + "</td></tr>";
        info += "<tr><td><b>Cursor:</b></td><td>" + card.cursorTheme + "</td></tr>";
        info += "<tr><td><b>UI Font:</b></td><td>" + card.uiFont + "</td></tr>";
        info += "<tr><td><b>Panel Opacity:</b></td><td>" + card.panelOpacity + "%</td></tr>";
        info += "</table>";
        info += "<br><b>Color Scheme:</b>";
        info += "<table style=\"font-size:12px\">";
        info += "<tr><td>Active BG:</td><td style=\"background:" + card.activeBg + ";width:40px;height:20px\"></td><td>" + card.activeBg + "</td></tr>";
        info += "<tr><td>Active FG:</td><td style=\"background:" + card.activeFg + ";width:40px;height:20px\"></td><td>" + card.activeFg + "</td></tr>";
        info += "<tr><td>Inactive BG:</td><td style=\"background:" + card.inactiveBg + ";width:40px;height:20px\"></td><td>" + card.inactiveBg + "</td></tr>";
        info += "<tr><td>Accent:</td><td style=\"background:" + card.accent + ";width:40px;height:20px\"></td><td>" + card.accent + "</td></tr>";
        info += "<tr><td>Close btn:</td><td style=\"background:" + card.closeBtn + ";width:40px;height:20px\"></td><td>" + card.closeBtn + "</td></tr>";
        info += "<tr><td>Max btn:</td><td style=\"background:" + card.maxBtn + ";width:40px;height:20px\"></td><td>" + card.maxBtn + "</td></tr>";
        info += "</table>";

        previewLabel->setText(info);
        previewLabel->show();
    }

private:
    struct ThemeCard {
        QString name, filePath;
        QString metaName, description, accent;
        QString gtkTheme, wmTheme, qtStyle, qtPalette;
        QString iconTheme, cursorTheme, cursorSize;
        QString uiFont, monoFont;
        QString panelOpacity, panelBg;
        QString activeBg, activeFg, inactiveBg, inactiveFg;
        QString accent2, borderActive, borderInactive;
        QString closeBtn, maxBtn, minBtn;
    };

    void parseThemeProfile(ThemeCard &card) {
        QFile f(card.filePath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
        QString content = QString(f.readAll());
        f.close();

        QString section;
        QMap<QString, QString> data;

        for (const auto &line : content.split('\n')) {
            QString l = line.trimmed();
            if (l.isEmpty() || l.startsWith('#')) continue;

            QRegularExpression secRe("^\\[(\\w+)\\]$");
            QRegularExpressionMatch secM = secRe.match(l);
            if (secM.hasMatch()) {
                section = secM.captured(1);
                continue;
            }

            QRegularExpression kvRe("^(\\w+)=(.*)$");
            QRegularExpressionMatch kvM = kvRe.match(l);
            if (kvM.hasMatch()) {
                data[section + "." + kvM.captured(1)] = kvM.captured(2).trimmed();
            }
        };

        auto get = [&](const QString &key) -> QString {
            return data.value(key, "");
        };

        card.metaName = get("meta.name");
        card.description = get("meta.description");
        card.accent = get("meta.accent");
        card.gtkTheme = get("gtk.theme");
        card.wmTheme = get("wm.theme");
        card.qtStyle = get("qt.style");
        card.qtPalette = get("qt.palette");
        card.iconTheme = get("icons.theme");
        card.cursorTheme = get("cursor.theme");
        card.cursorSize = get("cursor.size");
        card.uiFont = get("fonts.ui");
        card.monoFont = get("fonts.mono");
        card.panelOpacity = get("panel.opacity");
        card.panelBg = get("panel.background");
        card.activeBg = get("colors.active_bg");
        card.activeFg = get("colors.active_fg");
        card.inactiveBg = get("colors.inactive_bg");
        card.inactiveFg = get("colors.inactive_fg");
        card.accent2 = get("meta.accent");
        card.borderActive = get("colors.border_active");
        card.borderInactive = get("colors.border_inactive");
        card.closeBtn = get("colors.button_close");
        card.maxBtn = get("colors.button_maximize");
        card.minBtn = get("colors.button_iconify");
    }

    void showThemeDetails(int idx) {
        if (idx < 0 || idx >= themeCards.size()) return;
        const ThemeCard &card = themeCards[idx];

        detailNameLabel->setText(card.metaName);
        detailDescLabel->setText(card.description);

        // Color swatches
        auto setSwatch = [](QLabel *label, const QString &color) {
            label->setStyleSheet(QString(
                "background-color: %1; border-radius: 4px; border: 1px solid #45475a;").arg(color));
            label->setToolTip(color);
        };
        setSwatch(activeBgSwatch, card.activeBg);
        setSwatch(activeFgSwatch, card.activeFg);
        setSwatch(inactiveBgSwatch, card.inactiveBg);
        setSwatch(inactiveFgSwatch, card.inactiveFg);
        setSwatch(accentSwatch, card.accent);
        setSwatch(closeSwatch, card.closeBtn);
        setSwatch(maxSwatch, card.maxBtn);
        setSwatch(minSwatch, card.minBtn);

        // Component summary
        componentLabel->setText(QString(
            "GTK: %1 | WM: %2 | Qt: %3 | Icons: %4 | Cursor: %5")
            .arg(card.gtkTheme, card.wmTheme, card.qtStyle, card.iconTheme, card.cursorTheme));

        detailWidget->show();
    }

    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(16, 16, 16, 16);

        auto *header = new QLabel("One-click global themes — applies GTK, WM, Qt, icons, cursor, panel, and fonts");
        header->setStyleSheet("font-size: 14px; color: #a6adc8;");
        layout->addWidget(header);

        // Current theme indicator
        currentThemeLabel = new QLabel("Active: none");
        currentThemeLabel->setStyleSheet(
            "background-color: #313244; color: #a6e3a1; padding: 8px 14px; "
            "border-radius: 6px; font-weight: bold;");
        layout->addWidget(currentThemeLabel);

        // Main split: list on left, details on right
        auto *splitter = new QSplitter(Qt::Horizontal);

        // Left: theme list
        auto *leftWidget = new QWidget();
        auto *leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(0, 0, 0, 0);

        themeList = new QListWidget();
        themeList->setStyleSheet(
            "QListWidget { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; font-size: 14px; }"
            "QListWidget::item { padding: 10px; border-bottom: 1px solid #313244; }"
            "QListWidget::item:selected { background-color: #313244; color: #89b4fa; font-weight: bold; }"
            "QListWidget::item:hover { background-color: #292a3a; }");
        connect(themeList, &QListWidget::currentRowChanged, this, &GlobalThemesTab::onThemeSelected);
        leftLayout->addWidget(themeList);

        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(8);

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(36);
        connect(refreshBtn, &QPushButton::clicked, this, &GlobalThemesTab::scanThemes);
        btnRow->addWidget(refreshBtn);

        btnRow->addStretch();
        leftLayout->addLayout(btnRow);

        splitter->addWidget(leftWidget);

        // Right: details panel
        auto *rightWidget = new QWidget();
        auto *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(8, 0, 0, 0);

        detailWidget = new QWidget();
        auto *detailLayout = new QVBoxLayout(detailWidget);
        detailLayout->setContentsMargins(0, 0, 0, 0);
        detailLayout->setSpacing(10);

        detailNameLabel = new QLabel();
        detailNameLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #cdd6f4;");
        detailLayout->addWidget(detailNameLabel);

        detailDescLabel = new QLabel();
        detailDescLabel->setStyleSheet("font-size: 13px; color: #a6adc8; font-style: italic;");
        detailDescLabel->setWordWrap(true);
        detailLayout->addWidget(detailDescLabel);

        // Color swatches
        auto *colorGroup = new QGroupBox("Color Palette");
        colorGroup->setStyleSheet(
            "QGroupBox { color: #89b4fa; font-weight: bold; font-size: 13px; "
            "border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding-top: 16px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }");
        auto *colorGrid = new QGridLayout(colorGroup);
        colorGrid->setSpacing(6);

        auto addSwatch = [&](int row, int col, const QString &name, QLabel *&swatch) {
            auto *label = new QLabel(name);
            label->setStyleSheet("color: #a6adc8; font-size: 11px;");
            colorGrid->addWidget(label, row * 2, col);
            swatch = new QLabel();
            swatch->setFixedSize(36, 24);
            swatch->setStyleSheet("background-color: #313244; border-radius: 4px;");
            colorGrid->addWidget(swatch, row * 2 + 1, col);
        };

        addSwatch(0, 0, "Active BG", activeBgSwatch);
        addSwatch(0, 1, "Active FG", activeFgSwatch);
        addSwatch(0, 2, "Inactive BG", inactiveBgSwatch);
        addSwatch(0, 3, "Inactive FG", inactiveFgSwatch);
        addSwatch(1, 0, "Accent", accentSwatch);
        addSwatch(1, 1, "Close", closeSwatch);
        addSwatch(1, 2, "Maximize", maxSwatch);
        addSwatch(1, 3, "Minimize", minSwatch);

        detailLayout->addWidget(colorGroup);

        // Component summary
        componentLabel = new QLabel();
        componentLabel->setStyleSheet(
            "background-color: #181825; color: #a6adc8; padding: 8px; "
            "border-radius: 4px; font-size: 12px;");
        componentLabel->setWordWrap(true);
        detailLayout->addWidget(componentLabel);

        // Action buttons
        auto *actionRow = new QHBoxLayout();
        actionRow->setSpacing(8);

        auto *applyBtn = new QPushButton("Apply Theme");
        applyBtn->setMinimumHeight(40);
        applyBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
            "padding: 10px 20px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }");
        connect(applyBtn, &QPushButton::clicked, this, &GlobalThemesTab::applySelectedTheme);
        actionRow->addWidget(applyBtn);

        auto *previewBtn = new QPushButton("Preview");
        previewBtn->setMinimumHeight(40);
        connect(previewBtn, &QPushButton::clicked, this, &GlobalThemesTab::previewTheme);
        actionRow->addWidget(previewBtn);

        actionRow->addStretch();
        detailLayout->addLayout(actionRow);

        rightLayout->addWidget(detailWidget);

        // Preview area (hidden by default)
        previewLabel = new QLabel();
        previewLabel->setStyleSheet(
            "background-color: #181825; border: 1px solid #313244; border-radius: 6px; "
            "padding: 12px; color: #cdd6f4;");
        previewLabel->setWordWrap(true);
        previewLabel->setTextFormat(Qt::RichText);
        previewLabel->hide();
        rightLayout->addWidget(previewLabel);

        rightLayout->addStretch();
        splitter->addWidget(rightWidget);

        splitter->setSizes({300, 500});
        layout->addWidget(splitter);

        detailWidget->hide();
    }

    QStatusBar *statusBar;
    QListWidget *themeList;
    QList<ThemeCard> themeCards;
    QLabel *currentThemeLabel;
    QWidget *detailWidget;
    QLabel *detailNameLabel;
    QLabel *detailDescLabel;
    QLabel *activeBgSwatch, *activeFgSwatch;
    QLabel *inactiveBgSwatch, *inactiveFgSwatch;
    QLabel *accentSwatch;
    QLabel *closeSwatch, *maxSwatch, *minSwatch;
    QLabel *componentLabel;
    QLabel *previewLabel;
};

// ============================================================
// Tab 8: Health Check
// ============================================================
class HealthTab : public QWidget {
    Q_OBJECT
public:
    explicit HealthTab(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
    }

    void runAll() {
        // Run each checker's health widget
        for (auto *hw : checkers) {
            hw->runCheck();
        }
    }

private:
    void setupUI() {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        auto *tabs = new QTabWidget();
        tabs->setStyleSheet(
            "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; "
            "background-color: #1e1e2e; }"
            "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 8px 14px; "
            "border: 1px solid #313244; border-bottom: none; "
            "border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
            "QTabBar::tab:hover { background-color: #313244; }");

        auto addTab = [&](HealthChecker *checker, const QString &title) {
            auto *w = new HealthCheckWidget(checker);
            checkers.append(w);
            tabs->addTab(w, title);
        };

        addTab(new EnvHealthChecker(this), "Environment");
        addTab(new ServiceHealthChecker(this), "Services");
        addTab(new WmHealthChecker(this), "Window Manager");
        addTab(new ThemeHealthChecker(this), "Theme");
        addTab(new NetworkHealthChecker(this), "Network");
        addTab(new AudioHealthChecker(this), "Audio");

        layout->addWidget(tabs);
    }

    QList<HealthCheckWidget *> checkers;
};

// ============================================================
// Main Window
// ============================================================
class ThemeManager : public QMainWindow {
    Q_OBJECT

public:
    ThemeManager(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Theme Manager");
        resize(820, 650);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setContentsMargins(12, 12, 12, 12);

        tabs = new QTabWidget();
        tabs->setTabPosition(QTabWidget::North);
        tabs->setStyleSheet(tabStyle());

        wmTab = new WmDecorationTab(statusBar());
        qtTab = new QtThemeTab(statusBar());
        gtkTab = new GtkThemeTab(statusBar());
        iconTab = new IconCursorTab(statusBar());
        panelTab = new PanelTab(statusBar());
        fontTab = new FontThemeTab(statusBar());
        globalTab = new GlobalThemesTab(statusBar());
        healthTab = new HealthTab();

        tabs->addTab(globalTab, "Global Themes");
        tabs->addTab(wmTab, "Window Decorations");
        tabs->addTab(qtTab, "Qt Theme");
        tabs->addTab(gtkTab, "GTK Theme");
        tabs->addTab(iconTab, "Icons & Cursors");
        tabs->addTab(panelTab, "Panel");
        tabs->addTab(fontTab, "Fonts");
        tabs->addTab(healthTab, "Health");

        mainLayout->addWidget(tabs);

        statusBar()->setStyleSheet(
            "background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");

        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 13px; }"
            "QLineEdit { background-color: #181825; border: 1px solid #313244; "
            "padding: 6px; border-radius: 4px; color: #cdd6f4; }"
            "QLineEdit:focus { border-color: #89b4fa; }"
            "QComboBox { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 6px 10px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
            "border-right: 5px solid transparent; border-top: 6px solid #a6adc8; margin-right: 8px; }"
            "QComboBox QAbstractItemView { background-color: #181825; color: #cdd6f4; "
            "selection-background-color: #313244; border: 1px solid #45475a; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 5px; "
            "padding: 8px 14px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }");

        statusBar()->showMessage("Ready");
        QTimer::singleShot(500, healthTab, &HealthTab::runAll);
    }

private:
    QString tabStyle() {
        return "QTabWidget::pane { border: 1px solid #313244; border-radius: 6px; "
               "background-color: #1e1e2e; }"
               "QTabBar::tab { background-color: #181825; color: #a6adc8; padding: 10px 18px; "
               "border: 1px solid #313244; border-bottom: none; "
               "border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
               "QTabBar::tab:selected { background-color: #1e1e2e; color: #cdd6f4; font-weight: bold; }"
               "QTabBar::tab:hover { background-color: #313244; }";
    }

    QTabWidget *tabs;
    WmDecorationTab *wmTab;
    QtThemeTab *qtTab;
    GtkThemeTab *gtkTab;
    IconCursorTab *iconTab;
    PanelTab *panelTab;
    FontThemeTab *fontTab;
    GlobalThemesTab *globalTab;
    HealthTab *healthTab;
};

// ============================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nv-theme-manager");
    ThemeManager gui;
    gui.show();
    return app.exec();
}

#include "main.moc"
