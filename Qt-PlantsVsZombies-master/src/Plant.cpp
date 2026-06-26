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
        QUuid myUuid = uuid;
        QSharedPointer<std::function<void(QUuid)> > triggerCheck(new std::function<void(QUuid)>);
        *triggerCheck = [this, triggerCheck, myUuid] (QUuid zombieUuid) {
            (new Timer(picture, 1400, [this, zombieUuid, triggerCheck, myUuid] {
                // Guard: check if this plant still alive
                PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
                if (!self || self != this) { return; }
                ZombieInstance *zombie = plantProtoType->scene->getZombie(zombieUuid);
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

Peashooter::Peashooter()//豌豆射手
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

SnowPea::SnowPea()//寒冰射手
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
    // ensure snow pea has same firing cooldown as regular peashooter
    coolTime = 7.5;
}

SnowPeaInstance::SnowPeaInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void SnowPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    // play pea sound
    // use same sound player as Peashooter (firePea is private in base but sound is short-lived)
    QMediaPlayer *player = new QMediaPlayer(picture);
    player->setMedia(QUrl("qrc:/audio/firepea.mp3"));
    player->play();
    // produce a snow pea bullet (type 1) which will apply slow on hit
    (new Bullet(plantProtoType->scene, 1, row, attackedLX, attackedLX - 40, picture->y() + 3, picture->zValue() + 2, 0))->start();
}

// Repeater（双射豌豆射手）实现：发射两颗普通豌豆，间隔 150ms
Repeater::Repeater()//双发射手
{
    eName = "oRepeater";
    cName = tr("双发射手");
    beAttackedPointR = 51;
    sunNum = 200; // 花费
    cardGif = "Card/Plants/Repeater.png";
    // 使用动图作为静态预览（若有单帧 0.gif 可替换）
    staticGif = "Plants/Repeater/Repeater.gif";
    normalGif = "Plants/Repeater/Repeater.gif";
    // 中文 tooltip，包含简介与属性
    toolTip = tr("双发射手可以一次发射两颗豌豆<br/>伤害：中等（每颗）<br/>发射速度：两倍<br/><br/>\n双发射手很凶悍，他是在街头混大的。他不在乎任何人的看法，无论是植物还是僵尸，他打出豌豆，是为了让别人离他远点。其实呢，双发射手一直暗暗地渴望着爱情。");
    // 与普通豌豆相同冷却
    coolTime = 7.5;
}

RepeaterInstance::RepeaterInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void RepeaterInstance::normalAttack(ZombieInstance *zombieInstance)
{
    // 立即发射一颗（复用基类实现）
    PeashooterInstance::normalAttack(zombieInstance);
    // 在短延迟后再发射一颗
    (new Timer(picture, 150, [this, zombieInstance] {
        PeashooterInstance::normalAttack(zombieInstance);
    }))->start();
}

// GatlingPea（机枪射手）实现：连续发射四颗豌豆，每颗间隔 150ms
GatlingPea::GatlingPea()//机枪射手
{
    eName = "oGatlingPea";
    cName = tr("机枪射手");
    beAttackedPointR = 51;
    sunNum = 250; // 花费
    cardGif = "Card/Plants/GatlingPea.png";
    staticGif = "Plants/GatlingPea/0.gif";
    normalGif = "Plants/GatlingPea/GatlingPea.gif";
    toolTip = tr("机枪射手一次可以发射四颗豌豆<br/>伤害：重型（每颗）<br/>发射速度：四倍<br/><br/>\n机枪射手喜欢大声说话，尤其是用他的加特林机枪。他是植物大军里最受敬重的战士之一，虽然有时候他激动起来会把子弹打得到处都是。");
    coolTime = 7.5;
}

GatlingPeaInstance::GatlingPeaInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void GatlingPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    // 第一颗立即发射
    PeashooterInstance::normalAttack(zombieInstance);
    // 第二颗 150ms 后
    (new Timer(picture, 150, [this, zombieInstance] {
        PeashooterInstance::normalAttack(zombieInstance);
    }))->start();
    // 第三颗 300ms 后
    (new Timer(picture, 300, [this, zombieInstance] {
        PeashooterInstance::normalAttack(zombieInstance);
    }))->start();
    // 第四颗 450ms 后
    (new Timer(picture, 450, [this, zombieInstance] {
        PeashooterInstance::normalAttack(zombieInstance);
    }))->start();
}

// PuffShroom（小喷菇）实现：夜间植物，白天睡觉，短程攻击，免费
PuffShroom::PuffShroom()//小喷菇
{
    eName = "oPuffShroom";
    cName = tr("小喷菇");
    bKind = 0;
    beAttackedPointR = 45;
    sunNum = 0; // 免费
    night = true; // 夜行性植物
    cardGif = "Card/Plants/PuffShroom.png";
    staticGif = "Plants/PuffShroom/PuffShroom.gif";
    normalGif = "Plants/PuffShroom/PuffShroom.gif";
    toolTip = tr("小喷菇是夜间植物，白天会睡觉<br/>伤害：中等<br/>射程：短<br/>花费：0<br/><br/>\n小喷菇是新手的好朋友。它们虽然个子小，射程也短，但是它们完全免费，可以大批量种植。不过白天它们就顶不住打瞌睡了。");
    coolTime = 7.5;
}

PuffShroomInstance::PuffShroomInstance(const Plant *plant)
    : PeashooterInstance(plant),
      sleepGif("Plants/PuffShroom/PuffShroomSleep.gif")
{
}

bool PuffShroomInstance::isDaytime()
{
    return plantProtoType->scene->getGameLevelData()->dKind != 0;
}

void PuffShroomInstance::birth(int c, int r)
{
    PlantInstance::birth(c, r);
    // If daytime, show sleeping animation
    if (isDaytime()) {
        picture->setMovie(sleepGif);
        picture->start();
    }
}

void PuffShroomInstance::initTrigger()
{
    // Short range: only ~3 cells ahead
    Trigger *trigger = new Trigger(this, attackedLX, attackedLX + 250, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void PuffShroomInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    // Sleep during daytime — do nothing
    if (isDaytime()) {
        canTrigger = true;
        return;
    }
    // Nighttime: normal attack behavior
    PlantInstance::triggerCheck(zombieInstance, trigger);
}

// =====【小喷菇 泡泡高度调整点】===== 改 picture->y() + 后面的数字，越大越低 =====
void PuffShroomInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    (new Bullet(plantProtoType->scene, 2, row, attackedLX, attackedLX - 40,
                picture->y() + 30, picture->zValue() + 2, 0))->start();
}

// ScaredyShroom（胆小菇）实现：夜间植物，白天睡觉，全屏攻击，僵尸靠近时哭泣停止攻击
ScaredyShroom::ScaredyShroom()//胆小菇
{
    eName = "oScaredyShroom";
    cName = tr("胆小菇");
    bKind = 0;
    beAttackedPointR = 45;
    sunNum = 25; // 花费
    night = true; // 夜行性植物
    cardGif = "Card/Plants/ScaredyShroom.png";
    staticGif = "Plants/ScaredyShroom/ScaredyShroom.gif";
    normalGif = "Plants/ScaredyShroom/ScaredyShroom.gif";
    toolTip = tr("胆小菇是夜间植物，白天会睡觉<br/>伤害：中等<br/>射程：全屏<br/>花费：25<br/>特性：僵尸靠近时会停止攻击<br/><br/>\n胆小菇胆子很小，一有僵尸靠近就吓得缩成一团不敢攻击。不过它的视力很好，远处的僵尸能被它看得一清二楚。");
    coolTime = 7.5;
}

ScaredyShroomInstance::ScaredyShroomInstance(const Plant *plant)
    : PuffShroomInstance(plant),
      cryGif("Plants/ScaredyShroom/ScaredyShroomCry.gif"),
      m_scared(false)
{
}

bool ScaredyShroomInstance::isScared() const { return m_scared; }

void ScaredyShroomInstance::enterScared()
{
    if (m_scared) return;
    m_scared = true;
    picture->setMovie(cryGif);
    picture->start();
}

void ScaredyShroomInstance::exitScared()
{
    if (!m_scared) return;
    m_scared = false;
    picture->setMovie(plantProtoType->normalGif);
    picture->start();
}

void ScaredyShroomInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) enterScared();
}

void ScaredyShroomInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, 880, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void ScaredyShroomInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (isDaytime()) { canTrigger = true; return; }
    if (m_scared) {
        bool stillAttacked = false;
        QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp <= 0 || z->goingDie) continue;
            if (z->isAttacking && qAbs(z->attackedLX - attackedLX) < 80) {
                stillAttacked = true;
                break;
            }
        }
        if (!stillAttacked) exitScared();
        canTrigger = true;
        return;
    }
    PlantInstance::triggerCheck(zombieInstance, trigger);
}

// =====【胆小菇 泡泡高度调整点】===== 改 picture->y() + 后面的数字，越大越低
void ScaredyShroomInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    (new Bullet(plantProtoType->scene, 2, row, attackedLX, attackedLX - 40,
                picture->y() + 27, picture->zValue() + 2, 0))->start();
}

// FumeShroom（大喷菇）实现：夜间植物，4格范围毒气攻击，攻击动画与子弹同步
FumeShroom::FumeShroom()//大喷菇
{
    eName = "oFumeShroom";
    cName = tr("大喷菇");
    bKind = 0;
    beAttackedPointR = 45;
    sunNum = 75;
    night = true;
    cardGif = "Card/Plants/FumeShroom.png";
    staticGif = "Plants/FumeShroom/FumeShroom.gif";
    normalGif = "Plants/FumeShroom/FumeShroom.gif";
    toolTip = tr("大喷菇是夜间植物，白天会睡觉<br/>伤害：中等<br/>射程：4格<br/>花费：75<br/>特性：喷射毒气穿透一行<br/><br/>\n大喷菇喷出的毒气可以穿透一整行僵尸，伤害范围内的所有敌人。它讨厌阳光，白天就蔫了。");
    coolTime = 7.5;
}

FumeShroomInstance::FumeShroomInstance(const Plant *plant)
    : PuffShroomInstance(plant),
      attackGif("Plants/FumeShroom/FumeShroomAttack.gif"),
      bulletGif("Plants/FumeShroom/FumeShroomBullet.gif")
{
}

void FumeShroomInstance::birth(int c, int r)
{
    PuffShroomInstance::birth(c, r);
}

void FumeShroomInstance::initTrigger()
{
    // 4-tile range = 320px
    Trigger *trigger = new Trigger(this, attackedLX, attackedLX + 320, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void FumeShroomInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (isDaytime()) { canTrigger = true; return; }
    PlantInstance::triggerCheck(zombieInstance, trigger);
}

void FumeShroomInstance::normalAttack(ZombieInstance *zombieInstance)
{
    firePea->play();
    // Switch to attack animation, fire bullet synced with animation
    picture->setMovieOnNewLoop(attackGif, [this] {
        // =====【大喷菇 毒气高度调整点】===== picture->y() + 后面的数字
        (new Bullet(plantProtoType->scene, 3, row, attackedLX,
                    attackedLX - 40,
                    picture->y() + 10, picture->zValue() + 2, 0))->start();
        // After attack animation plays once, switch back to normal idle GIF
        (new Timer(picture, 1000, [this] {
            picture->setMovieOnNewLoop(plantProtoType->normalGif);
            picture->start();
        }))->start();
    });
    picture->start();
}

SunFlower::SunFlower()//向日葵
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
//向日葵产生阳光数值调整
void SunFlowerInstance::initTrigger()
{
    QUuid myUuid = uuid;
    (new Timer(picture, 5000, [this, myUuid] {
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;
        QSharedPointer<std::function<void(void)> > generateSun(new std::function<void(void)>);
        *generateSun = [this, generateSun, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            picture->setMovieOnNewLoop(lightedGif, [this, generateSun, myUuid] {
                PlantInstance *self3 = plantProtoType->scene->getPlant(myUuid);
                if (!self3 || self3 != this) return;
                (new Timer(picture, 1000, [this, generateSun, myUuid] {
                    PlantInstance *self4 = plantProtoType->scene->getPlant(myUuid);
                    if (!self4 || self4 != this) return;
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(50);
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
                    picture->setMovieOnNewLoop(plantProtoType->normalGif, [this, generateSun, myUuid] {
                        PlantInstance *self5 = plantProtoType->scene->getPlant(myUuid);
                        if (!self5 || self5 != this) return;
                        (new Timer(picture, 24000, [this, generateSun, myUuid] {
                            PlantInstance *self6 = plantProtoType->scene->getPlant(myUuid);
                            if (!self6 || self6 != this) return;
                            (*generateSun)();
                        }))->start();
                    });
                }))->start();
            });
        };
        (*generateSun)();
    }))->start();
}

WallNut::WallNut()//坚果墙
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

LawnCleaner::LawnCleaner()//小车
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

PoolCleaner::PoolCleaner()//池塘清扫车
{
    eName = "oPoolCleaner";
    cName = tr("Pool Cleaner");
    beAttackedPointR = 47;
    staticGif = normalGif = "interface/PoolCleaner.png";
    toolTip = tr("Pool Cleaner");
    update();
}

CherryBomb::CherryBomb()//樱桃炸弹
{
    eName = "oCherryBomb";
    cName = tr("樱桃炸弹");
    beAttackedPointR = 30;
    sunNum = 150;
    coolTime = 20; // 冷却时间
    cardGif = "Card/Plants/CherryBomb.png";
    staticGif = "Plants/CherryBomb/0.gif";
    normalGif = "Plants/CherryBomb/CherryBomb.gif";
    toolTip = tr("Explodes and kills surrounding zombies");
}

CherryBombInstance::CherryBombInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

void CherryBombInstance::initTrigger()
{
    // Cherry Bomb auto-explodes on its own cell:
    //   idle(500ms) → inflate(600ms) → Boom.gif+sound(1500ms) → zombies→ash → crater → die
    QUuid myUuid = uuid;
    (new Timer(picture, 500, [this, myUuid] {
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;
        // Phase 1: Inflate — CherryBomb swells up
        Animate(picture).scale(1.3).duration(600).shape(QTimeLine::EaseOutCurve).finish([this, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            // Phase 2: Boom!
            QMediaPlayer *player = new QMediaPlayer(picture);
            player->setMedia(QUrl("qrc:/audio/cherrybomb.mp3"));
            player->play();

            picture->setScale(1.0);
            picture->setMovie("Plants/CherryBomb/Boom.gif");
            picture->start();

            // Phase 3: Kill zombies after Boom animation plays (use Timer, not QMovie::finished)
            (new Timer(picture, 1500, [this, myUuid] {
                PlantInstance *self3 = plantProtoType->scene->getPlant(myUuid);
                if (!self3 || self3 != this) return;
                // 3×3 area centered on the bomb's cell
                qreal tileW = 80.0;
                qreal cx = (attackedLX + attackedRX) / 2.0;
                for (int r = row - 1; r <= row + 1; ++r) {
                    if (r < 1 || r > plantProtoType->scene->getCoordinate().rowCount())
                        continue;
                    QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRowRange(
                        r, cx - tileW * 1.5, cx + tileW * 1.5);
                    for (auto z: zombies) {
                        if (z->hp > 0 && !z->goingDie)
                            z->ashDie();
                    }
                }
                plantProtoType->scene->addCrater(col, row);
                plantProtoType->scene->plantDie(this);
            }))->start();
        });
    }))->start();
}

void CherryBombInstance::triggerCheck(ZombieInstance *, Trigger *)
{
    // Cherry Bomb auto-explodes — no trigger-based attack needed
}

PotatoMine::PotatoMine()//土豆雷
{
    eName = "oPotatoMine";
    cName = tr("土豆雷");
    beAttackedPointR = 47;
    sunNum = 25;
    coolTime = 30;
    canEat = false;
    cardGif = "Card/Plants/PotatoMine.png";
    staticGif = "Plants/PotatoMine/0.gif";
    normalGif = "Plants/PotatoMine/PotatoMine.gif";
    toolTip = tr("土豆雷需要时间准备，准备完成后僵尸踩到就会爆炸<br/>伤害：极高<br/>花费：25<br/>冷却：慢<br/><br/>\n土豆雷很内向，需要在地下酝酿一段时间才能准备好。一旦准备好了，就是僵尸的噩梦。");
}

PotatoMineInstance::PotatoMineInstance(const Plant *plant)
    : PlantInstance(plant),
      notReadyGif("Plants/PotatoMine/PotatoMineNotReady.gif"),
      mashGif("Plants/PotatoMine/PotatoMine_mashed.gif"),
      explosionGif("Plants/PotatoMine/ExplosionSpudow.gif"),
      isArmed(false), exploded(false)
{
    picture->setMovie(notReadyGif);
}

void PotatoMineInstance::birth(int c, int r)
{
    // 先调用基类的 birth 来设置位置、阴影等
    PlantInstance::birth(c, r);
    // 基类会将图片设置为 normalGif（已准备好的形态），
    // 这里立即切换回"未准备"的动画
    picture->setMovie(notReadyGif);
    picture->start();
}

void PotatoMineInstance::initTrigger()
{
    // 15秒后准备就绪
    QUuid myUuid = uuid;
    (new Timer(picture, 15000, [this, myUuid] {
        if (exploded) return;
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;
        isArmed = true;
        // 播放发芽动画
        MoviePixmapItem *growEffect = new MoviePixmapItem("interface/GrowSoil.gif");
        growEffect->setPos(picture->x() - 20, picture->y() - 20);
        growEffect->setZValue(picture->zValue() + 1);
        plantProtoType->scene->addToGame(growEffect);
        growEffect->start();
        (new Timer(picture, 600, [this, growEffect, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            growEffect->deleteLater();
            picture->setMovie(plantProtoType->normalGif);
            picture->start();
        }))->start();
    }))->start();
    // 扩大触发范围到整个格子宽度（约80px），确保僵尸走进格子就能触发
    qreal cellCenterX = plantProtoType->scene->getCoordinate().getX(col);
    Trigger *trigger = new Trigger(this, cellCenterX - 40, cellCenterX + 40, 0, 0);
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
    picture->setMovie(mashGif);
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 400, [this, myUuid] {
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;
        picture->setMovie(explosionGif);
        picture->setScale(1.5);
        QPointF center = picture->pos();
        picture->setPos(center.x() - 30, center.y() - 30);
        picture->start();
        (new Timer(picture, 600, [this, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            GameScene *scene = plantProtoType->scene;
            QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
            for (auto zombie : zombies) {
                if (zombie->hp > 0 && !zombie->goingDie) {
                    int zCol = scene->getCoordinate().getCol(zombie->ZX);
                    if (zCol >= col - 1 && zCol <= col + 1)
                        zombie->ashDie();
                }
            }
            scene->plantDie(this);
        }))->start();
    }))->start();
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

Bullet::Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue, int direction)
        : scene(scene), type(type), row(row), direction(direction), from(from)//子弹代码type（0豌豆 1冰豆 2孢子 3毒气）
{
    count = 0;
    QString picName;
    if (type == 3)
        picName = QString("Plants/FumeShroom/FumeShroomBullet.gif");
    else if (type == 2)//第二种攻击方式喷射泡泡
        picName = QString("Plants/ShroomBullet.gif");
    else if (type == 1)//第一种攻击方，喷射豌豆
        picName = QString("Plants/PB-%1%2.gif").arg(type).arg(direction);
    else
        picName = QString("Plants/PB%1%2.gif").arg(type).arg(direction);
    picture = new QGraphicsPixmapItem(gImageCache->load(picName));
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
    if (type < 1 && plants.contains(1) && plants[1]->plantProtoType->eName == "oTorchwood") {
        ++type;
        QString picName;
        if (type == 1)
            picName = QString("Plants/PB-%1%2.gif").arg(type).arg(direction);
        else
            picName = QString("Plants/PB%1%2.gif").arg(type).arg(direction);
        picture->setPixmap(gImageCache->load(picName));
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
    // TODO: 子弹打击到僵尸时的效果，（泡泡，豌豆，寒冰射手的子弹）
    if (type == 3) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp > 0 && !z->goingDie && z->attackedLX >= from && z->attackedLX <= from + 320)
                z->getPea(20, direction);
        }
        (new Timer(scene, 500, [this] { delete this; }))->start();
        return;
    }
    if (zombie && zombie->altitude >= 0 && !zombie->goingDie) {
        // TODO: other attacks
        zombie->getPea(20, direction);
        // if this is a snow pea (type == 1), apply slow effect for a short duration
        if (type == 1) {
            // 应用寒冰射手减速：速度乘以 0.5，持续 5000 毫秒
            zombie->applySlow(0.5, 5000);//寒冰射手减速
        }
        picture->setPos(picture->pos() + QPointF(28, 0));
        if (type == 2)
            picture->setPixmap(gImageCache->load("Plants/ShroomBulletHit.gif"));
        else
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

Plant *PlantFactory(GameScene *scene, const QString &eName)
{
    Plant *plant = nullptr;
    if (eName == "oPeashooter")//豌豆射手
        plant = new Peashooter;
    else if (eName == "oSnowPea")//寒冰射手
        plant = new SnowPea;
    else if (eName == "oRepeater")//双发射手
        plant = new Repeater;
    else if (eName == "oGatlingPea")//机枪射手
        plant = new GatlingPea;
    else if (eName == "oPuffShroom")//小喷菇
        plant = new PuffShroom;
    else if (eName == "oScaredyShroom")//胆小菇
        plant = new ScaredyShroom;
    else if (eName == "oFumeShroom")//大喷菇
        plant = new FumeShroom;
    else if (eName == "oSunflower")//向日葵
        plant = new SunFlower;
    else if (eName == "oWallNut")//坚果墙
        plant = new WallNut;
    else if (eName == "oLawnCleaner")//小车
        plant = new LawnCleaner;
    else if (eName == "oPoolCleaner")//池塘清扫车
        plant = new PoolCleaner;
    else if (eName == "oCherryBomb")//樱桃炸弹
        plant = new CherryBomb;
    else if (eName == "oPotatoMine")//土豆雷
        plant = new PotatoMine;
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
    if (plant->eName == "oRepeater")
        return new RepeaterInstance(plant);
    if (plant->eName == "oGatlingPea")
        return new GatlingPeaInstance(plant);
    if (plant->eName == "oPuffShroom")
        return new PuffShroomInstance(plant);
    if (plant->eName == "oScaredyShroom")
        return new ScaredyShroomInstance(plant);
    if (plant->eName == "oFumeShroom")
        return new FumeShroomInstance(plant);
    if (plant->eName == "oSnowPea")
        return new SnowPeaInstance(plant);
    if (plant->eName == "oSunflower")
        return new SunFlowerInstance(plant);
    if (plant->eName == "oCherryBomb")
        return new CherryBombInstance(plant);
    if (plant->eName == "oPotatoMine")
        return new PotatoMineInstance(plant);
    if (plant->eName == "oWallNut")
        return new WallNutInstance(plant);
    if (plant->eName == "oLawnCleaner")
        return new LawnCleanerInstance(plant);
    return new PlantInstance(plant);
}



