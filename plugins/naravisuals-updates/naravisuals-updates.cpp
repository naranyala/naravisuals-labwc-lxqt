#include <QLabel>
#include <QTimer>
#include <QProcess>
#include <ilxqtpanelplugin.h>

class NaravisualsUpdates : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsUpdates(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent), ILXQtPanelPlugin(startupInfo), mUpdateCount(0)
    {
        mLabel = new QLabel("📦 0");
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(50);
        mLabel->setCursor(Qt::PointingHandCursor);
        mLabel->setStyleSheet("QLabel { padding: 2px 6px; }");

        mTimer = new QTimer(this);
        connect(mTimer, &QTimer::timeout, this, &NaravisualsUpdates::checkUpdates);
        mTimer->start(3600000);

        checkUpdates();
    }

    ILXQtPanelPlugin::Flags flags() const override { return NoFlags; }
    QString themeId() const override { return QStringLiteral("NaravisualsUpdates"); }
    QWidget *widget() override { return mLabel; }

    void activated(ActivationReason reason) override {
        if (reason == Trigger || reason == DoubleClick) {
            QProcess::startDetached("bash", {"-c",
                "pkexec apt update && x-terminal-emulator -e 'bash -c \"sudo apt upgrade; read\"'"});
        } else if (reason == MiddleClick) {
            checkUpdates();
        }
    }

private slots:
    void checkUpdates() {
        QProcess proc;
        proc.start("apt", {"list", "--upgradable"});
        proc.waitForFinished(30000);
        QString output = QString::fromUtf8(proc.readAllStandardOutput());

        int count = 0;
        for (const QString &line : output.split('\n')) {
            if (line.contains("upgradable") || (line.contains('/') && line.contains('@'))) count++;
        }

        mUpdateCount = count;
        if (count > 0) {
            mLabel->setText(QString("📦 %1").arg(count));
            mLabel->setStyleSheet("QLabel { padding: 2px 6px; color: #fab387; font-weight: bold; }");
            mLabel->setToolTip(QString("%1 package updates available\nClick to update").arg(count));
        } else {
            mLabel->setText("📦 0");
            mLabel->setStyleSheet("QLabel { padding: 2px 6px; }");
            mLabel->setToolTip("System is up to date\nClick to check again");
        }
    }

private:
    QLabel *mLabel;
    QTimer *mTimer;
    int mUpdateCount;
};

class NaravisualsUpdatesLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsUpdates(startupInfo);
    }
};

#include "naravisuals-updates.moc"
