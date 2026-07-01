//
// SerialWorker: 在独立线程中执行所有串口 I/O 操作
// 所有 QSerialPort 的 open/close/read 都在工作线程中执行，
// 确保 UI 线程完全不受串口阻塞操作影响。
//

#ifndef SERIAL_WORKER_H
#define SERIAL_WORKER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QRegularExpression>
#include <QList>

class SerialWorker : public QObject
{
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

public slots:
    // 从主线程调用：开始扫描端口列表
    void startScan(const QList<QSerialPortInfo> &ports);
    // 从主线程调用：停止所有操作
    void stop();

signals:
    // 发射到主线程
    void dataReceived(int adcValue, float voltage);
    void connectionChanged(bool connected);
    void logMessage(const QString &message);
    void scanFinished();

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);
    void onPortTimeout();
    void tryNextCandidate();

private:
    bool openPort(const QString &name);
    void closePort();
    void processLine(const QByteArray &line);

    QSerialPort *serialPort;
    QTimer *portTimeoutTimer;
    QByteArray readBuffer;
    QRegularExpression dataRegex;

    bool connected;
    int lastAdc;
    float lastVolts;

    QList<QSerialPortInfo> candidatePorts;
    int scanIndex;

    static const int BAUD_RATE = 115200;
    static const int PORT_TIMEOUT_MS = 1500;
};

#endif // SERIAL_WORKER_H