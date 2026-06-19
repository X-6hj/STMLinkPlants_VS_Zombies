//
// LightSensorReader implementation
//

#include "LightSensorReader.h"
#include <QSerialPortInfo>
#include <QDebug>

const int LightSensorReader::BAUD_RATE = 115200;

LightSensorReader::LightSensorReader(QObject *parent)
    : QObject(parent),
      serialPort(new QSerialPort(this)),
      reconnectTimer(new QTimer(this)),
      dataRegex("Light ADC:\\s*(\\d+),\\s*Voltage:\\s*([\\d.]+)V"),
      currentlyNight(false),
      connected(false),
      lastAdc(0),
      lastVolts(0.0f),
      thresholdLow(1500),
      thresholdHigh(1700)
{
    reconnectTimer->setSingleShot(false);
    reconnectTimer->setInterval(2000);
    connect(reconnectTimer, &QTimer::timeout, this, &LightSensorReader::tryConnect);

    connect(serialPort, &QSerialPort::readyRead, this, &LightSensorReader::onReadyRead);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
            this, &LightSensorReader::onError);
}

LightSensorReader::~LightSensorReader()
{
    stop();
}

void LightSensorReader::start()
{
    tryConnect();
    reconnectTimer->start();
}

void LightSensorReader::stop()
{
    reconnectTimer->stop();
    closePort();
}

bool LightSensorReader::isConnected() const
{
    return connected && serialPort->isOpen();
}

bool LightSensorReader::nightMode() const
{
    return currentlyNight;
}

int LightSensorReader::lastAdcValue() const
{
    return lastAdc;
}

float LightSensorReader::lastVoltage() const
{
    return lastVolts;
}

void LightSensorReader::setNightThresholdLow(int value)
{
    thresholdLow = value;
}

void LightSensorReader::setNightThresholdHigh(int value)
{
    thresholdHigh = value;
}

void LightSensorReader::setPreferredPort(const QString &name)
{
    preferredPortName = name;
}

QString LightSensorReader::preferredPort() const
{
    return preferredPortName;
}

bool LightSensorReader::openPort(const QString &name)
{
    serialPort->setPortName(name);
    serialPort->setBaudRate(BAUD_RATE);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadOnly)) {
        qDebug() << "LightSensorReader: failed to open" << name << serialPort->errorString();
        return false;
    }

    qDebug() << "LightSensorReader: opened" << name;
    return true;
}

void LightSensorReader::closePort()
{
    if (serialPort->isOpen())
        serialPort->close();
    if (connected) {
        connected = false;
        emit connectionChanged(false);
        emit logMessage(tr("Light sensor disconnected"));
    }
}

static bool isLikelySensorPort(const QSerialPortInfo &info)
{
    QString desc = info.description().toUpper();
    QString manufacturer = info.manufacturer().toUpper();
    return desc.contains("CH340") || desc.contains("USB-SERIAL") ||
           manufacturer.contains("CH340") || manufacturer.contains("FTDI") ||
           manufacturer.contains("PROLIFIC");
}

void LightSensorReader::tryConnect()
{
    if (isConnected())
        return;

    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        emit logMessage(tr("No serial ports found"));
        return;
    }

    // Once we know which port the sensor is on (preferred or previously
    // successful), keep retrying only that port. Do not scan others.
    QString targetPort = !preferredPortName.isEmpty() ? preferredPortName : lastSuccessfulPortName;
    if (!targetPort.isEmpty()) {
        for (const QSerialPortInfo &info : ports) {
            if (info.portName() == targetPort) {
                if (!serialPort->isOpen()) {
                    if (!openPort(info.portName())) {
                        emit logMessage(tr("Sensor port %1 not available, retrying...").arg(targetPort));
                        return;
                    }
                }
                // Wait for the STM32 frame. readyRead will handle parsing
                // and mark us connected; if it doesn't, we leave the port
                // open and try again on the next timer tick.
                serialPort->waitForReadyRead(1500);
                return;
            }
        }
        emit logMessage(tr("Sensor port %1 disappeared, scanning...").arg(targetPort));
        // Fall through to auto-scan if the known port is gone.
    }

    // First-time auto-scan: preferred/CH340 first, then everything else.
    QList<QSerialPortInfo> candidates;
    for (const QSerialPortInfo &info : ports) {
        if (isLikelySensorPort(info))
            candidates.append(info);
    }
    for (const QSerialPortInfo &info : ports) {
        bool alreadyListed = false;
        for (const QSerialPortInfo &c : candidates) {
            if (c.portName() == info.portName()) {
                alreadyListed = true;
                break;
            }
        }
        if (!alreadyListed)
            candidates.append(info);
    }

    for (const QSerialPortInfo &info : candidates) {
        closePort();
        if (!openPort(info.portName()))
            continue;

        if (serialPort->waitForReadyRead(1500)) {
            if (connected)
                return;
        } else if (serialPort->bytesAvailable() == 0) {
            closePort();
        } else {
            return;
        }
    }

    if (!serialPort->isOpen())
        emit logMessage(tr("Light sensor not found, retrying..."));
}

void LightSensorReader::onReadyRead()
{
    readBuffer.append(serialPort->readAll());

    int pos;
    while ((pos = readBuffer.indexOf('\n')) != -1) {
        QByteArray line = readBuffer.left(pos);
        readBuffer.remove(0, pos + 1);
        if (line.endsWith('\r'))
            line.chop(1);
        processLine(line);
    }
}

void LightSensorReader::processLine(const QByteArray &line)
{
    QString str = QString::fromLatin1(line);
    QRegularExpressionMatch match = dataRegex.match(str);
    if (!match.hasMatch()) {
        emit logMessage(tr("Raw serial data (%1): %2").arg(serialPort->portName(), str));
        return;
    }

    int adc = match.captured(1).toInt();
    float volts = match.captured(2).toFloat();

    lastAdc = adc;
    lastVolts = volts;

    if (!connected) {
        connected = true;
        lastSuccessfulPortName = serialPort->portName();
        emit connectionChanged(true);
        emit logMessage(tr("Light sensor connected on %1").arg(serialPort->portName()));
    }

    emit logMessage(tr("ADC=%1 Voltage=%2V").arg(adc).arg(volts, 0, 'f', 3));
    emit dataReceived(adc, volts);

    // Hysteresis: for this light sensor, higher ADC means darker.
    // Switch to night above the high threshold, back to day below the low threshold.
    bool night = currentlyNight;
    if (adc > thresholdHigh)
        night = true;
    else if (adc < thresholdLow)
        night = false;

    if (night != currentlyNight) {
        currentlyNight = night;
        emit nightModeChanged(night);
        emit logMessage(night ? tr("Night mode detected") : tr("Day mode detected"));
    }
}

void LightSensorReader::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError)
        return;

    // Ignore transient/spurious errors; only close the port on serious ones.
    if (error != QSerialPort::ResourceError &&
        error != QSerialPort::PermissionError &&
        error != QSerialPort::NotOpenError) {
        qDebug() << "LightSensorReader serial error (ignored):" << error;
        return;
    }

    qDebug() << "LightSensorReader serial error:" << error;
    closePort();
}
