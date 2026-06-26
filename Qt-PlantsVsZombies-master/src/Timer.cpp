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
            // 暂停中：重新调度，稍后执行
            if (parent) {
                (new Timer(parent, timeout, functor))->start();
            }
            deleteLater();
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
        if (gPaused) return;  // 暂停时跳过更新
        onChanged(val);
    });
    connect(this, &TimeLine::finished, [this, onFinished] {
        if (gPaused) return;  // 暂停时跳过完成回调（TimeLine 会继续运行）
        onFinished();
        deleteLater();
    });
}