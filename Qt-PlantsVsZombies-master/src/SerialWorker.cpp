//
// SerialWorker implementation
// 所有串口 I/O 操作在独立工作线程中执行，确保 UI 线程不阻塞。
//

#include "SerialWorker.h"
#include <QSerialPortInfo>
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent),
      serialPort(new QSerialPort(this)),
      portTimeoutTimer(new QTimer(this)),
      dataRegex("Light ADC:\\s*(\\d+),\\s*Voltage:\\s*([\\d.]+)V"),
      connected(false),
      lastAdc(0),
      lastVolts(0.0f),
      scanIndex(0)
{
    // 每端口超时：如果在 PORT_TIMEOUT_MS 内没有收到有效数据，切换到下一个端口
    portTimeoutTimer->setSingleShot(true);
    portTimeoutTimer->setInterval(PORT_TIMEOUT_MS);
    connect(portTimeoutTimer, &QTimer::timeout, this, &SerialWorker::onPortTimeout);

    connect(serialPort, &QSerialPort::readyRead, this, &SerialWorker::onReadyRead);
    connect(serialPort, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
            this, &SerialWorker::onError);
}

SerialWorker::~SerialWorker()
{
    stop();
}

void SerialWorker::startScan(const QList<QSerialPortInfo> &ports)
{
    scanIndex = 0;
    candidatePorts.clear();

    if (ports.isEmpty()) {
        emit logMessage(tr("No serial ports available, light sensor disabled"));
        emit scanFinished();
        return;
    }

    // 优先排列 CH340 / USB-TTL 适配器
    auto isLikelySensor = [](const QSerialPortInfo &info) -> bool {
        QString desc = info.description().toUpper();
        QString manufacturer = info.manufacturer().toUpper();
        return desc.contains("CH340") || desc.contains("USB-SERIAL") ||
               manufacturer.contains("CH340") || manufacturer.contains("FTDI") ||
               manufacturer.contains("PROLIFIC");
    };

    for (const QSerialPortInfo &info : ports) {
        if (isLikelySensor(info))
            candidatePorts.append(info);
    }
    for (const QSerialPortInfo &info : ports) {
        bool alreadyListed = false;
        for (const QSerialPortInfo &c : candidatePorts) {
            if (c.portName() == info.portName()) {
                alreadyListed = true;
                break;
            }
        }
        if (!alreadyListed)
            candidatePorts.append(info);
    }

    // 开始异步扫描
    tryNextCandidate();
}

void SerialWorker::stop()
{
    portTimeoutTimer->stop();
    closePort();
}

bool SerialWorker::openPort(const QString &name)
{
    serialPort->setPortName(name);
    serialPort->setBaudRate(BAUD_RATE);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadOnly)) {
        qDebug() << "SerialWorker: failed to open" << name << serialPort->errorString();
        return false;
    }

    qDebug() << "SerialWorker: opened" << name;
    return true;
}

void SerialWorker::closePort()
{
    portTimeoutTimer->stop();
    if (serialPort->isOpen())
        serialPort->close();
    if (connected) {
        connected = false;
        emit connectionChanged(false);
        emit logMessage(tr("Light sensor disconnected"));
    }
}

void SerialWorker::tryNextCandidate()
{
    closePort();

    if (scanIndex >= candidatePorts.size()) {
        // 所有候选端口已尝试完毕
        if (!serialPort->isOpen())
            emit logMessage(tr("Light sensor not found, retrying..."));
        emit scanFinished();
        return;
    }

    const QSerialPortInfo &info = candidatePorts[scanIndex];
    scanIndex++;

    if (!openPort(info.portName())) {
        // 打开失败，异步尝试下一个（避免栈溢出）
        QTimer::singleShot(0, this, [this] { tryNextCandidate(); });
        return;
    }

    // 启动异步超时：如果在 PORT_TIMEOUT_MS 内没有收到有效数据，切换到下一个端口
    portTimeoutTimer->start();
}

void SerialWorker::onPortTimeout()
{
    // 当前端口超时未收到有效数据，关闭并尝试下一个
    closePort();
    tryNextCandidate();
}

void SerialWorker::onReadyRead()
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

void SerialWorker::processLine(const QByteArray &line)
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
        portTimeoutTimer->stop();  // 找到传感器，停止超时
        emit connectionChanged(true);
        emit logMessage(tr("Light sensor connected on %1").arg(serialPort->portName()));
    }

    emit logMessage(tr("ADC=%1 Voltage=%2V").arg(adc).arg(volts, 0, 'f', 3));
    emit dataReceived(adc, volts);
}

void SerialWorker::onError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError)
        return;

    // 只处理严重错误（资源错误、权限错误等）
    if (error != QSerialPort::ResourceError &&
        error != QSerialPort::PermissionError &&
        error != QSerialPort::NotOpenError) {
        qDebug() << "SerialWorker serial error (ignored):" << error;
        return;
    }

    qDebug() << "SerialWorker serial error:" << error;
    closePort();
}