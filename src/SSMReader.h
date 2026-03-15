#pragma once
#include <QThread>
#include <QSerialPort>
#include <QRegularExpression>

/// Reads the Airylab SSM serial stream in a background thread.
/// Protocol: continuous ASCII, tokens terminated by '$'
///   A<float>$  – seeing in arcsec
///   B<float>$  – input level (0.5–1.0 normal)
///   C<float>$  – unused
/// Baud: 115200, 8N1
class SSMReader : public QThread
{
    Q_OBJECT
public:
    explicit SSMReader(const QString &port, int baud = 115200,
                       QObject *parent = nullptr);
    void stop();

signals:
    void newSample(double inputLevel, double seeing);
    void rawLine(QString token);
    void connected();
    void disconnected();
    void error(QString msg);

protected:
    void run() override;

private:
    QString m_port;
    int     m_baud;
    bool    m_stop = false;

    static const QRegularExpression s_pattern;
};
