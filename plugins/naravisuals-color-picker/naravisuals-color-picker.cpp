#include <QLabel>
#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QTimer>
#include <QColor>
#include <ilxqtpanelplugin.h>

class NaravisualsColorPicker : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsColorPicker(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent), ILXQtPanelPlugin(startupInfo)
    {
        mLabel = new QLabel("🎨");
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(30);
        mLabel->setCursor(Qt::PointingHandCursor);
        mLabel->setStyleSheet("QLabel { padding: 2px 4px; }");
    }

    ILXQtPanelPlugin::Flags flags() const override { return HaveConfigDialog; }
    QString themeId() const override { return QStringLiteral("NaravisualsColorPicker"); }
    QWidget *widget() override { return mLabel; }

    void activated(ActivationReason reason) override {
        if (reason == Trigger || reason == DoubleClick) pickColor();
    }

    QDialog *configureDialog() override { pickColor(); return nullptr; }

private:
    void pickColor() {
        QProcess proc;
        proc.start("sh", {"-c", "xcolor -format '%[hex]'"});
        proc.waitForFinished(5000);
        QString hex = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

        if (!hex.isEmpty() && hex.startsWith('#')) {
            QApplication::clipboard()->setText(hex);
            mLabel->setToolTip("Last: " + hex);
            mLabel->setStyleSheet(QString(
                "QLabel { background-color: %1; border-radius: 4px; padding: 2px 4px; }").arg(hex));
            QTimer::singleShot(2000, this, [this]() {
                mLabel->setStyleSheet("QLabel { padding: 2px 4px; }");
            });
        }
    }

    QLabel *mLabel;
};

class NaravisualsColorPickerLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsColorPicker(startupInfo);
    }
};

#include "naravisuals-color-picker.moc"
