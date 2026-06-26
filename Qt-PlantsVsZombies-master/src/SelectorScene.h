//
// Created by sun on 8/25/16.
//

#ifndef PLANTS_VS_ZOMBIES_SELECTORSCENE_H
#define PLANTS_VS_ZOMBIES_SELECTORSCENE_H

#include <QtWidgets>
#include <QtMultimedia>

class HoverChangedPixmapItem;
class MoviePixmapItem;
class MouseEventRectItem;

class TextItemWithoutBorder: public QGraphicsTextItem
{
    Q_OBJECT

public:
    TextItemWithoutBorder(const QString &text, QGraphicsItem *parent = nullptr);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

class SelectorScene: public QGraphicsScene
{
    Q_OBJECT

public:
    SelectorScene();
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

    void loadReady();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void mousePress(QGraphicsSceneMouseEvent *event);
private:
    void showLevelPanel();
    void hideLevelPanel();
    void showOptionsPanel();
    void hideOptionsPanel();

    QGraphicsPixmapItem *background;
    QGraphicsPixmapItem *adventureShadow;
    HoverChangedPixmapItem *adventureButton;
    QGraphicsPixmapItem *survivalShadow;
    HoverChangedPixmapItem *survivalButton;
    QGraphicsPixmapItem *challengeShadow;
    HoverChangedPixmapItem *challengeButton;
    QGraphicsPixmapItem *woodSign1;
    QGraphicsPixmapItem *woodSign2;
    QGraphicsPixmapItem *woodSign3;
    MoviePixmapItem *zombieHand;
    MouseEventRectItem *quitButton;
    MouseEventRectItem *optionsButton;
    TextItemWithoutBorder *usernameText;

    // Level selection popup
    QGraphicsPixmapItem *levelPanel;
    QList<MouseEventRectItem *> levelButtons;
    QList<QGraphicsSimpleTextItem *> levelButtonTexts;

    // Options popup
    QGraphicsPixmapItem *optionsPanel;

    QMediaPlayer *backgroundMusic;
    QMediaPlayer *buttonBleep;

    QMetaObject::Connection levelPanelOutsideConn;
    QMetaObject::Connection optionsPanelOutsideConn;
};

#endif //PLANTS_VS_ZOMBIES_MENUSCENE_H
