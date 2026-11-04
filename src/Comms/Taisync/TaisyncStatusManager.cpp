#include "TaisyncStatusManager.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrl>

TaisyncStatusManager::TaisyncStatusManager(QObject *parent)
    : QObject(parent)
      , _networkManager(new QNetworkAccessManager(this))
      , _pollTimer(new QTimer(this))
{
    connect(_pollTimer, &QTimer::timeout, this, &TaisyncStatusManager::_pollBaseband);
    _pollTimer->start(1000); // 1 Hz polling
}

void TaisyncStatusManager::setRadioIp(const QString &ip)
{
    _radioIp = ip;
}

void TaisyncStatusManager::_pollBaseband()
{
    QUrl url(QStringLiteral("http://%1/v2/baseband.json").arg(_radioIp));
    QNetworkRequest request(url);

    // Drop the request if the radio takes more than 800ms to respond to prevent queue backlog
    request.setTransferTimeout(800);

    QNetworkReply *reply = _networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            // Force this to print so we can see the exact failure!
            qDebug() << "Taisync HTTP Error (" << reply->error() << "):" << reply->errorString();

            _connected = false;
            _linkStatus = QStringLiteral("Offline");
            _linkQuality = 0;
            emit metricsChanged();
            return;
        }

        _parseJsonPayload(reply->readAll());
    });
}

int TaisyncStatusManager::_parseFlexInt(const QJsonValue &val, int defaultVal)
{
    if (val.isDouble()) {
        return val.toInt(defaultVal);
    }
    if (val.isString()) {
        QString s = val.toString();
        s = s.remove('%').remove("dBm").trimmed();
        bool ok = false;
        int parsed = s.toInt(&ok);
        return ok ? parsed : defaultVal;
    }
    return defaultVal;
}

void TaisyncStatusManager::_parseJsonPayload(const QByteArray &jsonBytes)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    QJsonObject obj = doc.object();
    _connected = true;

    if (obj.contains("groundLinkStatus")) {
        _linkStatus = obj.value("groundLinkStatus").toString();
        _linkQuality = _parseFlexInt(obj.value("groundLinkQuality"));
        _groundSnr = _parseFlexInt(obj.value("groundSnr"));
        _airSnr = _parseFlexInt(obj.value("airSnr"));
        _groundRssi0 = _parseFlexInt(obj.value("groundRSSI0"));
        _groundRssi1 = _parseFlexInt(obj.value("groundRSSI1"));
        _airRssi0 = _parseFlexInt(obj.value("airRSSI0"));
        _airRssi1 = _parseFlexInt(obj.value("airRSSI1"));
    } else {
        _linkStatus = obj.value("linkStats").toString(obj.value("airLinkStatus").toString("Unknown"));
        _linkQuality = _parseFlexInt(obj.value("linkQuality"), _parseFlexInt(obj.value("airLinkQuality")));
        _groundSnr = _parseFlexInt(obj.value("snr"), _parseFlexInt(obj.value("airSnr")));
        _groundRssi0 = _parseFlexInt(obj.value("rssi0"), _parseFlexInt(obj.value("rssi")));
        _groundRssi1 = _parseFlexInt(obj.value("rssi1"));
    }

    _mcs = obj.value("MCS").toString(obj.value("mode").toString("--"));
    _uplinkRate = obj.value("uplinkDataRate").toString("0");
    _downlinkRate = obj.value("downlinkDataRate").toString("0");

    emit metricsChanged();
}