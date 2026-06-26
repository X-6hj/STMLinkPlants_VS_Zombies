//
// Light sensor reader: receives ADC data from STM32 via UART
// and notifies subscribers of the current ambient light level.
//

#ifndef LIGHT_SENSOR_READER_H
#define LIGHT_SENSOR_READER_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include <QRegularExpression>

class LightSensorReader : public QObject
{
    Q_OBJECT

public:
    explicit LightSensorReader(QObject *parent = nullptr);
    ~LightSensorReader();

    void start();
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
    void onReadyRead();
    void onError(QSerialPort::SerialPortError error);
    void tryConnect();

private:
    bool openPort(const QString &name);
    void closePort();
    void processLine(const QByteArray &line);

    QSerialPort *serialPort;
    QTimer *reconnectTimer;
    QByteArray readBuffer;
    QRegularExpression dataRegex;

    bool currentlyNight;
    bool connected;
    int lastAdc;
    float lastVolts;

    int thresholdLow;
    int thresholdHigh;
    QString preferredPortName;
    QString lastSuccessfulPortName;

    static const int BAUD_RATE;
};

#endif // LIGHT_SENSOR_READER_H