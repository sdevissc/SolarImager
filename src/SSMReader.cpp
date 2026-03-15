#include "SSMReader.h"
#include <QDebug>

// Static pattern: matches A0.530$ B1.230$ C0.800$ etc.
const QRegularExpression SSMReader::s_pattern(R"(([ABC])(\d+\.\d+)\$)");

SSMReader::SSMReader(const QString &port, int baud, QObject *parent)
    : QThread(parent), m_port(port), m_baud(baud)
{}

void SSMReader::stop()
{
    m_stop = true;
}

void SSMReader::run()
{
    QSerialPort serial;
    serial.setPortName(m_port);
    serial.setBaudRate(m_baud);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!serial.open(QIODevice::ReadOnly)) {
        emit error(QString("Cannot open %1: %2")
                   .arg(m_port, serial.errorString()));
        return;
    }

    emit connected();

    QString buf;
    double  inputVal = 0.0;

    while (!m_stop) {
        if (!serial.waitForReadyRead(200))
            continue;

        buf += QString::fromLatin1(serial.readAll());

        // Parse all complete tokens
        QRegularExpressionMatchIterator it = s_pattern.globalMatch(buf);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            const QString label = m.captured(1);
            const double  val   = m.captured(2).toDouble();

            emit rawLine(label + m.captured(2) + "$");

            if (label == "A")
                emit newSample(inputVal, val);   // seeing on A
            else if (label == "B")
                inputVal = val;                  // input level on B
        }

        // Keep only the incomplete tail after the last '$'
        int last = buf.lastIndexOf('$');
        if (last >= 0)
            buf = buf.mid(last + 1);
        if (buf.size() > 256)
            buf.clear();
    }

    serial.close();
    emit disconnected();
}
