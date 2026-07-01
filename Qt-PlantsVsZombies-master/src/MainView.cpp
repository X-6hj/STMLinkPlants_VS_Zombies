//
// Created by sun on 8/25/16.
//

#include "MainView.h"
#include "GameLevelData.h"
#include "SelectorScene.h"
#include "GameScene.h"
#include "AspectRatioLayout.h"
#include "Timer.h"
#include <QSerialPortInfo>

MainView *gMainView;

MainView::MainView(MainWindow *mainWindow)
        : width(900), height(600),
          usernameSettingEntry("Global/Username"),
          selectorScene(nullptr), gameScene(nullptr),
          mainWindow(mainWindow),
          lightSensor(new LightSensorReader(this))
{
    gMainView = this;

    setMouseTracking(true);

    connect(lightSensor, &LightSensorReader::logMessage, [](const QString &msg) {
        qDebug() << "[LightSensor]" << msg;
    });

    // 延迟所有串口操作到事件循环启动后，避免构造期间阻塞 UI 线程
    // QSerialPortInfo::availablePorts() 在 Windows 上可能耗时 100-500ms
    QTimer::singleShot(0, this, [this] {
        const auto ports = QSerialPortInfo::availablePorts();

        // Prefer a CH340 / USB-TTL adapter if present; allow override via env var.
        QString envPort = QString::fromLocal8Bit(qgetenv("LIGHT_SENSOR_PORT"));
        if (!envPort.isEmpty()) {
            lightSensor->setPreferredPort(envPort);
            qDebug() << "[LightSensor] preferred port from env:" << envPort;
        } else {
            for (const QSerialPortInfo &info : ports) {
                QString desc = info.description().toUpper();
                if (desc.contains("CH340") || desc.contains("USB-SERIAL")) {
                    lightSensor->setPreferredPort(info.portName());
                    qDebug() << "[LightSensor] preferred port auto-detected:" << info.portName() << info.description();
                    break;
                }
            }
        }

        // Thresholds can be tuned via env vars (higher ADC == darker for this sensor).
        QByteArray envDay = qgetenv("LIGHT_SENSOR_DAY_THRESHOLD");
        QByteArray envNight = qgetenv("LIGHT_SENSOR_NIGHT_THRESHOLD");
        if (!envDay.isEmpty() && !envNight.isEmpty()) {
            lightSensor->setNightThresholdLow(envDay.toInt());
            lightSensor->setNightThresholdHigh(envNight.toInt());
            qDebug() << "[LightSensor] thresholds from env: day<" << envDay.toInt()
                     << ", night>" << envNight.toInt();
        }

        // 传递已获取的端口列表，避免 start() 内部重复调用 availablePorts()
        lightSensor->start(ports);
    });

    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    setFrameStyle(0);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setMinimumSize(width, height);
    // Set up username
    if (getUsername().isEmpty()) {
        QString username = qgetenv("USER"); // Linux or Mac
        if (username.isEmpty())
            username = qgetenv("USERNAME"); // Windows
        if (username.isEmpty())
            username = tr("Guest");
        setUsername(username);
    }
}

MainView::~MainView()
{
    if (selectorScene)
        selectorScene->deleteLater();
    if (gameScene)
        gameScene->deleteLater();
}

QString MainView::getUsername() const
{
    return QSettings().value(usernameSettingEntry, "").toString();
}

void MainView::setUsername(const QString &username)
{
    return QSettings().setValue(usernameSettingEntry, username);
}

MainWindow *MainView::getMainWindow() const
{
    return mainWindow;
}

void MainView::switchToGameScene(const QString &eName)
{
    GameScene *newGameScene = new GameScene(GameLevelDataFactory(eName));
    connect(lightSensor, &LightSensorReader::nightModeChanged, newGameScene, &GameScene::setNightMode);
    newGameScene->setNightMode(lightSensor->nightMode());
    setScene(newGameScene);
    if (gameScene)
        gameScene->deleteLater();
    gameScene = newGameScene;
    gameScene->loadReady();
}

void MainView::switchToMenuScene()
{
    gPaused = false;  // 重置全局暂停标志，确保新场景定时器正常工作
    SelectorScene *newSelectorScene = new SelectorScene;
    setScene(newSelectorScene);
    if (selectorScene)
        selectorScene->deleteLater();
    selectorScene = newSelectorScene;
    selectorScene->loadReady();
}

void MainView::resizeEvent(QResizeEvent *event)
{
    // `fitInView` has a bug causing extra margin.
    // see "https://bugreports.qt.io/browse/QTBUG-11945"
    QRectF viewRect = frameRect();
    QTransform trans;
    trans.scale(viewRect.width() / width, viewRect.height() / height);
    setTransform(trans);
}

MainWindow::MainWindow()
    : fullScreenSettingEntry("UI/FullScreen"),
      mainView(new MainView(this)),
      fullScreenAction(new QAction)
{
    // Layout
    QWidget *centralWidget = new QWidget;
    QLayout *layout = new AspectRatioLayout;
    layout->addWidget(mainView);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
    // Full Screen action triggered with "F11"
    fullScreenAction->setCheckable(true);
    fullScreenAction->setShortcut(Qt::Key_F11);
    addAction(fullScreenAction);
    connect(fullScreenAction, &QAction::toggled, [this] (bool checked) {
        if (checked)
            setWindowState(Qt::WindowFullScreen);
        else
            setWindowState(Qt::WindowNoState);
        QSettings().setValue(fullScreenSettingEntry, checked);
    });
    fullScreenAction->setChecked(QSettings().value(fullScreenSettingEntry, false).toBool());
    // Set background color to black
    setPalette(Qt::black);
    setAutoFillBackground(true);

    adjustSize();
}

QAction *MainWindow::getFullScreenAction() const
{
    return fullScreenAction;
}