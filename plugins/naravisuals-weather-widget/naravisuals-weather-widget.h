#ifndef NARAVISUALS_WEATHER_WIDGET_H
#define NARAVISUALS_WEATHER_WIDGET_H

#include <QLabel>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <ilxqtpanelplugin.h>

class NaravisualsWeatherWidget : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsWeatherWidget(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr);
    ~NaravisualsWeatherWidget() override = default;

    ILXQtPanelPlugin::Flags flags() const override { return PreferRightAlignment | HaveConfigDialog; }
    QString themeId() const override { return QStringLiteral("NaravisualsWeatherWidget"); }
    QWidget *widget() override { return mLabel; }
    QDialog *configureDialog() override;
    void settingsChanged() override;

private slots:
    void fetchWeather();
    void onReplyFinished(QNetworkReply *reply);

private:
    QLabel *mLabel;
    QTimer *mTimer;
    QNetworkAccessManager *mNetManager;
    QString mCity;
    int mInterval;

    QString tempToIcon(double temp) const;
};

class NaravisualsWeatherWidgetLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsWeatherWidget(startupInfo);
    }
};

#endif // NARAVISUALS_WEATHER_WIDGET_H
