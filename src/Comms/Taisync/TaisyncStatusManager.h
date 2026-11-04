#pragma once

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonValue>
#include <QtQmlIntegration/qqmlintegration.h>

class TaisyncStatusManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TaisyncRadio)
    QML_SINGLETON
    Q_PROPERTY(bool connected READ connected NOTIFY metricsChanged)
    Q_PROPERTY(QString linkStatus READ linkStatus NOTIFY metricsChanged)
    Q_PROPERTY(int linkQuality READ linkQuality NOTIFY metricsChanged)
    Q_PROPERTY(int groundSnr READ groundSnr NOTIFY metricsChanged)
    Q_PROPERTY(int airSnr READ airSnr NOTIFY metricsChanged)
    Q_PROPERTY(int groundRssi0 READ groundRssi0 NOTIFY metricsChanged)
    Q_PROPERTY(int groundRssi1 READ groundRssi1 NOTIFY metricsChanged)
    Q_PROPERTY(int airRssi0 READ airRssi0 NOTIFY metricsChanged)
    Q_PROPERTY(int airRssi1 READ airRssi1 NOTIFY metricsChanged)
    Q_PROPERTY(QString mcs READ mcs NOTIFY metricsChanged)
    Q_PROPERTY(QString uplinkRate READ uplinkRate NOTIFY metricsChanged)
    Q_PROPERTY(QString downlinkRate READ downlinkRate NOTIFY metricsChanged)
    Q_PROPERTY(QString txFreq READ txFreq NOTIFY metricsChanged)

public:
    explicit TaisyncStatusManager(QObject *parent = nullptr);

    bool connected() const { return _connected; }
    QString linkStatus() const { return _linkStatus; }
    int linkQuality() const { return _linkQuality; }
    int groundSnr() const { return _groundSnr; }
    int airSnr() const { return _airSnr; }
    int groundRssi0() const { return _groundRssi0; }
    int groundRssi1() const { return _groundRssi1; }
    int airRssi0() const { return _airRssi0; }
    int airRssi1() const { return _airRssi1; }
    QString mcs() const { return _mcs; }
    QString uplinkRate() const { return _uplinkRate; }
    QString downlinkRate() const { return _downlinkRate; }
    QString txFreq() const { return _txFreq; }

signals:
    void metricsChanged();

private slots:
    void _readLogcat();

private:
    static int _parseFlexInt(const QJsonValue &val, int defaultVal = 0);
    void _parseJsonPayload(const QByteArray &jsonBytes);

    QProcess* _logcatProcess = nullptr;

    bool _connected = false;
    QString _linkStatus = QStringLiteral("Disconnected");
    int _linkQuality = 0;
    int _groundSnr = 0;
    int _airSnr = 0;
    int _groundRssi0 = 0;
    int _groundRssi1 = 0;
    int _airRssi0 = 0;
    int _airRssi1 = 0;
    QString _mcs = QStringLiteral("--");
    QString _uplinkRate = QStringLiteral("0");
    QString _downlinkRate = QStringLiteral("0");
    QString _txFreq = QStringLiteral("--");
};