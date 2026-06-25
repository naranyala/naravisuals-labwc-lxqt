#include "naravisuals-weather-widget.h"
#include <QVBoxLayout>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>

NaravisualsWeatherWidget::NaravisualsWeatherWidget(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent)
    : QObject(parent)
    , ILXQtPanelPlugin(startupInfo)
    , mLabel(new QLabel("--"))
    , mNetManager(new QNetworkAccessManager(this))
    , mCity(settings()->value("city", "London").toString())
    , mInterval(settings()->value("interval", 600).toInt())
{
    mLabel->setAlignment(Qt::AlignCenter);
    mLabel->setMinimumWidth(80);
    mLabel->setStyleSheet("QLabel { padding: 2px 6px; }");

    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &NaravisualsWeatherWidget::fetchWeather);
    mTimer->start(mInterval * 1000);

    connect(mNetManager, &QNetworkAccessManager::finished, this, &NaravisualsWeatherWidget::onReplyFinished);

    fetchWeather();
}

void NaravisualsWeatherWidget::fetchWeather() {
    QString url = QString("https://wttr.in/%1?format=j1").arg(mCity);
    mNetManager->get(QNetworkRequest(QUrl(url)));
}

void NaravisualsWeatherWidget::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        mLabel->setText("--");
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();

    QJsonObject current = root["current_condition"].toArray().first().toObject();
    double tempC = current["temp_C"].toString().toDouble();
    QString desc = current["weatherDesc"].toArray().first().toObject()["value"].toString();
    int humidity = current["humidity"].toString().toInt();

    QString icon = tempToIcon(tempC);
    mLabel->setText(QString("%1 %2°C %3").arg(icon).arg(static_cast<int>(tempC)).arg(desc.left(12)));
    mLabel->setToolTip(QString("City: %1\nTemp: %2°C\nDesc: %3\nHumidity: %4%")
        .arg(mCity).arg(static_cast<int>(tempC)).arg(desc).arg(humidity));

    reply->deleteLater();
}

QString NaravisualsWeatherWidget::tempToIcon(double temp) const {
    if (temp >= 35) return "🌡";
    if (temp >= 25) return "☀";
    if (temp >= 15) return "⛅";
    if (temp >= 5) return "☁";
    return "❄";
}

QDialog *NaravisualsWeatherWidget::configureDialog() {
    auto *dlg = new QDialog();
    dlg->setWindowTitle("Weather Settings");
    dlg->resize(300, 120);

    auto *layout = new QFormLayout(dlg);

    auto *cityEdit = new QLineEdit(mCity);
    layout->addRow("City:", cityEdit);

    auto *intervalSpin = new QSpinBox();
    intervalSpin->setRange(60, 7200);
    intervalSpin->setValue(mInterval);
    intervalSpin->setSuffix(" sec");
    layout->addRow("Refresh:", intervalSpin);

    auto *saveBtn = new QPushButton("Save");
    connect(saveBtn, &QPushButton::clicked, this, [this, dlg, cityEdit, intervalSpin]() {
        mCity = cityEdit->text();
        mInterval = intervalSpin->value();
        settings()->setValue("city", mCity);
        settings()->setValue("interval", mInterval);
        mTimer->setInterval(mInterval * 1000);
        fetchWeather();
        dlg->accept();
    });
    layout->addRow(saveBtn);

    return dlg;
}

void NaravisualsWeatherWidget::settingsChanged() {
    mCity = settings()->value("city", "London").toString();
    mInterval = settings()->value("interval", 600).toInt();
    mTimer->setInterval(mInterval * 1000);
    fetchWeather();
}

#include "naravisuals-weather-widget.moc"
