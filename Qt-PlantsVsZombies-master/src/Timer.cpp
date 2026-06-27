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
            // 暂停中：定时器保持当前对象，重新启动等待恢复后执行
            // 不创建新对象防止长时间暂停时内存泄漏（用户点击菜单久留不回）
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