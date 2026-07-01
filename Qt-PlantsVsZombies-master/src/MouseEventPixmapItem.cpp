//
// Created by sun on 8/26/16.
//
#include "MouseEventPixmapItem.h"

MouseEventRectItem::MouseEventRectItem()
{
    setAcceptHoverEvents(true);
}

MouseEventRectItem::MouseEventRectItem(const QRectF &rect) : QGraphicsRectItem(rect)
{
    setAcceptHoverEvents(true);
}

void MouseEventRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit clicked(event);
}

void MouseEventRectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverEntered(event);
}

void MouseEventRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverLeft(event);
}

void MouseEventRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverMoved(event);
}


MouseEventPixmapItem::MouseEventPixmapItem()
{
    setAcceptHoverEvents(true);
}

MouseEventPixmapItem::MouseEventPixmapItem(const QPixmap &image) : QGraphicsPixmapItem(image)
{
    setAcceptHoverEvents(true);
}

void MouseEventPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit clicked(event);
}

void MouseEventPixmapItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverEntered(event);
}

void MouseEventPixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverLeft(event);
}

void MouseEventPixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    emit hoverMoved(event);
}


HoverChangedPixmapItem::HoverChangedPixmapItem(const QPixmap &image) : origImage(image.copy(0, 0, image.width(), image.height() / 2)),
                                                                                 hoverImage(image.copy(0, image.height() / 2, image.width(), image.height() / 2))
{
    setPixmap(origImage);
    setAcceptHoverEvents(true);
    connect(this, &HoverChangedPixmapItem::hoverEntered, [this] { setPixmap(hoverImage); });
    connect(this, &HoverChangedPixmapItem::hoverLeft, [this] { setPixmap(origImage); });
}

MoviePixmapItem::MoviePixmapItem(const QString &filename)
        : movie(nullptr)
{
    setMovie(filename);
}

MoviePixmapItem::MoviePixmapItem()
        : movie(nullptr)
{}

MoviePixmapItem::~MoviePixmapItem()
{
    if (movie) {
        if (movie->state() == QMovie::Running)
            movie->stop();
    }
    // 清理缓存中的所有QMovie对象
    for (auto *m : movieCache.values())
        delete m;
    movieCache.clear();
}

void MoviePixmapItem::setMovie(const QString &filename)
{
    // 停止当前动画
    if (movie) {
        movie->stop();
        // 如果当前movie在缓存中，不要删除它
        if (!movieCache.values().contains(movie))
            delete movie;
    }

    // 检查缓存：如果已有该文件名的QMovie，直接复用
    if (movieCache.contains(filename)) {
        movie = movieCache[filename];
    } else {
        movie = new QMovie(":/images/" + filename);
        movie->setCacheMode(QMovie::CacheAll);
        movieCache[filename] = movie;
        connect(movie, &QMovie::frameChanged, [this](int i){
            setPixmap(movie->currentPixmap());
            if (i == 0)
                emit loopStarted();
        });
        connect(movie, &QMovie::finished, [this]{ emit finished(); });
    }

    movie->jumpToFrame(0);
    setPixmap(movie->currentPixmap());
}

void MoviePixmapItem::start()
{
    movie->start();
}

void MoviePixmapItem::stop()
{
    movie->stop();
}

void MoviePixmapItem::reset()
{
    movie->jumpToFrame(0);
}

void MoviePixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit click(event);
}

void MoviePixmapItem::setMovieOnNewLoop(const QString &filename, std::function<void(void)> functor)
{
    QSharedPointer<QMetaObject::Connection> connection(new QMetaObject::Connection);
    *connection = QObject::connect(this, &MoviePixmapItem::loopStarted, [this, connection, filename, functor] {
        QObject::disconnect(*connection);  // 必须先断开连接，否则 setMovie 内的 jumpToFrame(0) 会再次触发 loopStarted 导致重入
        setMovie(filename);
        start();
        functor();
    });
}
