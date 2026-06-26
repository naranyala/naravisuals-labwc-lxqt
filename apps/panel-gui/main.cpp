#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <unistd.h>

// ─── Theme constants ────────────────────────────────────────────────────────

static const QString BG       = "#1e1e2e";
static const QString BG_DARK  = "#181825";
static const QString SURFACE  = "#313244";
static const QString OVERLAY  = "#45475a";
static const QString TEXT     = "#cdd6f4";
static const QString SUBTEXT  = "#a6adc8";
static const QString BLUE     = "#89b4fa";
static const QString GREEN    = "#a6e3a1";
static const QString RED      = "#f38ba8";
static const QString YELLOW   = "#f9e2af";
static const QString MAUVE    = "#cba6f7";

static const QString STYLE_BTN = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(BLUE, BG, MAUVE, SURFACE);

static const QString STYLE_BTN_DANGER = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(RED, BG, "#f5a8c0", "#d16b8a");

static const QString STYLE_BTN_SUCCESS = QString(
    "QPushButton { background-color: %1; color: %2; border-radius: 6px; padding: 8px 16px; font-weight: bold; border: none; }"
    "QPushButton:hover { background-color: %3; }"
    "QPushButton:pressed { background-color: %4; }"
).arg(GREEN, BG, "#b4f9b8", "#89b885");

static const QString STYLE_LIST = QString(
    "QListWidget { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 4px; }"
    "QListWidget::item { padding: 8px; border-radius: 4px; margin: 2px; }"
    "QListWidget::item:selected { background-color: %4; color: %5; }"
    "QListWidget::item:hover { background-color: %3; }"
).arg(BG_DARK, TEXT, SURFACE, BLUE, BG);

static const QString STYLE_TABLE = QString(
    "QTableWidget { background-color: %1; color: %2; gridline-color: %3; border: 1px solid %4; border-radius: 6px; }"
    "QTableWidget::item { padding: 6px; }"
    "QTableWidget::item:selected { background-color: %3; color: %5; }"
    "QHeaderView::section { background-color: %6; color: %7; padding: 6px; border: 1px solid %3; font-weight: bold; }"
).arg(BG, TEXT, SURFACE, OVERLAY, BLUE, BG_DARK, SUBTEXT);

static const QString STYLE_COMBO = QString(
    "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px; }"
    "QComboBox::drop-down { border: none; }"
    "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; }"
).arg(BG, TEXT, SURFACE, BLUE);

static const QString STYLE_INPUT = QString(
    "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px; }"
    "QLineEdit:focus { border: 1px solid %4; }"
).arg(BG, TEXT, SURFACE, BLUE);

static const QString STYLE_SPIN = QString(
    "QSpinBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 6px; padding: 6px; }"
).arg(BG, TEXT, SURFACE);

// ─── Known lxqt-panel plugins ───────────────────────────────────────────────

struct PluginInfo {
    QString id;
    QString name;
    QString description;
};

static const QVector<PluginInfo> KNOWN_PLUGINS = {
    {"fancymenu",       "Fancy Menu",       "Application launcher menu"},
    {"desktopswitch",   "Desktop Switcher",  "Virtual desktop pager"},
    {"quicklaunch",     "Quick Launch",      "Application shortcuts dock"},
    {"taskbar",         "Taskbar",           "Running window list"},
    {"spacer",          "Spacer",            "Flexible spacing"},
    {"statusnotifier",  "Status Notifier",   "SNI system tray"},
    {"tray",            "Legacy Tray",       "XEmbed system tray"},
    {"volume",          "Volume",            "Audio volume control"},
    {"worldclock",      "World Clock",       "Date and time display"},
    {"showdesktop",     "Show Desktop",      "Minimize all windows button"},
    {"clock",           "Clock",             "Simple clock display"},
    {"colorpicker",     "Color Picker",      "Screen color sampler"},
    {"cpuwatch",        "CPU Monitor",       "CPU usage graph"},
    {"customcmd",       "Custom Command",    "Run arbitrary command"},
    {"directorymenu",   "Directory Menu",    "Browse directory tree"},
    {"miniclock",       "Mini Clock",        "Compact clock"},
    {"mount",           "Mount",             "Mount/unmount devices"},
    {"networkmonitor",  "Network Monitor",   "Network traffic graph"},
    {"perfmonitor",     "Performance Monitor","System performance graphs"},
    {"weather",         "Weather",           "Weather information"},
};

// ─── Panel config parser ────────────────────────────────────────────────────

struct PanelWidget {
    QString id;
    QHash<QString, QString> settings;
};

struct PanelConfig {
    int panelCount;
    QHash<QString, QString> general;
    QHash<QString, QString> panelSettings;
    QVector<PanelWidget> widgets;
    QString rawPanelId;
};

class PanelParser {
public:
    static PanelConfig parse(const QString &filePath) {
        PanelConfig config;
        config.panelCount = 0;

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return config;

        QTextStream in(&file);
        QString currentSection;
        QHash<QString, QString> *currentTarget = nullptr;
        QVector<QPair<QString, QHash<QString, QString>>> sections;

        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
                continue;

            QRegularExpression reSection("^\\[(.+)\\]$");
            QRegularExpressionMatch match = reSection.match(line);
            if (match.hasMatch()) {
                currentSection = match.captured(1);
                sections.append({currentSection, {}});
                currentTarget = &sections.last().second;
                continue;
            }

            int eq = line.indexOf('=');
            if (eq > 0 && currentTarget) {
                QString key = line.left(eq).trimmed();
                QString val = line.mid(eq + 1).trimmed();
                currentTarget->insert(key, val);
            }
        }
        file.close();

        // Extract [General]
        for (auto &[name, vals] : sections) {
            if (name == "General") {
                config.general = vals;
                bool ok;
                config.panelCount = vals.value("panels", "panel1").split(',').count();
            }
        }

        // Find first panel section
        for (auto &[name, vals] : sections) {
            if (name.startsWith("panel")) {
                config.rawPanelId = name;
                config.panelSettings = vals;
                break;
            }
        }

        // Parse plugin list from panel settings
        QString pluginsStr = config.panelSettings.value("plugins", "");
        QStringList pluginIds = pluginsStr.split(',', Qt::SkipEmptyParts);

        // Match plugin IDs to their config sections
        QHash<QString, QHash<QString, QString>> pluginSections;
        for (auto &[name, vals] : sections) {
            if (name != "General" && !name.startsWith("panel"))
                pluginSections[name] = vals;
        }

        for (const QString &pid : pluginIds) {
            PanelWidget w;
            w.id = pid.trimmed();
            if (pluginSections.contains(w.id))
                w.settings = pluginSections[w.id];
            else
                w.settings["type"] = w.id;
            config.widgets.append(w);
        }

        return config;
    }

    static bool save(const QString &filePath, const PanelConfig &config) {
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&file);
        out << "[General]\n";
        for (auto it = config.general.constBegin(); it != config.general.constEnd(); ++it)
            out << it.key() << "=" << it.value() << "\n";

        out << "\n[" << config.rawPanelId << "]\n";
        for (auto it = config.panelSettings.constBegin(); it != config.panelSettings.constEnd(); ++it)
            out << it.key() << "=" << it.value() << "\n";

        for (const PanelWidget &w : config.widgets) {
            out << "\n[" << w.id << "]\n";
            for (auto it = w.settings.constBegin(); it != w.settings.constEnd(); ++it)
                out << it.key() << "=" << it.value() << "\n";
        }

        file.close();
        return true;
    }
};

// ─── Main Window ────────────────────────────────────────────────────────────

class PanelManagerWindow : public QMainWindow {
    Q_OBJECT
public:
    PanelManagerWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("NaraVisuals — Panel Manager");
        setMinimumSize(900, 600);
        resize(1000, 650);

        configPath = QDir::homePath() + "/.config/lxqt/panel.conf";

        setupUi();
        loadConfig();
    }

private slots:
    void onWidgetSelected(int row) {
        if (row < 0 || row >= config.widgets.size()) {
            settingsArea->setEnabled(false);
            settingsTitle->setText("Select a widget");
            clearSettingsForm();
            return;
        }

        settingsArea->setEnabled(true);
        PanelWidget &w = config.widgets[row];
        QString pluginName = getPluginName(w.id);
        settingsTitle->setText(pluginName + " — Settings");
        buildSettingsForm(w);
    }

    void onAddWidget() {
        QStringList available;
        for (const PluginInfo &p : KNOWN_PLUGINS) {
            bool exists = false;
            for (const PanelWidget &w : config.widgets) {
                if (w.id == p.id) { exists = true; break; }
            }
            if (!exists) available.append(p.id + " — " + p.name);
        }

        if (available.isEmpty()) {
            QMessageBox::information(this, "Add Widget", "All known plugins are already on the panel.");
            return;
        }

        bool ok;
        QString choice = QInputDialog::getItem(this, "Add Widget", "Choose a plugin:", available, 0, false, &ok);
        if (!ok || choice.isEmpty()) return;

        QString pluginId = choice.split(" — ").first().trimmed();

        PanelWidget w;
        w.id = pluginId;
        w.settings["type"] = pluginId;
        w.settings["alignment"] = "Left";
        config.widgets.append(w);

        // Update panel plugins string
        syncPluginsString();
        refreshList();
        saveConfig();
    }

    void onRemoveWidget() {
        int row = widgetList->currentRow();
        if (row < 0) return;

        QString id = config.widgets[row].id;
        int ret = QMessageBox::question(this, "Remove Widget",
            "Remove \"" + getPluginName(id) + "\" from the panel?");
        if (ret != QMessageBox::Yes) return;

        config.widgets.removeAt(row);
        syncPluginsString();
        refreshList();
        saveConfig();
        clearSettingsForm();
    }

    void onMoveUp() {
        int row = widgetList->currentRow();
        if (row <= 0) return;
        config.widgets.swapItemsAt(row, row - 1);
        syncPluginsString();
        refreshList();
        widgetList->setCurrentRow(row - 1);
        saveConfig();
    }

    void onMoveDown() {
        int row = widgetList->currentRow();
        if (row < 0 || row >= config.widgets.size() - 1) return;
        config.widgets.swapItemsAt(row, row + 1);
        syncPluginsString();
        refreshList();
        widgetList->setCurrentRow(row + 1);
        saveConfig();
    }

    void onApplySettings() {
        int row = widgetList->currentRow();
        if (row < 0) return;
        applySettingsForm(config.widgets[row]);
        saveConfig();
        reloadPanel();
        QMessageBox::information(this, "Saved", "Panel configuration saved and reloaded.");
    }

    void onReloadPanel() {
        int row = widgetList->currentRow();
        if (row >= 0) applySettingsForm(config.widgets[row]);
        saveConfig();
        reloadPanel();
        QMessageBox::information(this, "Panel Reloaded", "Settings saved and panel restarted.");
    }

    void onRestartPanel() {
        reloadPanel();
        QMessageBox::information(this, "Panel Restarted", "lxqt-panel has been restarted.");
    }

    void onResetToDefault() {
        int ret = QMessageBox::question(this, "Reset Panel",
            "Reset panel to default configuration?\nThis will overwrite your current panel.conf.");
        if (ret != QMessageBox::Yes) return;

        QString stockPath = QDir::homePath() + "/.config/lxqt/panel-stock.conf";
        if (!QFile::exists(stockPath)) {
            QMessageBox::warning(this, "Error", "Stock config not found:\n" + stockPath);
            return;
        }

        QFile::remove(configPath);
        QFile::copy(stockPath, configPath);
        loadConfig();
        QMessageBox::information(this, "Reset", "Panel reset to stock configuration.\nRestart lxqt-panel to apply.");
    }

private:
    void setupUi() {
        auto *central = new QWidget(this);
        setCentralWidget(central);
        setStyleSheet(QString("QMainWindow, QWidget { background-color: %1; color: %2; }").arg(BG, TEXT));

        auto *mainLayout = new QHBoxLayout(central);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        auto *splitter = new QSplitter(Qt::Horizontal);
        splitter->setHandleWidth(2);

        // ─── Left panel: Widget list ───────────────────────────────────────
        auto *leftWidget = new QWidget();
        auto *leftLayout = new QVBoxLayout(leftWidget);
        leftLayout->setContentsMargins(12, 12, 8, 12);
        leftLayout->setSpacing(8);

        auto *titleLabel = new QLabel("Panel Widgets");
        titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; padding: 4px;").arg(MAUVE));
        leftLayout->addWidget(titleLabel);

        widgetList = new QListWidget();
        widgetList->setStyleSheet(STYLE_LIST);
        widgetList->setDragDropMode(QAbstractItemView::InternalMove);
        widgetList->setDefaultDropAction(Qt::MoveAction);
        connect(widgetList, &QListWidget::currentRowChanged, this, &PanelManagerWindow::onWidgetSelected);
        connect(widgetList->model(), &QAbstractItemModel::rowsMoved, this, [this]() { onListReordered(); });
        leftLayout->addWidget(widgetList);

        // Button row
        auto *btnRow = new QHBoxLayout();
        btnRow->setSpacing(6);

        auto *btnUp = new QPushButton("▲");
        btnUp->setToolTip("Move Up");
        btnUp->setStyleSheet(STYLE_BTN);
        btnUp->setFixedWidth(40);
        connect(btnUp, &QPushButton::clicked, this, &PanelManagerWindow::onMoveUp);

        auto *btnDown = new QPushButton("▼");
        btnDown->setToolTip("Move Down");
        btnDown->setStyleSheet(STYLE_BTN);
        btnDown->setFixedWidth(40);
        connect(btnDown, &QPushButton::clicked, this, &PanelManagerWindow::onMoveDown);

        auto *btnAdd = new QPushButton("+ Add");
        btnAdd->setStyleSheet(STYLE_BTN_SUCCESS);
        connect(btnAdd, &QPushButton::clicked, this, &PanelManagerWindow::onAddWidget);

        auto *btnRemove = new QPushButton("— Remove");
        btnRemove->setStyleSheet(STYLE_BTN_DANGER);
        connect(btnRemove, &QPushButton::clicked, this, &PanelManagerWindow::onRemoveWidget);

        btnRow->addWidget(btnUp);
        btnRow->addWidget(btnDown);
        btnRow->addStretch();
        btnRow->addWidget(btnAdd);
        btnRow->addWidget(btnRemove);
        leftLayout->addLayout(btnRow);

        // ─── Right panel: Settings editor ──────────────────────────────────
        auto *rightWidget = new QWidget();
        auto *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(8, 12, 12, 12);
        rightLayout->setSpacing(8);

        settingsTitle = new QLabel("Select a widget");
        settingsTitle->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; padding: 4px;").arg(MAUVE));
        rightLayout->addWidget(settingsTitle);

        auto *scrollArea = new QScrollArea();
        scrollArea->setWidgetResizable(true);
        scrollArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        settingsArea = new QWidget();
        settingsFormLayout = new QFormLayout(settingsArea);
        settingsFormLayout->setSpacing(8);
        settingsFormLayout->setContentsMargins(8, 8, 8, 8);
        scrollArea->setWidget(settingsArea);
        rightLayout->addWidget(scrollArea);

        auto *btnApply = new QPushButton("Save Settings");
        btnApply->setStyleSheet(STYLE_BTN_SUCCESS);
        connect(btnApply, &QPushButton::clicked, this, &PanelManagerWindow::onApplySettings);
        rightLayout->addWidget(btnApply);

        // ─── Bottom bar ───────────────────────────────────────────────────
        auto *bottomBar = new QWidget();
        bottomBar->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;").arg(BG_DARK, SURFACE));
        auto *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(12, 8, 12, 8);

        auto *btnRestart = new QPushButton("Restart Panel");
        btnRestart->setStyleSheet(STYLE_BTN);
        connect(btnRestart, &QPushButton::clicked, this, &PanelManagerWindow::onRestartPanel);

        auto *btnReload = new QPushButton("Save & Reload");
        btnReload->setStyleSheet(STYLE_BTN_SUCCESS);
        connect(btnReload, &QPushButton::clicked, this, &PanelManagerWindow::onReloadPanel);

        auto *btnReset = new QPushButton("Reset to Default");
        btnReset->setStyleSheet(STYLE_BTN_DANGER);
        connect(btnReset, &QPushButton::clicked, this, &PanelManagerWindow::onResetToDefault);

        auto *pathLabel = new QLabel(configPath);
        pathLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(SUBTEXT));

        bottomLayout->addWidget(btnReload);
        bottomLayout->addWidget(btnRestart);
        bottomLayout->addWidget(btnReset);
        bottomLayout->addStretch();
        bottomLayout->addWidget(pathLabel);

        splitter->addWidget(leftWidget);
        splitter->addWidget(rightWidget);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        mainLayout->addWidget(splitter);

        auto *outerLayout = new QVBoxLayout(central);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->setSpacing(0);
        outerLayout->addWidget(splitter);
        outerLayout->addWidget(bottomBar);
    }

    void loadConfig() {
        config = PanelParser::parse(configPath);
        refreshList();
    }

    void saveConfig() {
        PanelParser::save(configPath, config);
    }

    void refreshList() {
        widgetList->clear();
        for (const PanelWidget &w : config.widgets) {
            QString name = getPluginName(w.id);
            QString align = w.settings.value("alignment", "");
            QString label = name;
            if (!align.isEmpty()) label += "  (" + align + ")";
            widgetList->addItem(label);
        }
    }

    void onListReordered() {
        QVector<PanelWidget> newOrder;
        for (int i = 0; i < widgetList->count(); ++i) {
            QString text = widgetList->item(i)->text();
            // Extract plugin name from label "Plugin Name  (Left)"
            for (const PanelWidget &w : config.widgets) {
                if (text.startsWith(getPluginName(w.id))) {
                    newOrder.append(w);
                    break;
                }
            }
        }
        if (newOrder.size() == config.widgets.size()) {
            config.widgets = newOrder;
            syncPluginsString();
            saveConfig();
        }
    }

    void syncPluginsString() {
        QStringList ids;
        for (const PanelWidget &w : config.widgets)
            ids.append(w.id);
        config.panelSettings["plugins"] = ids.join(',');
    }

    void clearSettingsForm() {
        QLayoutItem *item;
        while ((item = settingsFormLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        settingsEditors.clear();
    }

    void buildSettingsForm(PanelWidget &w) {
        clearSettingsForm();

        for (auto it = w.settings.constBegin(); it != w.settings.constEnd(); ++it) {
            QString key = it.key();
            QString val = it.value();

            QLineEdit *edit = new QLineEdit(val);
            edit->setStyleSheet(STYLE_INPUT);
            settingsEditors[key] = edit;
            settingsFormLayout->addRow(key + ":", edit);
        }
    }

    void applySettingsForm(PanelWidget &w) {
        for (auto it = settingsEditors.constBegin(); it != settingsEditors.constEnd(); ++it) {
            w.settings[it.key()] = it.value()->text();
        }
    }

    void runToggleScript(const QString &flag) {
        QString scriptPath = QString(PROJECT_ROOT) + "/scripts/panel/toggle-lxqt-panel.sh";
        if (!QFile::exists(scriptPath)) {
            QMessageBox::warning(this, "Error", "toggle-lxqt-panel.sh not found:\n" + scriptPath);
            return;
        }
        QProcess::execute("bash", {scriptPath, flag});
    }

    bool isPanelRunning() {
        QProcess proc;
        proc.start("pidof", {"lxqt-panel"});
        proc.waitForFinished(1000);
        return proc.exitCode() == 0;
    }

    void killPanel() {
        QProcess::execute("pkill", {"-9", "lxqt-panel"});
        for (int i = 0; i < 20; i++) {
            if (!isPanelRunning()) break;
            usleep(100000);
        }
    }

    void startPanel() {
        QProcess::startDetached("lxqt-panel");
    }

    void reloadPanel() {
        killPanel();
        startPanel();
    }

    QString getPluginName(const QString &id) {
        for (const PluginInfo &p : KNOWN_PLUGINS) {
            if (p.id == id) return p.name;
        }
        return id;
    }

    // UI elements
    QListWidget *widgetList = nullptr;
    QLabel *settingsTitle = nullptr;
    QWidget *settingsArea = nullptr;
    QFormLayout *settingsFormLayout = nullptr;
    QHash<QString, QLineEdit*> settingsEditors;

    // Data
    QString configPath;
    PanelConfig config;
};

// ─── main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("nv-panel-gui");
    app.setApplicationVersion("1.0.0");

    PanelManagerWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"
