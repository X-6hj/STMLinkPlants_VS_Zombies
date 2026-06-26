//
// Created by sun on 8/25/16.
//

#include "SelectorScene.h"
#include "MainView.h"
#include "MouseEventPixmapItem.h"
#include "ImageManager.h"
#include "Animate.h"

TextItemWithoutBorder::TextItemWithoutBorder(const QString &text, QGraphicsItem *parent)
        : QGraphicsTextItem(text, parent)
{}

void TextItemWithoutBorder::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
    QGraphicsTextItem::paint(painter, &myOption, widget);
}

SelectorScene::SelectorScene()
        : QGraphicsScene(0, 0, 900, 600),
          background      (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorBackground.png"))),
          adventureShadow (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorAdventureShadow.png"))),
          adventureButton (new HoverChangedPixmapItem (gImageCache->load("interface/SelectorAdventureButton.png"))),
          survivalShadow  (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorSurvivalShadow.png"))),
          survivalButton  (new HoverChangedPixmapItem (gImageCache->load("interface/SelectorSurvivalButton.png"))),
          challengeShadow (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorChallengeShadow.png"))),
          challengeButton (new HoverChangedPixmapItem (gImageCache->load("interface/SelectorChallengeButton.png"))),
          woodSign1       (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorWoodSign1.png"))),
          woodSign2       (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorWoodSign2.png"))),
          woodSign3       (new QGraphicsPixmapItem    (gImageCache->load("interface/SelectorWoodSign3.png"))),
          zombieHand      (new MoviePixmapItem        ("interface/SelectorZombieHand.gif")),
          quitButton      (new MouseEventRectItem     (QRectF(0, 0, 79, 53))),
          optionsButton   (new MouseEventRectItem     (QRectF(0, 0, 140, 30))),
          usernameText    (new TextItemWithoutBorder  (gMainView->getUsername())),
          levelPanel      (new QGraphicsPixmapItem    (gImageCache->load("interface/SeedChooser_Background.png"))),
          optionsPanel    (new QGraphicsPixmapItem    (gImageCache->load("interface/SeedChooser_Background.png"))),
          backgroundMusic(new QMediaPlayer(this)),
          buttonBleep(new QMediaPlayer(this))
{
    addItem(background);

    quitButton      ->setPen(Qt::NoPen);

    adventureButton ->setCursor(Qt::PointingHandCursor);
    survivalButton  ->setCursor(Qt::PointingHandCursor);
    challengeButton ->setCursor(Qt::PointingHandCursor);
    quitButton      ->setCursor(Qt::PointingHandCursor);

    adventureShadow ->setPos(468, 82);  addItem(adventureShadow);
    adventureButton ->setPos(474, 80);  addItem(adventureButton);
    survivalShadow  ->setPos(476, 208); addItem(survivalShadow);
    survivalButton  ->setPos(474, 203); addItem(survivalButton);
    challengeShadow ->setPos(480, 307); addItem(challengeShadow);
    challengeButton ->setPos(478, 303); addItem(challengeButton);
    quitButton      ->setPos(800, 495); addItem(quitButton);
    woodSign1       ->setPos(20, -8);   addItem(woodSign1);
    woodSign2       ->setPos(23, 126);  addItem(woodSign2);
    woodSign3       ->setPos(34, 179);  addItem(woodSign3);
    zombieHand      ->setPos(262, 264); addItem(zombieHand);
    // Text for username
    usernameText->setParentItem(woodSign1);
    usernameText->setPos(35, 91);
    usernameText->setTextWidth(230);
    usernameText->document()->setDocumentMargin(0);
    usernameText->document()->setDefaultTextOption(QTextOption(Qt::AlignCenter));
    usernameText->setDefaultTextColor(QColor::fromRgb(0xf0c060));
    usernameText->setFont(QFont("Microsoft YaHei", 14, QFont::Bold));

    usernameText->installEventFilter(this);
    usernameText->setTextInteractionFlags(Qt::TextEditorInteraction);

    // ---- Level Selection Panel Setup ----
    levelPanel->setPos(250, 100);
    levelPanel->setZValue(10);
    levelPanel->setVisible(false);
    addItem(levelPanel);

    // Panel title
    QGraphicsSimpleTextItem *panelTitle = new QGraphicsSimpleTextItem(tr("Choose Your Level"));
    panelTitle->setBrush(QColor::fromRgb(0xf0c060));
    panelTitle->setFont(QFont("SimHei", 14, QFont::Bold));
    QSizeF titleSize = panelTitle->boundingRect().size();
    panelTitle->setPos((levelPanel->boundingRect().width() - titleSize.width()) / 2, 12);
    panelTitle->setParentItem(levelPanel);

    // Level data: {eName, displayName}
    QList<QPair<QString, QString> > levels;
    levels.append({"1", tr("Level 1-1")});
    levels.append({"2", tr("Level 2-1 (Night)")});
    levels.append({"3", tr("Level 1-3")});

    for (int i = 0; i < levels.size(); ++i) {
        MouseEventRectItem *btn = new MouseEventRectItem(QRectF(0, 0, 320, 44));
        btn->setPen(QPen(QColor(0x6b, 0x4a, 0x28), 2));
        btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        btn->setPos(40, 50 + i * 52);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setParentItem(levelPanel);

        QGraphicsSimpleTextItem *btnText = new QGraphicsSimpleTextItem(levels[i].second);
        btnText->setFont(QFont("SimHei", 12, QFont::Bold));
        btnText->setBrush(QColor(0x4a, 0x2a, 0x0a));
        QSizeF btnTextSize = btnText->boundingRect().size();
        btnText->setPos((320 - btnTextSize.width()) / 2, (44 - btnTextSize.height()) / 2);
        btnText->setParentItem(btn);

        levelButtons.append(btn);
        levelButtonTexts.append(btnText);

        QString levelEName = levels[i].first;
        connect(btn, &MouseEventRectItem::clicked, [this, levelEName] {
            buttonBleep->stop(); buttonBleep->play();
            hideLevelPanel();
            adventureButton->setCursor(Qt::ArrowCursor);
            survivalButton->setCursor(Qt::ArrowCursor);
            challengeButton->setCursor(Qt::ArrowCursor);
            adventureButton->setEnabled(false);
            survivalButton->setEnabled(false);
            challengeButton->setEnabled(false);

            zombieHand->start();
            backgroundMusic->pause();
            QMediaPlayer *loseMusic2 = new QMediaPlayer(this);
            loseMusic2->setMedia(QUrl("qrc:/audio/losemusic.mp3"));
            loseMusic2->play();
            connect(zombieHand, &MoviePixmapItem::finished, [this, loseMusic2, levelEName] {
                (new Timer(this, 2500, [this, loseMusic2, levelEName](){
                    loseMusic2->stop();
                    QSettings().setValue("Global/NextLevel", levelEName);
                    gMainView->switchToGameScene(levelEName);
                }))->start();
            });
        });

        // Hover effects
        connect(btn, &MouseEventRectItem::hoverEntered, [this, btn] {
            buttonBleep->stop(); buttonBleep->play();
            btn->setBrush(QColor(0xf0, 0xb0, 0x60, 220));
        });
        connect(btn, &MouseEventRectItem::hoverLeft, [this, btn] {
            btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        });
    }

    // Close button for the level panel
    MouseEventRectItem *closeBtn = new MouseEventRectItem(QRectF(0, 0, 80, 30));
    closeBtn->setPen(QPen(QColor(0x8b, 0x3a, 0x3a), 2));
    closeBtn->setBrush(QColor(0xc0, 0x40, 0x40, 200));
    closeBtn->setPos((levelPanel->boundingRect().width() - 80) / 2,
                     levelPanel->boundingRect().height() - 45);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setParentItem(levelPanel);

    QGraphicsSimpleTextItem *closeText = new QGraphicsSimpleTextItem(tr("Back"));
    closeText->setFont(QFont("SimHei", 12, QFont::Bold));
    closeText->setBrush(Qt::white);
    QSizeF closeTextSize = closeText->boundingRect().size();
    closeText->setPos((80 - closeTextSize.width()) / 2, (30 - closeTextSize.height()) / 2);
    closeText->setParentItem(closeBtn);

    connect(closeBtn, &MouseEventRectItem::clicked, [this] {
        buttonBleep->stop(); buttonBleep->play();
        hideLevelPanel();
    });
    // ---- End Level Selection Panel ----

    // ---- Options Button (transparent overlay on woodSign3 "选项" area) ----
    optionsButton->setPen(Qt::NoPen);
    optionsButton->setBrush(Qt::transparent);
    optionsButton->setPos(34, 210);
    optionsButton->setCursor(Qt::PointingHandCursor);
    addItem(optionsButton);

    // ---- Options Panel Setup ----
    optionsPanel->setPos(250, 80);
    optionsPanel->setZValue(11);
    optionsPanel->setVisible(false);
    addItem(optionsPanel);

    QGraphicsSimpleTextItem *optionsTitle = new QGraphicsSimpleTextItem(tr("Options"));
    optionsTitle->setBrush(QColor::fromRgb(0xf0c060));
    optionsTitle->setFont(QFont("SimHei", 14, QFont::Bold));
    QSizeF optTitleSize = optionsTitle->boundingRect().size();
    optionsTitle->setPos((optionsPanel->boundingRect().width() - optTitleSize.width()) / 2, 12);
    optionsTitle->setParentItem(optionsPanel);

    // Full Screen toggle
    {
        MouseEventRectItem *btn = new MouseEventRectItem(QRectF(0, 0, 320, 44));
        btn->setPen(QPen(QColor(0x6b, 0x4a, 0x28), 2));
        btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        btn->setPos(40, 50);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setParentItem(optionsPanel);

        bool isFullScreen = QSettings().value("UI/FullScreen", false).toBool();
        QGraphicsSimpleTextItem *btnText = new QGraphicsSimpleTextItem(
            tr("Full Screen: ") + (isFullScreen ? tr("On") : tr("Off")));
        btnText->setFont(QFont("SimHei", 12, QFont::Bold));
        btnText->setBrush(QColor(0x4a, 0x2a, 0x0a));
        QSizeF sz = btnText->boundingRect().size();
        btnText->setPos((320 - sz.width()) / 2, (44 - sz.height()) / 2);
        btnText->setParentItem(btn);

        connect(btn, &MouseEventRectItem::clicked, [this, btnText] {
            buttonBleep->stop(); buttonBleep->play();
            bool cur = QSettings().value("UI/FullScreen", false).toBool();
            bool next = !cur;
            QSettings().setValue("UI/FullScreen", next);
            btnText->setText(tr("Full Screen: ") + (next ? tr("On") : tr("Off")));
            QSizeF sz = btnText->boundingRect().size();
            btnText->setPos((320 - sz.width()) / 2, (44 - sz.height()) / 2);
            gMainView->getMainWindow()->getFullScreenAction()->toggle();
        });

        connect(btn, &MouseEventRectItem::hoverEntered, [this, btn] {
            buttonBleep->stop(); buttonBleep->play();
            btn->setBrush(QColor(0xf0, 0xb0, 0x60, 220));
        });
        connect(btn, &MouseEventRectItem::hoverLeft, [this, btn] {
            btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        });
    }

    // Music toggle
    {
        MouseEventRectItem *btn = new MouseEventRectItem(QRectF(0, 0, 320, 44));
        btn->setPen(QPen(QColor(0x6b, 0x4a, 0x28), 2));
        btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        btn->setPos(40, 102);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setParentItem(optionsPanel);

        QGraphicsSimpleTextItem *btnText = new QGraphicsSimpleTextItem(tr("Music: On"));
        btnText->setFont(QFont("SimHei", 12, QFont::Bold));
        btnText->setBrush(QColor(0x4a, 0x2a, 0x0a));
        QSizeF sz = btnText->boundingRect().size();
        btnText->setPos((320 - sz.width()) / 2, (44 - sz.height()) / 2);
        btnText->setParentItem(btn);

        bool musicOn = true;
        connect(btn, &MouseEventRectItem::clicked, [this, btnText, musicOn]() mutable {
            buttonBleep->stop(); buttonBleep->play();
            musicOn = !musicOn;
            btnText->setText(musicOn ? tr("Music: On") : tr("Music: Off"));
            QSizeF sz = btnText->boundingRect().size();
            btnText->setPos((320 - sz.width()) / 2, (44 - sz.height()) / 2);
            if (musicOn)
                backgroundMusic->play();
            else
                backgroundMusic->pause();
        });

        connect(btn, &MouseEventRectItem::hoverEntered, [this, btn] {
            buttonBleep->stop(); buttonBleep->play();
            btn->setBrush(QColor(0xf0, 0xb0, 0x60, 220));
        });
        connect(btn, &MouseEventRectItem::hoverLeft, [this, btn] {
            btn->setBrush(QColor(0xd4, 0xa0, 0x5a, 200));
        });
    }

    // Close button for options panel
    {
        MouseEventRectItem *closeBtn = new MouseEventRectItem(QRectF(0, 0, 80, 30));
        closeBtn->setPen(QPen(QColor(0x8b, 0x3a, 0x3a), 2));
        closeBtn->setBrush(QColor(0xc0, 0x40, 0x40, 200));
        closeBtn->setPos((optionsPanel->boundingRect().width() - 80) / 2,
                         optionsPanel->boundingRect().height() - 45);
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setParentItem(optionsPanel);

        QGraphicsSimpleTextItem *closeText = new QGraphicsSimpleTextItem(tr("Close"));
        closeText->setFont(QFont("SimHei", 12, QFont::Bold));
        closeText->setBrush(Qt::white);
        QSizeF sz = closeText->boundingRect().size();
        closeText->setPos((80 - sz.width()) / 2, (30 - sz.height()) / 2);
        closeText->setParentItem(closeBtn);

        connect(closeBtn, &MouseEventRectItem::clicked, [this] {
            buttonBleep->stop(); buttonBleep->play();
            hideOptionsPanel();
        });
    }
    // ---- End Options Panel ----

    backgroundMusic->setMedia(QUrl("qrc:/audio/Faster.mp3"));
    buttonBleep->setMedia(QUrl("qrc:/audio/bleep.mp3"));

    connect(backgroundMusic, &QMediaPlayer::stateChanged, [this](QMediaPlayer::State state) {
        if (state == QMediaPlayer::StoppedState)
            backgroundMusic->play();
    });

    connect(adventureButton, &HoverChangedPixmapItem::hoverEntered, [this] { buttonBleep->stop(); buttonBleep->play(); });
    connect(survivalButton, &HoverChangedPixmapItem::hoverEntered, [this] { buttonBleep->stop(); buttonBleep->play(); });
    connect(challengeButton, &HoverChangedPixmapItem::hoverEntered, [this] { buttonBleep->stop(); buttonBleep->play(); });

    // Adventure button now shows level selection panel
    connect(adventureButton, &HoverChangedPixmapItem::clicked, [this] {
        buttonBleep->stop(); buttonBleep->play();
        showLevelPanel();
    });

    // Survival button - show coming soon
    connect(survivalButton, &HoverChangedPixmapItem::clicked, [this] {
        buttonBleep->stop(); buttonBleep->play();
        showLevelPanel(); // reuse level panel for now
    });

    // Challenge button - show coming soon
    connect(challengeButton, &HoverChangedPixmapItem::clicked, [this] {
        buttonBleep->stop(); buttonBleep->play();
        showLevelPanel(); // reuse level panel for now
    });

    connect(optionsButton, &MouseEventRectItem::clicked, [this] {
        buttonBleep->stop(); buttonBleep->play();
        showOptionsPanel();
    });

    connect(quitButton, &MouseEventRectItem::clicked, [] {
        gMainView->getMainWindow()->close();
    });
}

void SelectorScene::showLevelPanel()
{
    if (levelPanel->isVisible()) return;
    hideOptionsPanel();
    levelPanel->setVisible(true);
    levelPanel->setOpacity(0.0);
    Animate(levelPanel).fade(1.0).duration(200).finish();
    // Click outside to close
    disconnect(levelPanelOutsideConn);
    levelPanelOutsideConn = connect(this, &SelectorScene::mousePress, [this](QGraphicsSceneMouseEvent *ev) {
        if (levelPanel->isVisible() && !levelPanel->contains(ev->scenePos() - levelPanel->scenePos())) {
            hideLevelPanel();
        }
    });
}

void SelectorScene::hideLevelPanel()
{
    if (!levelPanel->isVisible()) return;
    disconnect(levelPanelOutsideConn);
    Animate(levelPanel).fade(0.0).duration(150).finish([this] {
        levelPanel->setVisible(false);
    });
}

void SelectorScene::showOptionsPanel()
{
    if (optionsPanel->isVisible()) return;
    hideLevelPanel();
    optionsPanel->setVisible(true);
    optionsPanel->setOpacity(0.0);
    Animate(optionsPanel).fade(1.0).duration(200).finish();
    // Click outside to close
    disconnect(optionsPanelOutsideConn);
    optionsPanelOutsideConn = connect(this, &SelectorScene::mousePress, [this](QGraphicsSceneMouseEvent *ev) {
        if (optionsPanel->isVisible() && !optionsPanel->contains(ev->scenePos() - optionsPanel->scenePos())) {
            hideOptionsPanel();
        }
    });
}

void SelectorScene::hideOptionsPanel()
{
    if (!optionsPanel->isVisible()) return;
    disconnect(optionsPanelOutsideConn);
    Animate(optionsPanel).fade(0.0).duration(150).finish([this] {
        optionsPanel->setVisible(false);
    });
}

bool SelectorScene::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == usernameText) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                // Save the username
                gMainView->setUsername(usernameText->toPlainText());
                setFocusItem(nullptr);
                return true;
            }
            return false;
        }
        return false;
    }
    return QGraphicsScene::eventFilter(watched, event);
}

void SelectorScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    emit mousePress(event);
}

void SelectorScene::loadReady()
{
    // Animation is so UGLY.
    //moveItemWithDuration(woodSign1, QPointF(20, -8), 400, [] {}, QTimeLine::EaseOutCurve);
    //moveItemWithDuration(woodSign2, QPointF(23, 126), 500, [] {}, QTimeLine::EaseOutCurve);
    //moveItemWithDuration(woodSign3, QPointF(34, 179), 600, [] {}, QTimeLine::EaseOutCurve);
    gMainView->getMainWindow()->setWindowTitle(tr("Plants vs. Zombies"));
    backgroundMusic->play();
}
