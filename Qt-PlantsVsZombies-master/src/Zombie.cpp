//
// Created by sun on 8/26/16.
//

#include "Zombie.h"
#include "GameScene.h"
#include "GameLevelData.h"
#include "ImageManager.h"
#include "MouseEventPixmapItem.h"
#include "Plant.h"
#include "Timer.h"
#include "Animate.h"
#include <cmath>
#include <QGraphicsEllipseItem>

Zombie::Zombie()
    : hp(270), level(1), speed(1.5),
      aKind(0), attack(100),
      canSelect(true), canDisplay(true),
      beAttackedPointL(82), beAttackedPointR(156),
      breakPoint(90), sunNum(0),
      damagePoint1(0),  // 0 = 不启用第一损伤阶段
      coolTime(0)
{}

bool Zombie::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 1;
}

void Zombie::update()
{
    QPixmap pic = gImageCache->load(staticGif);
    width = pic.width();
    height = pic.height();
}

Zombie1::Zombie1()
{
    eName = "oZombie";
    cName = tr("Zombie");
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    breakPoint = 90;  // std2 = 90
    cardGif = "Card/Zombies/Zombie.png";
    QString path = "Zombies/Zombie/";
    staticGif = path + "0.gif";
    normalGif = path + "Zombie.gif";
    attackGif = path + "ZombieAttack.gif";
    lostHeadGif = path + "ZombieLostHead.gif";
    lostHeadAttackGif = path + "ZombieLostHeadAttack.gif";
    headGif = path + "ZombieHead.gif";
    dieGif = path + "ZombieDie.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

Zombie2::Zombie2()
{
    eName = "oZombie2";
    damagePoint1 = hp * 2 / 3;
    normalGif = "Zombies/Zombie/Zombie2.gif";
    standGif = "Zombies/Zombie/2.gif";
}

Zombie3::Zombie3()
{
    eName = "oZombie3";
    damagePoint1 = hp * 2 / 3;
    normalGif = "Zombies/Zombie/Zombie3.gif";
    standGif = "Zombies/Zombie/3.gif";
}

FlagZombie::FlagZombie()
{
    eName = "oFlagZombie";
    cName = tr("Flag Zombie");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    speed = 2.2;
    beAttackedPointR = 101;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/FlagZombie/";
    cardGif = "Card/Zombies/FlagZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "FlagZombie.gif";
    attackGif = path + "FlagZombieAttack.gif";
    lostHeadGif = path + "FlagZombieLostHead.gif";
    lostHeadAttackGif = path + "FlagZombieLostHeadAttack.gif";
    headGif = path + "FlagZombieHead.gif";
    dieGif = path + "FlagZombieDie.gif";
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

ZombieInstance::ZombieInstance(const Zombie *zombie)
    : zombieProtoType(zombie), picture(new MoviePixmapItem),
      attackMusic(new QMediaPlayer(picture)),
      hitMusic(new QMediaPlayer(picture))
{
    uuid = QUuid::createUuid();
    hp = zombieProtoType->hp;
    baseSpeed = zombie->speed;
    speed = baseSpeed;
    altitude = 1;
    beAttacked = true;
    isAttacking = false;
    goingDie = false;
    damageStage1 = false;
    normalGif = zombie->normalGif;
    attackGif = zombie->attackGif;
}

void ZombieInstance::birth(int row)
{
    ZX = attackedLX = zombieProtoType->scene->getCoordinate().getX(11);
    X = attackedLX - zombieProtoType->beAttackedPointL;
    attackedRX = X + zombieProtoType->beAttackedPointR;
    this->row = row;

    Coordinate &coordinate = zombieProtoType->scene->getCoordinate();
    picture->setMovie(normalGif);
    picture->setPos(X, coordinate.getY(row) - zombieProtoType->height - 10);
    picture->setZValue(3 * row + 1);
    shadowPNG = new QGraphicsPixmapItem(gImageCache->load("interface/plantShadow.png"));
    shadowPNG->setPos(zombieProtoType->beAttackedPointL - 10, zombieProtoType->height - 22);
    shadowPNG->setFlag(QGraphicsItem::ItemStacksBehindParent);
    shadowPNG->setParentItem(picture);
    picture->start();
    zombieProtoType->scene->addToGame(picture);
}

void ZombieInstance::checkActs()
{
    if (hp < 1) return;
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        // Clean up zombies that have gone well past the house (avoid memory leak)
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
        }
    }
}

void ZombieInstance::judgeAttack()
{
    bool tempIsAttacking = false;
    PlantInstance *plant = nullptr;
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col >= 1 && col <= 9) {
        auto plants = zombieProtoType->scene->getPlant(col, row);
        QList<int> keys = plants.keys();
        qSort(keys.begin(), keys.end(), [](int a, int b) { return b < a; });
        for (auto key: keys) {
            plant = plants[key];
            if (plant->plantProtoType->canEat && plant->attackedRX >= ZX && plant->attackedLX <= ZX) {
                tempIsAttacking = true;
                break;
            }
        }
    }
    if (tempIsAttacking != isAttacking) {
        isAttacking = tempIsAttacking;
        if (damageStage1 && !zombieProtoType->damageGif1.isEmpty()) {
            picture->setMovie(isAttacking ? zombieProtoType->damageAttackGif1 : zombieProtoType->damageGif1);
        } else {
            picture->setMovie(isAttacking ? attackGif : normalGif);
        }
        picture->start();
    }
    if (isAttacking)
        normalAttack(plant);
}

void ZombieInstance::normalAttack(PlantInstance *plantInstance)
{
    if (qrand() % 2)
        attackMusic->setMedia(QUrl("qrc:/audio/chomp.mp3"));
    else
        attackMusic->setMedia(QUrl("qrc:/audio/chompsoft.mp3"));
    attackMusic->play();
    QUuid myUuid = uuid;
    (new Timer(this->picture, 500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        if (qrand() % 2)
            attackMusic->setMedia(QUrl("qrc:/audio/chomp.mp3"));
        else
            attackMusic->setMedia(QUrl("qrc:/audio/chompsoft.mp3"));
        attackMusic->play();
    }))->start();
    QUuid plantUuid = plantInstance->uuid;
    (new Timer(this->picture, 1000, [this, plantUuid, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        if (beAttacked) {
            PlantInstance *plant = zombieProtoType->scene->getPlant(plantUuid);
            if (plant)
                plant->getHurt(this, zombieProtoType->aKind, zombieProtoType->attack);
            judgeAttack();
        }
    }))->start();
}

ZombieInstance::~ZombieInstance()
{
    picture->deleteLater();
}


// 对僵尸施加减速：multiplier 为速度乘数（<1），durationMs 为持续毫秒
void ZombieInstance::applySlow(qreal multiplier, int durationMs)
{
    if (multiplier >= 1.0 || durationMs <= 0 || goingDie)
        return;
    // 将乘数加入列表
    slowMultipliers.append(multiplier);
    // 重新计算实际速度（baseSpeed * 所有乘数之积）
    qreal prod = 1.0;
    for (auto m: slowMultipliers)
        prod *= m;
    speed = baseSpeed * prod;

    // 在 durationMs 后移除该乘数；以 picture 作为父对象，僵尸死亡时回调会被取消
    QSharedPointer<qreal> multPtr(new qreal(multiplier));
    (new Timer(picture, durationMs, [this, multPtr] {
        // 移除一次该乘数
        for (int i = 0; i < slowMultipliers.size(); ++i) {
            if (qFuzzyCompare(slowMultipliers[i] + 1.0, *multPtr + 1.0)) {
                slowMultipliers.removeAt(i);
                break;
            }
        }
        qreal prod2 = 1.0;
        for (auto m: slowMultipliers)
            prod2 *= m;
        speed = baseSpeed * prod2;
    }))->start();
}

void ZombieInstance::crushDie()
{
    if (goingDie)
        return;
    goingDie =  true;
    hp = 0;
    MoviePixmapItem *goingDieHead = new MoviePixmapItem(zombieProtoType->headGif);
    goingDieHead->setPos(zombieProtoType->beAttackedPointL, -20);
    goingDieHead->setParentItem(picture);
    goingDieHead->start();
    shadowPNG->setPixmap(QPixmap());
    picture->stop();
    picture->setPixmap(QPixmap());
    QUuid myUuid = uuid;
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::getPea(int attack, int direction)
{
    playNormalballAudio();
    getHit(attack);
}

void ZombieInstance::getHit(int attack)
{
    if (!beAttacked || goingDie)
        return;
    hp -= attack;
    if (hp < zombieProtoType->breakPoint) {
        // 第二阶段：断头 — 移除损伤染血效果
        picture->setGraphicsEffect(nullptr);
        if (isAttacking)
            picture->setMovie(zombieProtoType->lostHeadAttackGif);
        else
            picture->setMovie(zombieProtoType->lostHeadGif);
        picture->start();
        MoviePixmapItem *goingDieHead = new MoviePixmapItem(zombieProtoType->headGif);
        goingDieHead->setPos(attackedLX, picture->y() - 20);
        goingDieHead->setZValue(picture->zValue());
        zombieProtoType->scene->addToGame(goingDieHead);
        goingDieHead->start();
        (new Timer(zombieProtoType->scene, 2000, [goingDieHead] {
            goingDieHead->deleteLater();
        }))->start();
        beAttacked = 0;
        autoReduceHp();
    }
    else if (!damageStage1 && zombieProtoType->damagePoint1 > 0 && hp < zombieProtoType->damagePoint1) {
        // 第一阶段：损伤（原版表现为断臂，本项目模拟断臂粒子效果 + 染血）
        damageStage1 = true;
        if (zombieProtoType->damageGif1.isEmpty()) {
            // 红色染血着色效果
            QGraphicsColorizeEffect *damageEffect = new QGraphicsColorizeEffect;
            damageEffect->setColor(QColor(170, 40, 30));
            damageEffect->setStrength(0.55);
            picture->setGraphicsEffect(damageEffect);
        } else {
            if (isAttacking)
                picture->setMovie(zombieProtoType->damageAttackGif1);
            else
                picture->setMovie(zombieProtoType->damageGif1);
            picture->start();
        }
        // 模拟"断臂"粒子效果：散射出几个小碎片
        QPointF center(attackedLX, picture->y() + picture->boundingRect().height() * 0.4);
        for (int i = 0; i < 5; ++i) {
            QGraphicsEllipseItem *particle = new QGraphicsEllipseItem(-3, -3, 6, 6);
            particle->setBrush(QColor(180, 50, 30, 200));
            particle->setPen(Qt::NoPen);
            particle->setPos(center);
            particle->setZValue(picture->zValue() + 0.1);
            zombieProtoType->scene->addItem(particle);
            // 粒子飞散动画（用 Timer 驱动）
            qreal angle = (qrand() % 360) * M_PI / 180.0;
            qreal dist = 20 + qrand() % 30;
            QPointF target = center + QPointF(cos(angle) * dist, sin(angle) * dist - 15);
            int steps = 8;
            int stepDelay = 60;
            for (int s = 1; s <= steps; ++s) {
                qreal progress = (qreal)s / steps;
                (new Timer(zombieProtoType->scene, stepDelay * s, [particle, center, target, progress] {
                    particle->setPos(center + (target - center) * progress);
                    particle->setOpacity(1.0 - progress);
                    if (progress >= 1.0) {
                        if (particle->scene())
                            particle->scene()->removeItem(particle);
                        delete particle;
                    }
                }))->start();
            }
        }
        // 受伤闪红
        picture->setOpacity(0.25);
        (new Timer(picture, 200, [this] {
            picture->setOpacity(1.0);
        }))->start();
    }
    else {
        // 普通受伤闪红
        picture->setOpacity(0.25);
        (new Timer(picture, 150, [this] {
            if (hp < zombieProtoType->breakPoint) return;
            picture->setOpacity(1.0);
        }))->start();
    }
}

void ZombieInstance::autoReduceHp()
{
    (new Timer(picture, 1000, [this] {
        if (goingDie) return; // guard against double-death
        hp -= 60;
        if (hp < 1)
            normalDie();
        else
            autoReduceHp();
    }))->start();
}

void ZombieInstance::normalDie()
{
    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    picture->setMovie(zombieProtoType->dieGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 2500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::boomDie()
{
    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    beAttacked = false;
    if (shadowPNG)
        shadowPNG->setPixmap(QPixmap());
    QString boomGif = zombieProtoType->boomDieGif.isEmpty() ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    picture->setMovie(boomGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::ashDie()
{
    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    beAttacked = false;
    if (shadowPNG)
        shadowPNG->setPixmap(QPixmap());
    // 使用 BoomDie.gif 播放化为灰烬的动画
    QString boomGif = zombieProtoType->boomDieGif.isEmpty() ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    picture->setMovie(boomGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::playNormalballAudio()
{
    hitMusic->stop();
    switch (qrand() % 3) {
        case 0: hitMusic->setMedia(QUrl("qrc:/audio/splat1.mp3")); break;
        case 1: hitMusic->setMedia(QUrl("qrc:/audio/splat2.mp3")); break;
        default: hitMusic->setMedia(QUrl("qrc:/audio/splat3.mp3")); break;
    }
    hitMusic->play();
}


OrnZombieInstance1::OrnZombieInstance1(const Zombie *zombie)
    : ZombieInstance(zombie)
{
    ornHp = getZombieProtoType()->ornHp;
    originalOrnHp = ornHp;  // 记录饰品原始HP，用于损伤阶段计算
    hasOrnaments = true;
}

const OrnZombie1 *OrnZombieInstance1::getZombieProtoType()
{
    return static_cast<const OrnZombie1 *>(zombieProtoType);
}

void OrnZombieInstance1::getHit(int attack)
{
    if (hasOrnaments) {
        ornHp -= attack;
        if (ornHp < 1) {
            hp += ornHp;
            hasOrnaments = false;
            // 移除饰品损伤效果
            picture->setGraphicsEffect(nullptr);
            normalGif = getZombieProtoType()->ornLostNormalGif;
            attackGif = getZombieProtoType()->ornLostAttackGif;
            picture->setMovie(isAttacking ? attackGif : normalGif);
            picture->start();
        } else {
            // 饰品损伤阶段：根据剩余HP比例应用染色效果
            // 原版：损伤点1 = 2/3, 损伤点2 = 1/3
            int ornDmg1 = originalOrnHp * 2 / 3;  // 轻微损伤
            int ornDmg2 = originalOrnHp * 1 / 3;  // 严重损伤
            QGraphicsColorizeEffect *ornEffect = new QGraphicsColorizeEffect;
            if (ornHp < ornDmg2) {
                // 严重损伤：深色/金属色
                ornEffect->setColor(QColor(100, 90, 80));
                ornEffect->setStrength(0.5);
            } else if (ornHp < ornDmg1) {
                // 轻微损伤：浅色
                ornEffect->setColor(QColor(140, 130, 110));
                ornEffect->setStrength(0.25);
            }
            picture->setGraphicsEffect(ornEffect);
        }
        // 受击闪白
        picture->setOpacity(0.5);
        (new Timer(picture, 100, [this] {
            picture->setOpacity(1);
        }))->start();
    }
    else
        ZombieInstance::getHit(attack);
}

ConeheadZombie::ConeheadZombie()
{
    eName = "oConeheadZombie";
    cName = tr("Conehead Zombie");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    ornHp = 370;
    level = 2;
    sunNum = 75;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/ConeheadZombie/";
    cardGif = "Card/Zombies/ConeheadZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "ConeheadZombie.gif";
    attackGif = path + "ConeheadZombieAttack.gif";
    // 本体损伤使用红色染色效果（原版为断臂动画，本项目无对应GIF）
    ornLostNormalGif =  "Zombies/Zombie/Zombie.gif";
    ornLostAttackGif = "Zombies/Zombie/ZombieAttack.gif";
    lostHeadGif = path + "ConeheadZombieLostHead.gif";
    lostHeadAttackGif = path + "ConeheadZombieLostHeadAttack.gif";
    headGif = path + "ConeheadZombieHead.gif";
    dieGif = path + "ConeheadZombieDie.gif";
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

ConeheadZombieInstance::ConeheadZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void ConeheadZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        hitMusic->stop();
        hitMusic->setMedia(QUrl("qrc:/audio/plastichit.mp3"));
        hitMusic->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

BucketheadZombieInstance::BucketheadZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void BucketheadZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        hitMusic->stop();
        if (qrand() % 2)
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        hitMusic->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

BucketheadZombie::BucketheadZombie()
{
    eName = "oBucketheadZombie";
    cName = tr("Buckethead Zombie");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    ornHp = 1100;
    level = 3;
    sunNum = 125;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/BucketheadZombie/";
    cardGif = "Card/Zombies/BucketheadZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "BucketheadZombie.gif";
    attackGif = path + "BucketheadZombieAttack.gif";
    // 本体损伤使用红色染色效果
    ornLostNormalGif =  "Zombies/Zombie/Zombie2.gif";
    ornLostAttackGif = "Zombies/Zombie/ZombieAttack.gif";
    lostHeadGif = path + "BucketheadZombieLostHead.gif";
    lostHeadAttackGif = path + "BucketheadZombieLostHeadAttack.gif";
    headGif = path + "BucketheadZombieHead.gif";
    dieGif = path + "BucketheadZombieDie.gif";
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

PoleVaultingZombie::PoleVaultingZombie()
{
    eName = "oPoleVaultingZombie";
    cName = tr("Pole Vaulting Zombie");
    hp = 500;
    damagePoint1 = hp * 2 / 3;  // 333
    speed = 3.2;
    beAttackedPointL = 215;
    beAttackedPointR = 260;
    level = 2;
    sunNum = 75;
    QString path = "Zombies/PoleVaultingZombie/";
    cardGif = "Card/Zombies/PoleVaultingZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "PoleVaultingZombieWalk.gif";
    attackGif = path + "PoleVaultingZombieAttack.gif";
    lostHeadGif = path + "PoleVaultingZombieLostHeadWalk.gif";
    lostHeadAttackGif = path + "PoleVaultingZombieLostHeadAttack.gif";
    headGif = path + "PoleVaultingZombieHead.gif";
    dieGif = path + "PoleVaultingZombieDie.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

PoleVaultingZombieInstance::PoleVaultingZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), hasPole(true), jumping(false),
      poleVaultMusic(new QMediaPlayer(picture))
{
    this->normalGif = zombie->normalGif;
    this->attackGif = zombie->attackGif;
    poleVaultMusic->setMedia(QUrl("qrc:/audio/polevault.mp3"));
}

void PoleVaultingZombieInstance::checkActs()
{
    if (hp < 1) return;
    // 撑杆跳：跳过第一个植物
    if (hasPole && !jumping) {
        int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
        if (col >= 1 && col <= 9) {
            auto plants = zombieProtoType->scene->getPlant(col, row);
            for (auto plant: plants.values()) {
                if (plant->plantProtoType->canEat && plant->attackedRX >= ZX && plant->attackedLX <= ZX) {
                    jumping = true;
                    hasPole = false;
                    poleVaultMusic->stop();
                    poleVaultMusic->play();
                    picture->setMovie("Zombies/PoleVaultingZombie/PoleVaultingZombieJump.gif");
                    picture->start();
                    (new Timer(picture, 800, [this] {
                        jumping = false;
                        ZX = attackedLX -= 80;
                        X -= 80;
                        attackedRX -= 80;
                        picture->setX(X);
                        picture->setMovie(zombieProtoType->normalGif);
                        picture->start();
                        this->normalGif = zombieProtoType->normalGif;
                        this->attackGif = zombieProtoType->attackGif;
                    }))->start();
                    return;
                }
            }
        }
    }
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking && !jumping) {
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
        }
    }
}

void PoleVaultingZombieInstance::playNormalballAudio()
{
    ZombieInstance::playNormalballAudio();
}

NewspaperZombie::NewspaperZombie()
{
    eName = "oNewspaperZombie";
    cName = tr("读报僵尸");
    hp = 270;
    ornHp = 200;
    speed = 1.5;
    level = 2;
    sunNum = 75;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    QString path = "Zombies/NewspaperZombie/";
    cardGif = "Card/Zombies/NewspaperZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "HeadWalk0.gif";
    attackGif = path + "HeadAttack0.gif";
    // 损伤第一阶段用HeadWalk1.gif（稍微不同的姿态）
    damageGif1 = path + "HeadWalk1.gif";
    damageAttackGif1 = path + "HeadAttack1.gif";
    ornLostNormalGif = path + "LostNewspaper.gif";
    ornLostAttackGif = path + "LostHeadAttack0.gif";
    headGif = path + "Head.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

NewspaperZombieInstance::NewspaperZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie), isAngry(false),
      angryMusic(new QMediaPlayer(picture))
{
    angryMusic->setMedia(QUrl("qrc:/audio/newspaper_rarrgh2.mp3"));
}

void NewspaperZombieInstance::birth(int row)
{
    OrnZombieInstance1::birth(row);
    // 报纸已在 HeadWalk0.gif 动画中，不需要额外覆盖层
}

void NewspaperZombieInstance::getHit(int attack)
{
    if (hasOrnaments) {
        ornHp -= attack;
        if (ornHp < 1) {
            hp += ornHp;
            hasOrnaments = false;
            // 移除饰品损伤效果
            picture->setGraphicsEffect(nullptr);
            if (!isAngry) {
                isAngry = true;
                baseSpeed *= 2.0;
                speed = baseSpeed;
                angryMusic->stop();
                angryMusic->play();
            }
            normalGif = getZombieProtoType()->ornLostNormalGif;
            attackGif = getZombieProtoType()->ornLostAttackGif;
            picture->setMovie(isAttacking ? attackGif : normalGif);
            picture->start();
        } else {
            // 报纸损伤阶段
            int ornDmg1 = originalOrnHp * 2 / 3;
            int ornDmg2 = originalOrnHp * 1 / 3;
            QGraphicsColorizeEffect *ornEffect = new QGraphicsColorizeEffect;
            if (ornHp < ornDmg2) {
                ornEffect->setColor(QColor(100, 90, 80));
                ornEffect->setStrength(0.5);
            } else if (ornHp < ornDmg1) {
                ornEffect->setColor(QColor(140, 130, 110));
                ornEffect->setStrength(0.25);
            }
            picture->setGraphicsEffect(ornEffect);
        }
        picture->setOpacity(0.5);
        (new Timer(picture, 100, [this] {
            picture->setOpacity(1);
        }))->start();
    }
    else
        ZombieInstance::getHit(attack);
}

void NewspaperZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        hitMusic->stop();
        hitMusic->setMedia(QUrl("qrc:/audio/plastichit.mp3"));
        hitMusic->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

FootballZombie::FootballZombie()
{
    eName = "oFootballZombie";
    cName = tr("橄榄球僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    ornHp = 1400;
    speed = 2.5;
    level = 4;
    sunNum = 175;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/FootballZombie/";
    cardGif = "Card/Zombies/FootballZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "FootballZombie.gif";
    attackGif = path + "FootballZombieAttack.gif";
    // 本体损伤使用红色染色效果
    damageGif1 = path + "OrnLost.gif";
    damageAttackGif1 = path + "OrnLostAttack.gif";
    ornLostNormalGif = path + "FootballZombieOrnLost.gif";
    ornLostAttackGif = path + "FootballZombieOrnLostAttack.gif";
    lostHeadGif = path + "LostHead.gif";
    lostHeadAttackGif = path + "LostHeadAttack.gif";
    headGif = path + "FootballZombieHead.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

FootballZombieInstance::FootballZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{
}

void FootballZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        hitMusic->stop();
        if (qrand() % 2)
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        hitMusic->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

// ===================== 铁网门僵尸 =====================
ScreenDoorZombie::ScreenDoorZombie()
{
    eName = "oScreenDoorZombie";
    cName = tr("铁网门僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    ornHp = 1100;
    speed = 1.5;
    level = 3;
    sunNum = 125;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/ScreenDoorZombie/";
    cardGif = "Card/Zombies/ScreenDoorZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "ScreenDoorZombie.gif";
    attackGif = path + "ScreenDoorZombieAttack.gif";
    // 本体损伤使用红色染色效果
    ornLostNormalGif = "Zombies/Zombie/Zombie.gif";
    ornLostAttackGif = "Zombies/Zombie/ZombieAttack.gif";
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
    // This zombie doesn't have BoomDie.gif in its folder, use default
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    dieGif = "Zombies/Zombie/ZombieDie.gif";
    lostHeadGif = path + "LostHeadWalk1.gif";
    lostHeadAttackGif = path + "LostHeadAttack1.gif";
    headGif = path + "HeadWalk1.gif";
}

ScreenDoorZombieInstance::ScreenDoorZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void ScreenDoorZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        hitMusic->stop();
        if (qrand() % 2)
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            hitMusic->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        hitMusic->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

// ===================== 小丑僵尸 =====================
JackinTheBoxZombie::JackinTheBoxZombie()
{
    eName = "oJackinTheBoxZombie";
    cName = tr("小丑僵尸");
    hp = 500;
    damagePoint1 = hp * 2 / 3;  // 333
    speed = 2.2;
    level = 3;
    sunNum = 100;
    beAttackedPointL = 80;
    beAttackedPointR = 30;
    breakPoint = 90;
    QString path = "Zombies/JackinTheBoxZombie/";
    cardGif = "Card/Zombies/JackboxZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk.gif";
    attackGif = path + "Attack.gif";
    lostHeadGif = path + "LostHead.gif";
    lostHeadAttackGif = path + "LostHeadAttack.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

JackinTheBoxZombieInstance::JackinTheBoxZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), exploded(false), walkTicks(0)
{}

void JackinTheBoxZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;
    if (exploded) {
        // 爆炸后僵尸直接死亡
        ashDie();
        return;
    }
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
            return;
        }
        // 每走一段距离检查是否爆炸
        walkTicks++;
        if (walkTicks > 50) {
            // 检查周围是否有植物，有概率爆炸
            int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
            if (col >= 1 && col <= 9) {
                auto plants = zombieProtoType->scene->getPlant(col, row);
                for (auto plant: plants.values()) {
                    if (plant->plantProtoType->canEat
                        && plant->attackedRX >= ZX && plant->attackedLX <= ZX) {
                        // 靠近植物，立即爆炸
                        exploded = true;
                        picture->setMovie("Zombies/JackinTheBoxZombie/Boom.gif");
                        picture->start();
                        // 对周围植物造成伤害
                        QMediaPlayer *boomSound = new QMediaPlayer(picture);
                        boomSound->setMedia(QUrl("qrc:/audio/cherrybomb.mp3"));
                        boomSound->play();
                        QUuid myUuid = uuid;
                        (new Timer(picture, 800, [this, myUuid] {
                            ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
                            if (!self || self != this) return;
                            // 伤害周围植物
                            int curCol = zombieProtoType->scene->getCoordinate().getCol(ZX);
                            for (int r = row - 1; r <= row + 1; ++r) {
                                if (r < 1 || r > zombieProtoType->scene->getCoordinate().rowCount()) continue;
                                for (int c = curCol - 1; c <= curCol + 1; ++c) {
                                    auto nearbyPlants = zombieProtoType->scene->getPlant(c, r);
                                    for (auto p: nearbyPlants.values()) {
                                        if (p->canTrigger)
                                            p->getHurt(this, 0, 1800);
                                    }
                                }
                            }
                            ashDie();
                        }))->start();
                        return;
                    }
                }
            }
            walkTicks = 0;
        }
    }
}

// ===================== 舞王僵尸 =====================
DancingZombie::DancingZombie()
{
    eName = "oDancingZombie";
    cName = tr("舞王僵尸");
    hp = 500;
    damagePoint1 = hp * 2 / 3;  // 333
    speed = 1.5;
    level = 4;
    sunNum = 150;
    beAttackedPointL = 80;
    beAttackedPointR = 30;
    breakPoint = 90;
    QString path = "Zombies/DancingZombie/";
    cardGif = "Card/Zombies/DancingZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Dancing.gif";
    attackGif = path + "Attack.gif";
    lostHeadGif = path + "LostHead.gif";
    lostHeadAttackGif = path + "LostHeadAttack.gif";
    headGif = path + "Head.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "0.gif"; // no 1.gif in folder
}

DancingZombieInstance::DancingZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), walkDistance(0), danceTimer(0), replenishCooldown(0), hasSummoned(false)
{
    // 原版：初始太空滑步速度(约1.2s/格)，召唤后降速(约5.5s/格)
    speed = 6.5; // 初始快速前进
}

void DancingZombieInstance::spawnAllBackupDancers()
{
    if (goingDie) return;
    GameScene *scene = zombieProtoType->scene;
    Zombie *backupProto = scene->getZombieProtoType("oBackupDancer");
    if (!backupProto) return;

    // 原版：在舞王僵尸的上下左右各召唤一个伴舞
    // 游戏中只有一行，所以在上行、下行、后方(2个不同x位置)放置
    int targetRows[] = { row - 1, row + 1, row, row };
    qreal xOffsets[] = { 0, 0, -100, -50 };
    for (int i = 0; i < 4; ++i) {
        int r = targetRows[i];
        if (r < 1 || r > scene->getGameLevelData()->LF.size()) continue;
        if (scene->getGameLevelData()->LF[r-1] != 1) continue; // 只放在可行走行
        ZombieInstance *backup = ZombieInstanceFactory(backupProto);
        backup->birth(r);
        backup->X = X + xOffsets[i];
        backup->attackedLX = X + xOffsets[i];
        backup->ZX = X + xOffsets[i];
        backup->attackedRX = backup->X + backup->zombieProtoType->beAttackedPointR;
        backup->picture->setX(backup->X);
        backup->picture->setY(backup->picture->y());
        scene->addZombie(backup);
        backupDancerUuids.append(backup->uuid);
    }
}

void DancingZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        qreal oldX = X;
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
            return;
        }

        if (!hasSummoned) {
            // 原版：太空滑步约3格后召唤
            walkDistance += qAbs(X - oldX);
            qreal gridWidth = 80.0; // 一格约80像素
            if (walkDistance >= gridWidth * 3.0) {
                hasSummoned = true;
                spawnAllBackupDancers();
                // 召唤后降速
                speed = 1.2; // 约5.5s/格
                danceTimer = 0;
            }
        } else {
            // 召唤后，每走4步集体舞蹈一次(用计时器模拟)
            danceTimer++;
            if (danceTimer > 40) {
                danceTimer = 0;
                // 原版：舞蹈动作
                picture->setMovie(zombieProtoType->normalGif);
                picture->start();
                // 清理已死亡的伴舞UUID
                GameScene *scene = zombieProtoType->scene;
                for (int i = backupDancerUuids.size() - 1; i >= 0; --i) {
                    if (!scene->getZombie(backupDancerUuids[i])) {
                        backupDancerUuids.removeAt(i);
                    }
                }
                // 检查伴舞是否减员，减员则补充（每200帧约6-7秒最多补充一次，避免刷屏崩溃）
                replenishCooldown++;
                int aliveDancers = backupDancerUuids.size();
                if (aliveDancers < 4 && replenishCooldown > 200) {
                    replenishCooldown = 0;
                    int need = 4 - aliveDancers;
                    Zombie *backupProto = scene->getZombieProtoType("oBackupDancer");
                    if (backupProto) {
                        int targetRows[] = { row - 1, row + 1, row, row };
                        qreal xOffsets[] = { 0, 0, -100, -50 };
                        int added = 0;
                        for (int i = 0; i < 4 && added < need; ++i) {
                            int r = targetRows[i];
                            if (r < 1 || r > scene->getGameLevelData()->LF.size()) continue;
                            if (scene->getGameLevelData()->LF[r-1] != 1) continue;
                            ZombieInstance *backup = ZombieInstanceFactory(backupProto);
                            backup->birth(r);
                            backup->X = X + xOffsets[i];
                            backup->attackedLX = X + xOffsets[i];
                            backup->ZX = X + xOffsets[i];
                            backup->attackedRX = backup->X + backup->zombieProtoType->beAttackedPointR;
                            backup->picture->setX(backup->X);
                            backup->picture->setY(backup->picture->y());
                            scene->addZombie(backup);
                            backupDancerUuids.append(backup->uuid);
                            added++;
                        }
                    }
                }
            }
        }
    }
}

void DancingZombieInstance::normalDie()
{
    if (goingDie) return;
    goingDie = true;
    hp = 0;
    picture->setMovie(zombieProtoType->dieGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 2500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

// ===================== 伴舞僵尸 =====================
BackupDancer::BackupDancer()
{
    eName = "oBackupDancer";
    cName = tr("伴舞僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    speed = 1.2;
    level = 1;
    sunNum = 25;
    beAttackedPointL = 80;
    beAttackedPointR = 30;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/BackupDancer/";
    cardGif = "Card/Zombies/BackupDancer.png";
    staticGif = path + "0.gif";
    normalGif = path + "BackupDancer.gif";
    attackGif = path + "Attack.gif";
    lostHeadGif = path + "LostHead.gif";
    lostHeadAttackGif = path + "LostHeadAttack.gif";
    // 本体损伤使用红色染色效果
    headGif = path + "Head.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "0.gif"; // no 1.gif in folder
}

BackupDancerInstance::BackupDancerInstance(const Zombie *zombie)
    : ZombieInstance(zombie)
{}

// ===================== 潜水僵尸 =====================
SnorkelZombie::SnorkelZombie()
{
    eName = "oSnorkelZombie";
    cName = tr("潜水僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    speed = 1.5;
    level = 2;
    sunNum = 50;
    beAttackedPointL = 80;
    beAttackedPointR = 30;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/SnorkelZombie/";
    cardGif = "Card/Zombies/SnorkelZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";
    attackGif = path + "Attack.gif";
    lostHeadGif = path + "Walk2.gif";
    lostHeadAttackGif = path + "Attack.gif";
    dieGif = path + "Die.gif";
    headGif = path + "Head.gif";
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

SnorkelZombieInstance::SnorkelZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), submerged(true), visCheckTimer(0)
{}

void SnorkelZombieInstance::updateVisibility()
{
    // 检查当前位置是否有植物
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    bool nearPlant = false;
    if (col >= 1 && col <= 9) {
        auto plants = zombieProtoType->scene->getPlant(col, row);
        for (auto plant: plants.values()) {
            if (plant->plantProtoType->canEat
                && plant->attackedRX >= ZX - 20 && plant->attackedLX <= ZX + 20) {
                nearPlant = true;
                break;
            }
        }
    }
    if (nearPlant && submerged) {
        submerged = false;
        picture->setOpacity(1.0);
        picture->setMovie(normalGif);
        picture->start();
    } else if (!nearPlant && !submerged) {
        submerged = true;
        picture->setOpacity(0.15);
        picture->setMovie(normalGif);
        picture->start();
    }
}

void SnorkelZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;
    // 每5帧检查一次可见性，减少性能开销
    visCheckTimer++;
    if (visCheckTimer >= 5) {
        visCheckTimer = 0;
        updateVisibility();
    }
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        // 潜水状态下移动速度减半
        qreal moveSpeed = submerged ? speed * 0.5 : speed;
        attackedRX -= moveSpeed;
        ZX = attackedLX -= moveSpeed;
        X -= moveSpeed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
        }
    }
}

void SnorkelZombieInstance::getHit(int attack)
{
    // 潜水状态下受到伤害减半
    if (submerged) {
        ZombieInstance::getHit(attack / 2);
    } else {
        ZombieInstance::getHit(attack);
    }
}

// ===================== 海豚骑士僵尸 =====================
DolphinRiderZombie::DolphinRiderZombie()
{
    eName = "oDolphinRiderZombie";
    cName = tr("海豚骑士僵尸");
    hp = 500;
    damagePoint1 = hp * 2 / 3;  // std1 = 333
    speed = 3.2;
    level = 2;
    sunNum = 75;
    beAttackedPointL = 120;
    beAttackedPointR = 160;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/DolphinRiderZombie/";
    cardGif = "Card/Zombies/DolphinRiderZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";
    attackGif = path + "Attack.gif";
    // 多阶段损伤动画：Walk2=轻伤, Walk3=重伤, Walk4=濒死
    damageGif1 = path + "Walk2.gif";
    damageAttackGif1 = path + "Attack.gif";
    lostHeadGif = path + "Walk4.gif";
    lostHeadAttackGif = path + "Attack.gif";
    dieGif = path + "Die.gif";
    headGif = path + "Head.gif";
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

DolphinRiderZombieInstance::DolphinRiderZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), jumped(false), jumpTimer(0)
{
    this->normalGif = zombie->normalGif;
    this->attackGif = zombie->attackGif;
}

void DolphinRiderZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;
    // 海豚骑士跳跃：跳过遇到的第一个植物
    if (!jumped) {
        int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
        if (col >= 1 && col <= 9) {
            auto plants = zombieProtoType->scene->getPlant(col, row);
            for (auto plant: plants.values()) {
                if (plant->plantProtoType->canEat
                    && plant->attackedRX >= ZX && plant->attackedLX <= ZX) {
                    jumped = true;
                    picture->setMovie("Zombies/DolphinRiderZombie/Jump.gif");
                    picture->start();
                    (new Timer(picture, 800, [this] {
                        ZX = attackedLX -= 80;
                        X -= 80;
                        attackedRX -= 80;
                        picture->setX(X);
                        picture->setMovie(zombieProtoType->normalGif);
                        picture->start();
                        this->normalGif = zombieProtoType->normalGif;
                        this->attackGif = zombieProtoType->attackGif;
                        // 原版：跃过后速度大降(约4-7s/格)
                        speed = 1.0;
                    }))->start();
                    return;
                }
            }
        }
    }
    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
        }
    }
}

// ===================== 冰车僵尸 =====================
ZomboniZombie::ZomboniZombie()
{
    eName = "oZomboni";
    cName = tr("冰车僵尸");
    hp = 1350;
    damagePoint1 = hp * 2 / 3;  // 900
    speed = 1.8;
    level = 4;
    sunNum = 175;
    beAttackedPointL = 120;
    beAttackedPointR = 160;
    breakPoint = 0;
    QString path = "Zombies/Zomboni/";
    cardGif = "Card/Zombies/Zomboni.png";
    staticGif = path + "0.gif";
    normalGif = path + "0.gif";
    attackGif = "Zombies/Zombie/ZombieAttack.gif";
    dieGif = "Zombies/Zombie/ZombieDie.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
    canSelect = true;
    aKind = 1; // 碾压类型攻击
}

ZomboniZombieInstance::ZomboniZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), iceTrailTimer(0)
{}

void ZomboniZombieInstance::leaveIceTrail()
{
    // 在当前位置留下冰道
    QGraphicsPixmapItem *ice = new QGraphicsPixmapItem(
        gImageCache->load("Zombies/Zomboni/ice.png"));
    ice->setPos(picture->x() + zombieProtoType->beAttackedPointL - 40,
                picture->y() + zombieProtoType->height - 20);
    ice->setZValue(picture->zValue() - 1);
    zombieProtoType->scene->addToGame(ice);
    // 原版：冰道持续30秒
    QGraphicsScene *scene = zombieProtoType->scene;
    QTimer::singleShot(30000, [scene, ice] {
        scene->removeItem(ice);
        delete ice;
    });
}

void ZomboniZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;
    if (!isAttacking) {
        attackedRX -= speed;
        ZX = attackedLX -= speed;
        X -= speed;
        picture->setX(X);
        if (attackedRX < -200) {
            zombieProtoType->scene->zombieDie(this);
            return;
        }
        // 每隔一段距离留下冰道（优化频率）
        iceTrailTimer++;
        if (iceTrailTimer > 15) {
            iceTrailTimer = 0;
            leaveIceTrail();
        }
        // 碾压前方植物
        int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
        if (col >= 1 && col <= 9) {
            auto plants = zombieProtoType->scene->getPlant(col, row);
            for (auto plant: plants.values()) {
                if (plant->plantProtoType->canEat
                    && plant->attackedRX >= ZX && plant->attackedLX <= ZX) {
                    // 直接碾压植物
                    plant->getHurt(this, 1, 1800);
                    break;
                }
            }
        }
    }
}

void ZomboniZombieInstance::applySlow(qreal multiplier, int durationMs)
{
    // 冰车僵尸不受减速影响
    Q_UNUSED(multiplier);
    Q_UNUSED(durationMs);
}

void ZomboniZombieInstance::boomDie()
{
    if (goingDie) return;
    goingDie = true;
    hp = 0;
    beAttacked = false;
    if (shadowPNG) shadowPNG->setPixmap(QPixmap());
    QString boomGif = zombieProtoType->boomDieGif.isEmpty() ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    picture->setMovie(boomGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZomboniZombieInstance::ashDie()
{
    boomDie();
}

Zombie *ZombieFactory(GameScene *scene, const QString &ename)
{
    Zombie *zombie = nullptr;
    if (ename == "oZombie")
        zombie = new Zombie1;
    if (ename == "oZombie2")
        zombie = new Zombie2;
    if (ename == "oZombie3")
        zombie = new Zombie3;
    if (ename == "oFlagZombie")
        zombie = new FlagZombie;
    if (ename == "oConeheadZombie")
        zombie = new ConeheadZombie;
    if (ename == "oBucketheadZombie")
        zombie = new BucketheadZombie;
    if (ename == "oPoleVaultingZombie")
        zombie = new PoleVaultingZombie;
    if (ename == "oNewspaperZombie")
        zombie = new NewspaperZombie;
    if (ename == "oFootballZombie")
        zombie = new FootballZombie;
    if (ename == "oScreenDoorZombie")
        zombie = new ScreenDoorZombie;
    if (ename == "oJackinTheBoxZombie")
        zombie = new JackinTheBoxZombie;
    if (ename == "oDancingZombie")
        zombie = new DancingZombie;
    if (ename == "oBackupDancer")
        zombie = new BackupDancer;
    if (ename == "oSnorkelZombie")
        zombie = new SnorkelZombie;
    if (ename == "oDolphinRiderZombie")
        zombie = new DolphinRiderZombie;
    if (ename == "oZomboni")
        zombie = new ZomboniZombie;
    if (zombie) {
        zombie->scene = scene;
        zombie->update();
    }
    return zombie;
}

ZombieInstance *ZombieInstanceFactory(const Zombie *zombie)
{
    if (zombie->eName == "oConeheadZombie")
        return new ConeheadZombieInstance(zombie);
    if (zombie->eName == "oBucketheadZombie")
        return new BucketheadZombieInstance(zombie);
    if (zombie->eName == "oPoleVaultingZombie")
        return new PoleVaultingZombieInstance(zombie);
    if (zombie->eName == "oNewspaperZombie")
        return new NewspaperZombieInstance(zombie);
    if (zombie->eName == "oFootballZombie")
        return new FootballZombieInstance(zombie);
    if (zombie->eName == "oScreenDoorZombie")
        return new ScreenDoorZombieInstance(zombie);
    if (zombie->eName == "oJackinTheBoxZombie")
        return new JackinTheBoxZombieInstance(zombie);
    if (zombie->eName == "oDancingZombie")
        return new DancingZombieInstance(zombie);
    if (zombie->eName == "oBackupDancer")
        return new BackupDancerInstance(zombie);
    if (zombie->eName == "oSnorkelZombie")
        return new SnorkelZombieInstance(zombie);
    if (zombie->eName == "oDolphinRiderZombie")
        return new DolphinRiderZombieInstance(zombie);
    if (zombie->eName == "oZomboni")
        return new ZomboniZombieInstance(zombie);
    return new ZombieInstance(zombie);
}


