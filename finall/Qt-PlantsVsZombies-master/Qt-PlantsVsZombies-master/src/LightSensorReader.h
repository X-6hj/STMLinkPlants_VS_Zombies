//
// Light sensor reader: 从 STM32 通过 UART 接收 ADC 数据
// 并通知订阅者当前环境光等级。
//
// 串口 I/O 操作委托给 SerialWorker（在独立线程中运行），
// 确保 UI 线程不受串口阻塞影响。
//

#ifndef LIGHT_SENSOR_READER_H
#define LIGHT_SENSOR_READER_H

#include <QObject>
#include <QSerialPortInfo>
#include <QTimer>
#include <QThread>

class SerialWorker;

class LightSensorReader : public QObject
{
    Q_OBJECT

public:
    explicit LightSensorReader(QObject *parent = nullptr);
    ~LightSensorReader();

    void start(const QList<QSerialPortInfo> &knownPorts = {});
    void stop();

    bool isConnected() const;
    bool nightMode() const;
    int lastAdcValue() const;
    float lastVoltage() const;

    void setNightThresholdLow(int value);
    void setNightThresholdHigh(int value);

    void setPreferredPort(const QString &name);
    QString preferredPort() const;

signals:
    void dataReceived(int adcValue, float voltage);
    void nightModeChanged(bool night);
    void connectionChanged(bool connected);
    void logMessage(const QString &message);

private slots:
    void onWorkerDataReceived(int adcValue, float voltage);
    void onWorkerConnectionChanged(bool connected);
    void onWorkerScanFinished();
    void tryReconnect();

private:
    QThread *workerThread;
    SerialWorker *worker;

    QTimer *reconnectTimer;

    bool currentlyNight;
    bool m_connected;
    int lastAdc;
    float lastVolts;

    int thresholdLow;
    int thresholdHigh;
    QString preferredPortName;
    int retryCount;

    QList<QSerialPortInfo> lastKnownPorts;

    static const int MAX_RETRY_COUNT = 10;
};

#endif // LIGHT_SENSOR_READER_H