//
// Created by sun on 9/2/16.
//

#include "Timer.h"

bool gPaused = false;

Timer::Timer(QObject *parent, int timeout, std::function<void(void)> functor) : QTimer(parent)
{
    setInterval(timeout);
    if (timeout < 50)
        setTimerType(Qt::PreciseTimer);
    setSingleShot(true);
    connect(this, &Timer::timeout, [this, functor, timeout, parent] {
        if (gPaused && timeout > 0) {
            // 暂停中：以 500ms 间隔重新启动，避免高频空转消耗 CPU
            // 长时间暂停时不会产生大量定时器事件
            this->setInterval(500);
            this->start();
            return;
        }
        functor();
        deleteLater();
    });
}

TimeLine::TimeLine(QObject *parent, int duration, int interval, std::function<void(qreal)> onChanged, std::function<void(void)> onFinished, CurveShape shape)
        : QTimeLine(duration, parent)
{
    if (duration == 0) {
        int i = 1;
        ++i;
    }
    setUpdateInterval(interval);
    setCurveShape(shape);
    connect(this, &TimeLine::valueChanged, [onChanged](qreal val) {
        onChanged(val);
    });
    connect(this, &TimeLine::finished, [this, onFinished] {
        onFinished();
        deleteLater();
    });

    // 监听全局暂停标志，正确暂停/恢复 QTimeLine 内部计时
    QTimer *pauseWatcher = new QTimer(this);
    pauseWatcher->setInterval(100);
    connect(pauseWatcher, &QTimer::timeout, [this] {
        if (gPaused) {
            if (state() == Running)
                setPaused(true);
        } else {
            if (state() == Paused)
                setPaused(false);
        }
    });
    pauseWatcher->start();
}