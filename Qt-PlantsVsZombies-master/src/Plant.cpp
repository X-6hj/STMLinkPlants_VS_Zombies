//
// Created by sun on 8/26/16.
//

#include <QtMultimedia>
#include "Plant.h"
#include "ImageManager.h"
#include "GameScene.h"
#include "GameLevelData.h"
#include "MouseEventPixmapItem.h"
#include "Timer.h"
#include "Animate.h"

Plant::Plant()
        : hp(300),
          pKind(1), bKind(0),
          beAttackedPointL(20), beAttackedPointR(20),
          zIndex(0),
          canEat(true), canSelect(true), night(false),
          coolTime(7.5), stature(0), sleep(0), scene(nullptr)
{}

double Plant::getDX() const
{
    return -0.5 * width;
}

double Plant::getDY(int x, int y) const
{
    return scene->getPlant(x, y).contains(0) ? -21 : -15;
}

bool Plant::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return !plants.contains(1);
    return plants.contains(0) && !plants.contains(1);
}

void Plant::update()
{
    QPixmap pic = gImageCache->load(staticGif);
    width = pic.width();
    height = pic.height();
}

PlantInstance::PlantInstance(const Plant *plant) : plantProtoType(plant)
{
    uuid = QUuid::createUuid();
    hp = plantProtoType->hp;
    canTrigger = true;
    picture = new MoviePixmapItem;
}

PlantInstance::~PlantInstance()
{
    picture->deleteLater();
}

void PlantInstance::birth(int c, int r)
{
    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
    double x = coordinate.getX(c) + plantProtoType->getDX(), y = coordinate.getY(r) + plantProtoType->getDY(c, r) - plantProtoType->height;
    col = c, row = r;
    attackedLX = x + plantProtoType->beAttackedPointL;
    attackedRX = x + plantProtoType->beAttackedPointR;
    picture->setMovie(plantProtoType->normalGif);
    picture->setPos(x, y);
    picture->setZValue(plantProtoType->zIndex + 3 * r);
    shadowPNG = new QGraphicsPixmapItem(gImageCache->load("interface/plantShadow.png"));
    shadowPNG->setPos(plantProtoType->width * 0.5 - 48, plantProtoType->height - 22);
    shadowPNG->setFlag(QGraphicsItem::ItemStacksBehindParent);
    shadowPNG->setParentItem(picture);
    picture->start();
    plantProtoType->scene->addToGame(picture);
    initTrigger();
}

void PlantInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, 880, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger } );
    plantProtoType->scene->addTrigger(row, trigger);
}

bool PlantInstance::contains(const QPointF &pos)
{
    return picture->contains(pos);
}

void PlantInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (zombieInstance->altitude > 0) {
        canTrigger = false;
        QSharedPointer<std::function<void(QUuid)> > triggerCheck(new std::function<void(QUuid)>);
        *triggerCheck = [this, triggerCheck] (QUuid zombieUuid) {
            (new Timer(picture, 1400, [this, zombieUuid, triggerCheck] {
                ZombieInstance *zombie = this->plantProtoType->scene->getZombie(zombieUuid);
                if (zombie) {
                    for (auto i: triggers[zombie->row]) {
                        if (zombie->hp > 0 && i->from <= zombie->ZX && i->to >= zombie->ZX && zombie->altitude > 0) {
                            normalAttack(zombie);
                            (*triggerCheck)(zombie->uuid);
                            return;
                        }
                    }
                }
                canTrigger = true;
            }))->start();
        };
        normalAttack(zombieInstance);
        (*triggerCheck)(zombieInstance->uuid);
    }
}

void PlantInstance::normalAttack(ZombieInstance *zombieInstance)
{
    qDebug() << plantProtoType->cName << uuid << "Attack" << zombieInstance->zombieProtoType->cName << zombieInstance;
}

void PlantInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    if (aKind == 0)
        hp -= attack;
    if (hp < 1 || aKind != 0)
        plantProtoType->scene->plantDie(this);
}

Peashooter::Peashooter()
{
    eName = "oPeashooter";
    cName = tr("Peashooter");
    beAttackedPointR = 51;
    sunNum = 100;
    cardGif = "Card/Plants/Peashooter.png";
    staticGif = "Plants/Peashooter/0.gif";
    normalGif = "Plants/Peashooter/Peashooter.gif";
    toolTip = tr("Shoots peas at zombies");
}

SnowPea::SnowPea()
{
    eName = "oSnowPea";
    cName = tr("Snow Pea");
    bKind = -1;
    beAttackedPointR = 51;
    sunNum = 175;
    cardGif = "Card/Plants/SnowPea.png";
    staticGif = "Plants/SnowPea/0.gif";
    normalGif = "Plants/SnowPea/SnowPea.gif";
    toolTip = tr("Slows down zombies with cold precision");
}

SunFlower::SunFlower()
{
    eName = "oSunflower";
    cName = tr("Sunflower");
    beAttackedPointR = 53;
    sunNum = 50;
    cardGif = "Card/Plants/SunFlower.png";
    staticGif = "Plants/SunFlower/0.gif";
    normalGif = "Plants/SunFlower/SunFlower1.gif";
    toolTip = tr("Makes extra Sun for placing plants");
}

SunFlowerInstance::SunFlowerInstance(const Plant *plant)
        : PlantInstance(plant),
          lightedGif("Plants/SunFlower/SunFlower2.gif")
{

}

void SunFlowerInstance::initTrigger()
{
    (new Timer(picture, 5000, [this] {
        QSharedPointer<std::function<void(void)> > generateSun(new std::function<void(void)>);
        *generateSun = [this, generateSun] {
            picture->setMovieOnNewLoop(lightedGif, [this, generateSun] {
                (new Timer(picture, 1000, [this, generateSun] {
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(25);
                    MoviePixmapItem *sunGif = sunGifAndOnFinished.first;
                    std::function<void(bool)> onFinished = sunGifAndOnFinished.second;
                    Coordinate &coordinate = plantProtoType->scene->getCoordinate();
                    double fromX = coordinate.getX(col) - sunGif->boundingRect().width() / 2 + 15,
                            toX = coordinate.getX(col) - qrand() % 80,
                            toY = coordinate.getY(row) - sunGif->boundingRect().height();
                    sunGif->setScale(0.6);
                    sunGif->setPos(fromX, toY - 25);
                    sunGif->start();
                    Animate(sunGif).move(QPointF((fromX + toX) / 2, toY - 50)).scale(0.9).speed(0.2).shape(
                                    QTimeLine::EaseOutCurve).finish()
                            .move(QPointF(toX, toY)).scale(1.0).speed(0.2).shape(QTimeLine::EaseInCurve).finish(
                                    onFinished);
                    picture->setMovieOnNewLoop(plantProtoType->normalGif, [this, generateSun] {
                        (new Timer(picture, 24000, [this, generateSun] {
                            (*generateSun)();
                        }))->start();
                    });
                }))->start();
            });
        };
        (*generateSun)();
    }))->start();
}

WallNut::WallNut()
{
    eName = "oWallNut";
    cName = tr("Wall-nut");
    hp = 4000;
    beAttackedPointR = 45;
    sunNum = 50;
    coolTime = 30;
    cardGif = "Card/Plants/WallNut.png";
    staticGif = "Plants/WallNut/0.gif";
    normalGif = "Plants/WallNut/WallNut.gif";
    toolTip = tr("Stops zombies with its chewy shell");
}

bool WallNut::canGrow(int x, int y) const
{
   if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (groundType == 1)
        return !plants.contains(1) || plants[1]->plantProtoType->eName == "oWallNut";
    return plants.contains(0) && (!plants.contains(1) || plants[1]->plantProtoType->eName == "oWallNut");

}

LawnCleaner::LawnCleaner()
{
    eName = "oLawnCleaner";
    cName = tr("Lawn Cleaner");
    beAttackedPointL = 0;
    beAttackedPointR = 71;
    sunNum = 0;
    staticGif = normalGif = "interface/LawnCleaner.png";
    canEat = 0;
    stature = 1;
    toolTip = tr("Normal lawn cleaner");
}

PoolCleaner::PoolCleaner()
{
    eName = "oPoolCleaner";
    cName = tr("Pool Cleaner");
    beAttackedPointR = 47;
    staticGif = normalGif = "interface/PoolCleaner.png";
    toolTip = tr("Pool Cleaner");
    update();
}

void WallNutInstance::initTrigger()
{}

WallNutInstance::WallNutInstance(const Plant *plant)
    : PlantInstance(plant)
{
    hurtStatus = 0;
    crackedGif1 = "Plants/WallNut/Wallnut_cracked1.gif";
    crackedGif2 = "Plants/WallNut/Wallnut_cracked2.gif";
}

void WallNutInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) {
        if (hp < 1334) {
            if (hurtStatus < 2) {
                hurtStatus = 2;
                picture->setMovie(crackedGif2);
                picture->start();
            }
        }
        else if (hp < 2667) {
            if (hurtStatus < 1) {
                hurtStatus = 1;
                picture->setMovie(crackedGif1);
                picture->start();
            }
        }
    }
}

LawnCleanerInstance::LawnCleanerInstance(const Plant *plant)
    : PlantInstance(plant)
{}

void LawnCleanerInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, attackedRX, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger } );
    plantProtoType->scene->addTrigger(row, trigger);
}

void LawnCleanerInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (zombieInstance->beAttacked && zombieInstance->altitude > 0) {
        canTrigger = 0;
        normalAttack(nullptr);
    }
}

void LawnCleanerInstance::normalAttack(ZombieInstance *zombieInstance)
{
    QMediaPlayer *player = new QMediaPlayer(plantProtoType->scene);
    player->setMedia(QUrl("qrc:/audio/lawnmower.mp3"));
    player->play();
    QSharedPointer<std::function<void(void)> > crush(new std::function<void(void)>);
    *crush = [this, crush] {
        for (auto zombie: plantProtoType->scene->getZombieOnRowRange(row, attackedLX, attackedRX)) {
            zombie->crushDie();
        }
        if (attackedLX > 900)
            plantProtoType->scene->plantDie(this);
        else {
            attackedLX += 10;
            attackedRX += 10;
            picture->setPos(picture->pos() + QPointF(10, 0));
            (new Timer(picture, 10, *crush))->start();
        }
    };
    (*crush)();
}

PeashooterInstance::PeashooterInstance(const Plant *plant)
    : PlantInstance(plant), firePea(new QMediaPlayer(picture))
{
    firePea->setMedia(QUrl("qrc:/audio/firepea.mp3"));
}

void PeashooterInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}

Bullet::Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue, int direction, int bKind)
        : scene(scene), type(type), row(row), direction(direction), from(from), bKind(bKind)
{
    count = 0;
    picture = new QGraphicsPixmapItem(gImageCache->load(QString("Plants/PB%1%2.gif").arg(type).arg(direction)));
    picture->setPos(x, y);
    picture->setZValue(zvalue);
}

Bullet::~Bullet()
{
    delete picture;
}

void Bullet::start()
{
    (new Timer(scene, 10, [this] {
        move();
    }))->start();
}

void Bullet::move()
{
    if (count++ == 10)
        scene->addItem(picture);
    int col = scene->getCoordinate().getCol(from);
    QMap<int, PlantInstance *> plants = scene->getPlant(col, row);
    if (type == 0 && plants.contains(1) && plants[1]->plantProtoType->eName == "oTorchwood") {
        ++type;
        picture->setPixmap(gImageCache->load(QString("Plants/PB%1%2.gif").arg(type).arg(direction)));
    }
    ZombieInstance *zombie = nullptr;
    if (direction == 0) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto iter = zombies.end(); iter-- != zombies.begin() && (*iter)->attackedLX <= from;) {
            if ((*iter)->hp > 0 && (*iter)->attackedRX >= from) {
                zombie = *iter;
                break;
            }
        }
    }
    // TODO: another direction
    if (zombie && zombie->altitude == 1) {
        // TODO: other attacks
        zombie->getPea(20, direction, bKind);
        picture->setPos(picture->pos() + QPointF(28, 0));
        picture->setPixmap(gImageCache->load("Plants/PeaBulletHit.gif"));
        (new Timer(scene, 100, [this] {
            delete this;
        }))->start();
    }
    else {
        from += direction ? -5 : 5;
        if (from < 900 && from > 100) {
            picture->setPos(picture->pos() + QPointF(direction ? -5 : 5, 0));
            (new Timer(scene, 10, [this] {
                move();
            }))->start();
        }
        else
            delete this;
    }
}

SnowPeaInstance::SnowPeaInstance(const Plant *plant)
    : PlantInstance(plant), firePea(new QMediaPlayer(picture))
{
    firePea->setMedia(QUrl("qrc:/audio/firepea.mp3"));
}

void SnowPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    // 使用 bKind=-1 表示寒冰豌豆，type=-1 对应蓝色的 PB-10.gif 子弹
    (new Bullet(plantProtoType->scene, plantProtoType->bKind, row, attackedLX,
                attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0, plantProtoType->bKind))->start();
}

// ========== 双发射手 (Repeater) ==========
Repeater::Repeater()
{
    eName = "oRepeater";
    cName = tr("Repeater");
    beAttackedPointR = 51;
    sunNum = 200;
    coolTime = 7.5;
    cardGif = "Card/Plants/Repeater.png";
    staticGif = "Plants/Repeater/0.gif";
    normalGif = "Plants/Repeater/Repeater.gif";
    toolTip = tr("Shoots two peas at a time");
}

RepeaterInstance::RepeaterInstance(const Plant *plant)
    : PlantInstance(plant), firePea(new QMediaPlayer(picture))
{
    firePea->setMedia(QUrl("qrc:/audio/firepea.mp3"));
}

void RepeaterInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    // 发射第一颗豌豆
    (new Bullet(plantProtoType->scene, 0, row, attackedLX,
                attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    // 短暂延迟后发射第二颗豌豆
    (new Timer(picture, 150, [this] {
        (new Bullet(plantProtoType->scene, 0, row, attackedLX,
                    attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
    }))->start();
}

// ========== 樱桃炸弹 (CherryBomb) ==========
CherryBomb::CherryBomb()
{
    eName = "oCherryBomb";
    cName = tr("Cherry Bomb");
    bKind = 1;
    beAttackedPointR = 80;
    sunNum = 150;
    coolTime = 50;
    canEat = false;
    cardGif = "Card/Plants/CherryBomb.png";
    staticGif = "Plants/CherryBomb/0.gif";
    normalGif = "Plants/CherryBomb/CherryBomb.gif";
    toolTip = tr("Explodes and destroys all zombies in an area");
}

CherryBombInstance::CherryBombInstance(const Plant *plant)
    : PlantInstance(plant), boomGif("Plants/CherryBomb/Boom.gif"), exploded(false)
{
}

void CherryBombInstance::initTrigger()
{
    // 樱桃炸弹放置后直接爆炸
    QMediaPlayer *player = new QMediaPlayer(picture);
    player->setMedia(QUrl("qrc:/audio/explosion.mp3"));
    player->play();
    // 先播放爆炸动画，覆盖中心区域
    picture->setMovie(boomGif);
    picture->setScale(2.0);
    QPointF center = picture->pos();
    picture->setPos(center.x() - 60, center.y() - 60);
    picture->start();
    (new Timer(picture, 800, [this] {
        if (exploded) return;
        exploded = true;
        // 3x3 范围爆炸，消灭范围内所有僵尸（灰烬死亡）
        GameScene *scene = plantProtoType->scene;
        for (int r = qMax(1, row - 1); r <= qMin(5, row + 1); ++r) {
            QList<ZombieInstance *> zombies = scene->getZombieOnRow(r);
            for (auto zombie : zombies) {
                if (zombie->hp > 0 && !zombie->goingDie) {
                    int zCol = scene->getCoordinate().getCol(zombie->ZX);
                    if (zCol >= col - 1 && zCol <= col + 1) {
                        zombie->boomDie();
                    }
                }
            }
        }
        scene->plantDie(this);
    }))->start();
}

void CherryBombInstance::triggerCheck(ZombieInstance *, Trigger *)
{
    // 樱桃炸弹不需要触发检测
}

// ========== 土豆雷 (PotatoMine) ==========
PotatoMine::PotatoMine()
{
    eName = "oPotatoMine";
    cName = tr("Potato Mine");
    beAttackedPointR = 47;
    sunNum = 25;
    coolTime = 30;
    canEat = false;
    cardGif = "Card/Plants/PotatoMine.png";
    staticGif = "Plants/PotatoMine/0.gif";
    normalGif = "Plants/PotatoMine/PotatoMine.gif";
    toolTip = tr("Sneaks up and explodes when a zombie steps on it");
}

PotatoMineInstance::PotatoMineInstance(const Plant *plant)
    : PlantInstance(plant),
      notReadyGif("Plants/PotatoMine/PotatoMineNotReady.gif"),
      mashGif("Plants/PotatoMine/PotatoMine_mashed.gif"),
      explosionGif("Plants/PotatoMine/ExplosionSpudow.gif"),
      isArmed(false), exploded(false)
{
    // 初始显示未准备状态
    picture->setMovie(notReadyGif);
}

void PotatoMineInstance::initTrigger()
{
    // 15秒后准备就绪，先播放发芽动画再切换
    (new Timer(picture, 15000, [this] {
        if (exploded) return;
        isArmed = true;
        // 播放发芽动画（土壤隆起效果）
        MoviePixmapItem *growEffect = new MoviePixmapItem("interface/GrowSoil.gif");
        growEffect->setPos(picture->x() - 20, picture->y() - 20);
        growEffect->setZValue(picture->zValue() + 1);
        plantProtoType->scene->addToGame(growEffect);
        growEffect->start();
        (new Timer(picture, 600, [this, growEffect] {
            growEffect->deleteLater();
            // 切换为已准备状态
            picture->setMovie(plantProtoType->normalGif);
            picture->start();
        }))->start();
    }))->start();
    // 设置触发区
    Trigger *trigger = new Trigger(this, attackedLX, attackedRX, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void PotatoMineInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    Q_UNUSED(trigger);
    if (exploded || !isArmed || zombieInstance->altitude <= 0) return;
    canTrigger = false;
    normalAttack(zombieInstance);
}

void PotatoMineInstance::normalAttack(ZombieInstance *zombieInstance)
{
    Q_UNUSED(zombieInstance);
    if (exploded) return;
    exploded = true;
    QMediaPlayer *player = new QMediaPlayer(picture);
    player->setMedia(QUrl("qrc:/audio/potato_mine.mp3"));
    player->play();
    // 第一阶段：土豆被压扁的动画
    picture->setMovie(mashGif);
    picture->start();
    // 第二阶段：爆炸动画，消灭周围僵尸（灰烬死亡）
    (new Timer(picture, 400, [this] {
        picture->setMovie(explosionGif);
        picture->setScale(1.5);
        QPointF center = picture->pos();
        picture->setPos(center.x() - 30, center.y() - 30);
        picture->start();
        (new Timer(picture, 600, [this] {
            // 消灭触发行上的僵尸（灰烬死亡）
            GameScene *scene = plantProtoType->scene;
            QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
            for (auto zombie : zombies) {
                if (zombie->hp > 0 && !zombie->goingDie) {
                    int zCol = scene->getCoordinate().getCol(zombie->ZX);
                    if (zCol >= col - 1 && zCol <= col + 1) {
                        zombie->boomDie();
                    }
                }
            }
            scene->plantDie(this);
        }))->start();
    }))->start();
}

Plant *PlantFactory(GameScene *scene, const QString &eName)
{
    Plant *plant = nullptr;
    if (eName == "oPeashooter")
        plant = new Peashooter;
    else if (eName == "oSnowPea")
        plant = new SnowPea;
    else if (eName == "oRepeater")
        plant = new Repeater;
    else if (eName == "oCherryBomb")
        plant = new CherryBomb;
    else if (eName == "oPotatoMine")
        plant = new PotatoMine;
    else if (eName == "oSunflower")
        plant = new SunFlower;
    else if (eName == "oWallNut")
        plant = new WallNut;
    else if (eName == "oLawnCleaner")
        plant = new LawnCleaner;
    else if (eName == "oPoolCleaner")
        plant = new PoolCleaner;
    if (plant) {
        plant->scene = scene;
        plant->update();
    }
    return plant;
}

PlantInstance *PlantInstanceFactory(const Plant *plant)
{
    if (plant->eName == "oPeashooter")
        return new PeashooterInstance(plant);
    if (plant->eName == "oSnowPea")
        return new SnowPeaInstance(plant);
    if (plant->eName == "oRepeater")
        return new RepeaterInstance(plant);
    if (plant->eName == "oCherryBomb")
        return new CherryBombInstance(plant);
    if (plant->eName == "oPotatoMine")
        return new PotatoMineInstance(plant);
    if (plant->eName == "oSunflower")
        return new SunFlowerInstance(plant);
    if (plant->eName == "oWallNut")
        return new WallNutInstance(plant);
    if (plant->eName == "oLawnCleaner")
        return new LawnCleanerInstance(plant);
    return new PlantInstance(plant);
}



