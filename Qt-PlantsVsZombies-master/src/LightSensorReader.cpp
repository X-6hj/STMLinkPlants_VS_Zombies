//
// LightSensorReader implementation
// 串口 I/O 委托给 SerialWorker（独立线程），UI 线程仅处理信号转发和夜间模式逻辑。
//

#include "LightSensorReader.h"
#include "SerialWorker.h"
#include <QSerialPortInfo>
#include <QDebug>

LightSensorReader::LightSensorReader(QObject *parent)
    : QObject(parent),
      workerThread(new QThread(this)),
      worker(nullptr),
      reconnectTimer(new QTimer(this)),
      currentlyNight(false),
      m_connected(false),
      lastAdc(0),
      lastVolts(0.0f),
      thresholdLow(1500),
      thresholdHigh(1700),
      retryCount(0)
{
    // 创建 SerialWorker 并移入工作线程
    worker = new SerialWorker;  // 无父对象，将由 moveToThread 管理
    worker->moveToThread(workerThread);

    // 连接工作线程信号到主线程槽
    connect(worker, &SerialWorker::dataReceived, this, &LightSensorReader::onWorkerDataReceived);
    connect(worker, &SerialWorker::connectionChanged, this, &LightSensorReader::onWorkerConnectionChanged);
    connect(worker, &SerialWorker::logMessage, this, &LightSensorReader::logMessage);
    connect(worker, &SerialWorker::scanFinished, this, &LightSensorReader::onWorkerScanFinished);

    // 清理：线程结束时删除 worker
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);

    // 重连定时器（在主线程中运行）
    reconnectTimer->setSingleShot(false);
    reconnectTimer->setInterval(2000);
    connect(reconnectTimer, &QTimer::timeout, this, &LightSensorReader::tryReconnect);

    workerThread->start();
}

LightSensorReader::~LightSensorReader()
{
    stop();
    workerThread->quit();
    workerThread->wait(3000);
}

void LightSensorReader::start(const QList<QSerialPortInfo> &knownPorts)
{
    retryCount = 0;

    if (qEnvironmentVariableIsSet("LIGHT_SENSOR_DISABLED")) {
        emit logMessage(tr("Light sensor disabled by LIGHT_SENSOR_DISABLED env var"));
        return;
    }

    const auto ports = knownPorts.isEmpty() ? QSerialPortInfo::availablePorts() : knownPorts;
    lastKnownPorts = ports;

    if (ports.isEmpty()) {
        emit logMessage(tr("No serial ports available, light sensor disabled"));
        return;
    }

    // 延迟启动扫描到事件循环，避免阻塞 UI 线程
    // SerialWorker 在工作线程中执行实际的串口操作
    QTimer::singleShot(0, this, [this, ports] {
        QMetaObject::invokeMethod(worker, "startScan", Qt::QueuedConnection,
                                  Q_ARG(QList<QSerialPortInfo>, ports));
    });
    reconnectTimer->start();
}

void LightSensorReader::stop()
{
    reconnectTimer->stop();
    QMetaObject::invokeMethod(worker, "stop", Qt::QueuedConnection);
}

bool LightSensorReader::isConnected() const
{
    return m_connected;
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

void LightSensorReader::onWorkerDataReceived(int adcValue, float voltage)
{
    lastAdc = adcValue;
    lastVolts = voltage;

    emit dataReceived(adcValue, voltage);

    // 迟滞夜间模式检测（主线程纯计算，不阻塞）
    bool night = currentlyNight;
    if (adcValue > thresholdHigh)
        night = true;
    else if (adcValue < thresholdLow)
        night = false;

    if (night != currentlyNight) {
        currentlyNight = night;
        emit nightModeChanged(night);
        emit logMessage(night ? tr("Night mode detected") : tr("Day mode detected"));
    }
}

void LightSensorReader::onWorkerConnectionChanged(bool connected)
{
    m_connected = connected;
    if (connected)
        retryCount = 0;
    emit connectionChanged(connected);
}

void LightSensorReader::onWorkerScanFinished()
{
    // 所有候选端口已尝试完毕
    retryCount++;
    if (retryCount > MAX_RETRY_COUNT) {
        emit logMessage(tr("Light sensor: max retries exceeded, stopping scan"));
        reconnectTimer->stop();
        return;
    }
    // 重连定时器会继续触发 tryReconnect()
}

void LightSensorReader::tryReconnect()
{
    if (m_connected)
        return;

    retryCount++;
    if (retryCount > MAX_RETRY_COUNT) {
        emit logMessage(tr("Light sensor: max retries exceeded, stopping scan"));
        reconnectTimer->stop();
        return;
    }

    // 重新扫描端口列表（在主线程中调用 availablePorts，已通过 QTimer 延迟）
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.isEmpty()) {
        emit logMessage(tr("No serial ports found"));
        return;
    }
    lastKnownPorts = ports;

    // 委托给工作线程执行实际的串口连接
    QMetaObject::invokeMethod(worker, "startScan", Qt::QueuedConnection,
                              Q_ARG(QList<QSerialPortInfo>, ports));
}