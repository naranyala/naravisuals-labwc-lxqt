#include <QLabel>
#include <QProcess>
#include <QDir>
#include <QDateTime>
#include <ilxqtpanelplugin.h>

class NaravisualsScreenshot : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsScreenshot(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent), ILXQtPanelPlugin(startupInfo)
    {
        mLabel = new QLabel("📷");
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(30);
        mLabel->setCursor(Qt::PointingHandCursor);
        mLabel->setStyleSheet("QLabel { padding: 2px 4px; }");
    }

    ILXQtPanelPlugin::Flags flags() const override { return HaveConfigDialog; }
    QString themeId() const override { return QStringLiteral("NaravisualsScreenshot"); }
    QWidget *widget() override { return mLabel; }

    void activated(ActivationReason reason) override {
        if (reason == Trigger || reason == DoubleClick) takeScreenshot();
        else if (reason == MiddleClick) takeAreaScreenshot();
    }

    QDialog *configureDialog() override { return nullptr; }

private:
    void takeScreenshot() {
        QString dir = QDir::homePath() + "/Pictures/Screenshots";
        QDir().mkpath(dir);
        QString file = dir + "/screenshot-" +
            QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".png";
        QProcess::startDetached("grim", {file});
        mLabel->setToolTip("Screenshot saved to:\n" + file);
    }

    void takeAreaScreenshot() {
        QString dir = QDir::homePath() + "/Pictures/Screenshots";
        QDir().mkpath(dir);
        QString file = dir + "/screenshot-" +
            QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".png";
        QProcess::startDetached("sh", {"-c",
            QString("grim -g \"$(slurp)\" %1").arg(file)});
        mLabel->setToolTip("Area screenshot saved to:\n" + file);
    }

    QLabel *mLabel;
};

class NaravisualsScreenshotLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsScreenshot(startupInfo);
    }
};

#include "naravisuals-screenshot.moc"
