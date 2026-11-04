#include "TaisyncStatusManager.h"
#include <QJsonDocument>
#include <QDebug>

TaisyncStatusManager::TaisyncStatusManager(QObject *parent)
    : QObject(parent)
{
    _logcatProcess = new QProcess(this);
    connect(_logcatProcess, &QProcess::readyReadStandardOutput, this, &TaisyncStatusManager::_readLogcat);

    // Spawn logcat, strip timestamps (-v raw), and filter explicitly for the OEM tag
    _logcatProcess->start("logcat", QStringList() << "-v" << "raw" << "-s" << "TaisyncController:D");
}

void TaisyncStatusManager::_readLogcat()
{
    while (_logcatProcess->canReadLine()) {
        QByteArray line = _logcatProcess->readLine().trimmed();

        // 1. Filter out general logcat spam
        if (line.contains("getBaseBand onResponse")) {

            // 2. Find the strict boundaries of the JSON object
            int startIdx = line.indexOf('{');
            int endIdx = line.lastIndexOf('}');

            // 3. Ensure both brackets exist and the string hasn't been truncated
            if (startIdx != -1 && endIdx != -1 && endIdx > startIdx) {
                QByteArray jsonBytes = line.mid(startIdx, (endIdx - startIdx) + 1);
                _parseJsonPayload(jsonBytes);
            }
        }
    }
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
    if (obj.contains("CurTxFreq")) {
        _txFreq = QString::number(_parseFlexInt(obj.value("CurTxFreq"))) + " MHz";
    }

    emit metricsChanged();
}