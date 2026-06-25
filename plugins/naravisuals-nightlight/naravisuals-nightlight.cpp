#include <QLabel>
#include <QProcess>
#include <ilxqtpanelplugin.h>

class NaravisualsNightlight : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsNightlight(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent), ILXQtPanelPlugin(startupInfo)
    {
        mLabel = new QLabel();
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(30);
        mLabel->setCursor(Qt::PointingHandCursor);
        mLabel->setStyleSheet("QLabel { padding: 2px 4px; }");
        updateIcon();
    }

    ILXQtPanelPlugin::Flags flags() const override { return NoFlags; }
    QString themeId() const override { return QStringLiteral("NaravisualsNightlight"); }
    QWidget *widget() override { return mLabel; }

    void activated(ActivationReason reason) override {
        if (reason == Trigger || reason == DoubleClick) toggleNightlight();
    }

private:
    void toggleNightlight() {
        if (isRunning()) {
            QProcess::execute("pkill", {"wlsunset"});
        } else {
            QProcess::startDetached("wlsunset", {"-l", "51.5", "-L", "-0.12", "-t", "3500", "-T", "6500"});
        }
        updateIcon();
    }

    bool isRunning() {
        QProcess proc;
        proc.start("pgrep", {"-x", "wlsunset"});
        proc.waitForFinished(2000);
        return proc.exitCode() == 0;
    }

    void updateIcon() {
        if (isRunning()) {
            mLabel->setText("🌙");
            mLabel->setToolTip("Night Light: ON\nClick to disable");
        } else {
            mLabel->setText("☀");
            mLabel->setToolTip("Night Light: OFF\nClick to enable");
        }
    }

    QLabel *mLabel;
};

class NaravisualsNightlightLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsNightlight(startupInfo);
    }
};

#include "naravisuals-nightlight.moc"
