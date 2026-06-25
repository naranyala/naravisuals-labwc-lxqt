#include <QLabel>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <ilxqtpanelplugin.h>

class NaravisualsCpuMonitor : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsCpuMonitor(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent)
        , ILXQtPanelPlugin(startupInfo)
        , mLabel(new QLabel("CPU --%"))
        , mShowMem(settings()->value("showMem", true).toBool())
    {
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(90);
        mLabel->setStyleSheet("QLabel { padding: 2px 6px; }");

        mTimer = new QTimer(this);
        connect(mTimer, &QTimer::timeout, this, &NaravisualsCpuMonitor::updateStats);
        mTimer->start(2000);

        updateStats();
    }

    ILXQtPanelPlugin::Flags flags() const override { return PreferRightAlignment | HaveConfigDialog; }
    QString themeId() const override { return QStringLiteral("NaravisualsCpuMonitor"); }
    QWidget *widget() override { return mLabel; }

    void settingsChanged() override {
        mShowMem = settings()->value("showMem", true).toBool();
        updateStats();
    }

private slots:
    void updateStats() {
        double cpu = readCpuUsage();
        double mem = readMemUsage();

        if (mShowMem) {
            mLabel->setText(QString("CPU %1% | RAM %2%")
                .arg(static_cast<int>(cpu))
                .arg(static_cast<int>(mem)));
        } else {
            mLabel->setText(QString("CPU %1%").arg(static_cast<int>(cpu)));
        }

        mLabel->setToolTip(QString("CPU: %1%\nMemory: %2%")
            .arg(cpu, 0, 'f', 1)
            .arg(mem, 0, 'f', 1));
    }

private:
    double readCpuUsage() {
        QFile stat("/proc/stat");
        if (!stat.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;

        QString line = stat.readLine();
        stat.close();

        QStringList vals = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (vals.size() < 5) return 0;

        quint64 user = vals[1].toULongLong();
        quint64 nice = vals[2].toULongLong();
        quint64 system = vals[3].toULongLong();
        quint64 idle = vals[4].toULongLong();
        quint64 iowait = vals.size() > 5 ? vals[5].toULongLong() : 0;
        quint64 irq = vals.size() > 6 ? vals[6].toULongLong() : 0;
        quint64 softirq = vals.size() > 7 ? vals[7].toULongLong() : 0;

        quint64 total = user + nice + system + idle + iowait + irq + softirq;
        quint64 active = total - idle - iowait;

        quint64 deltaTotal = total - mPrevTotal;
        quint64 deltaActive = active - mPrevActive;

        mPrevTotal = total;
        mPrevActive = active;

        if (deltaTotal == 0) return 0;
        return (static_cast<double>(deltaActive) / deltaTotal) * 100.0;
    }

    double readMemUsage() {
        QFile meminfo("/proc/meminfo");
        if (!meminfo.open(QIODevice::ReadOnly | QIODevice::Text)) return 0;

        quint64 total = 0, available = 0;
        while (!meminfo.atEnd()) {
            QString line = meminfo.readLine();
            if (line.startsWith("MemTotal:"))
                total = line.split(QRegularExpression("\\s+"))[1].toULongLong();
            else if (line.startsWith("MemAvailable:"))
                available = line.split(QRegularExpression("\\s+"))[1].toULongLong();
        }
        meminfo.close();

        if (total == 0) return 0;
        return ((total - available) / static_cast<double>(total)) * 100.0;
    }

    QLabel *mLabel;
    QTimer *mTimer;
    bool mShowMem;
    quint64 mPrevTotal = 0;
    quint64 mPrevActive = 0;
};

class NaravisualsCpuMonitorLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsCpuMonitor(startupInfo);
    }
};

#include "naravisuals-cpu-monitor.moc"
