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
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
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
    m_awake = false;
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

void PlantInstance::wakeUp()
{
    m_awake = true;
    picture->setMovie(plantProtoType->normalGif);
    picture->start();
}

void PlantInstance::onDayNightChanged(bool isNight)
{
    Q_UNUSED(isNight);
    // 基类默认无操作，由蘑菇等夜间植物子类覆盖
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
    if (m_awake) return false; // 被咖啡豆唤醒后不再休眠
    return !plantProtoType->scene->nightMode();
}

void PuffShroomInstance::birth(int c, int r)
{
    PlantInstance::birth(c, r);

    //光敏传感器接口
    // If daytime, show sleeping animation
    if (isDaytime()) {
        picture->setMovie(sleepGif);
        picture->start();
    }
}

void PuffShroomInstance::onDayNightChanged(bool isNight)
{
    Q_UNUSED(isNight);
    if (m_awake) return;  // 被咖啡豆唤醒后不受昼夜影响

    //光敏传感器接口，切换为白天模式时，小喷菇进入睡眠状态，切换为黑夜模式时，小喷菇苏醒
    if (isDaytime()) {
        // 切换为白天 → 休眠
        picture->setMovie(sleepGif);
        picture->start();
    } else {
        // 切换为黑夜 → 苏醒
        picture->setMovie(plantProtoType->normalGif);
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

// ==================== 海蘑菇（SeaShroom） ====================
SeaShroom::SeaShroom()
{
    eName = "oSeaShroom";
    cName = tr("海蘑菇");
    hp = 300;
    sunNum = 0;          // 免费
    coolTime = 30.0;     // 冷却30秒
    night = true;        // 夜间植物
    beAttackedPointR = 45;
    cardGif = "Card/Plants/SeaShroom.png";
    staticGif = "Plants/SeaShroom/SeaShroom.gif";
    normalGif = "Plants/SeaShroom/SeaShroom.gif";
    toolTip = tr("Sea-shroom is an aquatic plant that shoots spores<br/>Damage: normal<br/>Range: short<br/>Cost: 0<br/>Special: only plantable on water");
}

bool SeaShroom::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    // 仅水域，直接种在水里不需要荷叶
    int groundType = scene->getGameLevelData()->LF[y];
    if (groundType != 2) return false;
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    return !plants.contains(1);
}

SeaShroomInstance::SeaShroomInstance(const Plant *plant)
    : PuffShroomInstance(plant)
{
    sleepGif = "Plants/SeaShroom/SeaShroomSleep.gif";
}

// ==================== 缠绕水草（TangleKelp） ====================
TangleKelp::TangleKelp()
{
    eName = "oTangleKlep";
    cName = tr("缠绕水草");
    hp = 300;
    sunNum = 25;
    coolTime = 30.0;
    pKind = 1;
    canEat = false;          // 一次性触发，僵尸不啃咬
    beAttackedPointR = 40;
    cardGif = "Card/Plants/TangleKlep.png";
    staticGif = "Plants/TangleKlep/0.gif";
    normalGif = "Plants/TangleKlep/Float.gif";
    toolTip = tr("Tangle Kelp grabs one water zombie and pulls it underwater");
}

bool TangleKelp::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    if (groundType != 2) return false; // 仅水域
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    return !plants.contains(1); // 不需要荷叶
}

TangleKelpInstance::TangleKelpInstance(const Plant *plant)
    : PlantInstance(plant), m_triggered(false),
      floatGif("Plants/TangleKlep/Float.gif"),
      attackGif("Plants/TangleKlep/TangleKlep.gif"),
      grabPng("Plants/TangleKlep/Grab.png"),
      splashPng("Plants/TangleKlep/splash.png")
{}

void TangleKelpInstance::initTrigger()
{
    // 仅在僵尸贴近自身格子时触发
    Trigger *trigger = new Trigger(this, attackedLX - 20, attackedRX + 10, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void TangleKelpInstance::triggerCheck(ZombieInstance *zombie, Trigger *trigger)
{
    Q_UNUSED(trigger);
    if (m_triggered) return;
    if (zombie->hp <= 0 || zombie->goingDie) return;

    // 仅对水域可下水僵尸生效（canPass 在水域行返回 true）
    if (!zombie->zombieProtoType->canPass(row)) return;

    m_triggered = true;
    canTrigger = false;

    // 立即标记僵尸为濒死状态，防止在拉入水动画期间继续移动/攻击
    zombie->goingDie = true;
    zombie->hp = 0;

    // 播放抓取动画
    picture->setMovie(attackGif);
    picture->start();

    // 显示抓取特效
    QGraphicsPixmapItem *grab = new QGraphicsPixmapItem(gImageCache->load(grabPng));
    grab->setPos(zombie->X - 20, picture->y() - 40);
    grab->setZValue(picture->zValue() + 2);
    plantProtoType->scene->addToGame(grab);

    QUuid zombieUuid = zombie->uuid;
    QUuid myUuid = uuid;

    // 延迟 600ms 后水花特效 + 正式移除僵尸 + 自身销毁
    (new Timer(picture, 600, [this, grab, zombieUuid, myUuid] {
        // 水花特效
        QGraphicsPixmapItem *splash = new QGraphicsPixmapItem(gImageCache->load(splashPng));
        splash->setPos(picture->x() + 20, picture->y() - 30);
        splash->setZValue(picture->zValue() + 3);
        plantProtoType->scene->addToGame(splash);
        (new Timer(plantProtoType->scene, 800, [splash] {
            if (splash->scene()) splash->scene()->removeItem(splash);
            delete splash;
        }))->start();

        // 移除抓取特效
        if (grab->scene()) grab->scene()->removeItem(grab);
        delete grab;

        // 正式移除僵尸（若未被其他途径清理）
        ZombieInstance *z = plantProtoType->scene->getZombie(zombieUuid);
        if (z) plantProtoType->scene->zombieDie(z);

        // 自身销毁（若未被其他途径清理）
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (self && self == this) plantProtoType->scene->plantDie(this);
    }))->start();
}

// ==================== 地刺（Spikeweed） ====================
Spikeweed::Spikeweed()
{
    eName = "oSpikeweed";
    cName = tr("地刺");
    hp = 300;
    sunNum = 100;
    coolTime = 7.5;
    pKind = 1;
    canEat = false;          // 僵尸不啃食，直接踩过受伤
    beAttackedPointL = 20;
    beAttackedPointR = 60;
    cardGif = "Card/Plants/Spikeweed.png";
    staticGif = "Plants/Spikeweed/Spikeweed.gif";
    normalGif = "Plants/Spikeweed/Spikeweed.gif";
    toolTip = tr("Spikeweed damages zombies that walk over it");
}

SpikeweedInstance::SpikeweedInstance(const Plant *plant)
    : PlantInstance(plant)
{
    canTrigger = true;  // 始终允许触发，由内部冷却控制频率
}

void SpikeweedInstance::initTrigger()
{
    // 宽触发区：覆盖自身格子，确保其他植物触发器不会挡住地刺的判定
    Trigger *trigger = new Trigger(this, attackedLX - 50, attackedRX + 80, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void SpikeweedInstance::triggerCheck(ZombieInstance *zombie, Trigger *trigger)
{
    Q_UNUSED(trigger);
    if (zombie->hp <= 0 || zombie->goingDie) return;
    // 飞行/跳跃中的僵尸不受地刺伤害
    if (zombie->altitude > 0) return;
    // 冷却中则跳过
    if (!canTrigger) return;

    // 造成伤害并进入冷却
    canTrigger = false;
    zombie->getHit(20);
    (new Timer(picture, 500, [this] { canTrigger = true; }))->start();
}

// ==================== 地刺王（Spikerock） ====================
Spikerock::Spikerock()
{
    eName = "oSpikerock";
    cName = tr("地刺王");
    hp = 3;                  // 3 点耐久 = 3 根刺
    sunNum = 125;
    coolTime = 7.5;
    pKind = 1;
    canEat = false;
    beAttackedPointL = 20;
    beAttackedPointR = 60;
    cardGif = "Card/Plants/Spikerock.png";
    staticGif = "Plants/Spikerock/Spikerock.gif";
    normalGif = "Plants/Spikerock/Spikerock.gif";
    toolTip = tr("Spikerock destroys zombies and takes damage with each attack");
}

bool Spikerock::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    if (groundType != 1) return false; // 仅陆地
    // 必须种在已有地刺上方
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    return plants.contains(1)
        && plants[1]->plantProtoType->eName == "oSpikeweed";
}

SpikerockInstance::SpikerockInstance(const Plant *plant)
    : SpikeweedInstance(plant)
{
    canTrigger = true;
}

void SpikerockInstance::birth(int c, int r)
{
    // 先铲除原地刺，再种下地刺王
    QMap<int, PlantInstance *> existing = plantProtoType->scene->getPlant(c, r);
    if (existing.contains(1)
        && existing[1]->plantProtoType->eName == "oSpikeweed") {
        plantProtoType->scene->plantDie(existing[1]);
    }
    SpikeweedInstance::birth(c, r);
}

void SpikerockInstance::updateSprite()
{
    if (hp >= 3)
        picture->setMovie("Plants/Spikerock/Spikerock.gif");
    else if (hp >= 2)
        picture->setMovie("Plants/Spikerock/2.gif");
    else if (hp >= 1)
        picture->setMovie("Plants/Spikerock/0.gif");
    picture->start();
}

void SpikerockInstance::triggerCheck(ZombieInstance *zombie, Trigger *trigger)
{
    Q_UNUSED(trigger);
    if (hp <= 0) return;  // 自身已耗尽
    if (zombie->hp <= 0 || zombie->goingDie) return;
    if (zombie->altitude > 0) return;
    if (!canTrigger) return;

    canTrigger = false;
    zombie->getHit(40);     // 双倍于普通地刺的伤害
    hp--;                   // 每攻击一次消耗 1 点耐久
    updateSprite();
    if (hp <= 0) {
        plantProtoType->scene->plantDie(this);
        return;
    }
    (new Timer(picture, 500, [this] { canTrigger = true; }))->start();
}

void SpikerockInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) updateSprite();  // 受击后刷新尖刺视觉
}

// ==================== 南瓜头（PumpkinHead） ====================
PumpkinHead::PumpkinHead()
{
    eName = "oPumpkinHead";
    cName = tr("南瓜头");
    hp = 4000;
    pKind = 2;              // 护甲层，独立于内部植物
    zIndex = 1;             // 渲染在内部植物上方，包裹效果
    sunNum = 0;
    coolTime = 0;
    canEat = true;           // 僵尸优先啃食
    beAttackedPointL = 20;
    beAttackedPointR = 70;
    cardGif = "Card/Plants/PumpkinHead.png";
    staticGif = "Plants/PumpkinHead/PumpkinHead.gif";
    normalGif = "Plants/PumpkinHead/PumpkinHead.gif";
    toolTip = tr("Pumpkin protects plants inside it from zombie attacks");
}

bool PumpkinHead::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    // 必须有内部植物且未套南瓜
    if (!plants.contains(1) || plants.contains(2)) return false;
    // 禁止套壳清单
    QString innerName = plants[1]->plantProtoType->eName;
    static const QStringList noWrap = {
        "oPumpkinHead", "oSpikeweed", "oSpikerock",
        "oCherryBomb", "oJalapeno", "oDoomShroom", "oSquash",
        "oLilyPad", "oTangleKlep", "oCoffeeBean"
    };
    if (noWrap.contains(innerName)) return false;
    // 兼容地面类型
    int groundType = scene->getGameLevelData()->LF[y];
    if (groundType == 1) return true;
    if (groundType == 2 && plants.contains(0)) return true; // 水路有莲叶
    return false;
}

PumpkinHeadInstance::PumpkinHeadInstance(const Plant *plant)
    : PlantInstance(plant)
{}

void PumpkinHeadInstance::updateDamageSprite()
{
    int maxHp = plantProtoType->hp;  // 4000
    if (hp > maxHp * 2 / 3)
        picture->setMovie("Plants/PumpkinHead/PumpkinHead.gif");
    else if (hp > maxHp / 3)
        picture->setMovie("Plants/PumpkinHead/PumpkinHead1.gif");
    else
        picture->setMovie("Plants/PumpkinHead/PumpkinHead2.gif");
    picture->start();
}

void PumpkinHeadInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) updateDamageSprite();
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
    sleepGif = "Plants/ScaredyShroom/ScaredyShroomSleep.gif";
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
      bulletGif("Plants/FumeShroom/FumeShroomBullet.gif")//子弹有问题
{
    sleepGif = "Plants/FumeShroom/FumeShroomSleep.gif";
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

// Torchwood（火炬树桩）：将穿过它的豌豆变成火豌豆（PB-10.gif），伤害翻倍
Torchwood::Torchwood()//火炬树桩
{
    eName = "oTorchwood";
    cName = tr("火炬树桩");
    hp = 300;
    beAttackedPointR = 50;
    sunNum = 175;
    cardGif = "Card/Plants/Torchwood.png";
    staticGif = "Plants/Torchwood/Torchwood.gif";
    normalGif = "Plants/Torchwood/Torchwood.gif";
    toolTip = tr("火炬树桩可以将穿过它的豌豆变成火豌豆<br/>火豌豆伤害翻倍<br/><br/>\n火炬树桩会点燃经过它的豌豆，把它们变成能造成双倍伤害的火球。不过火炬树桩本身不会攻击，它只是一个被动的增益植物。");
    coolTime = 7.5;
}

TorchwoodInstance::TorchwoodInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

void TorchwoodInstance::initTrigger()
{
    // Torchwood doesn't attack — no trigger
}

// SplitPea（双向射手）：向右射速正常，向左射速2倍
SplitPea::SplitPea()//双向射手
{
    eName = "oSplitPea";
    cName = tr("双向射手");
    beAttackedPointR = 51;
    sunNum = 125;
    cardGif = "Card/Plants/SplitPea.png";
    staticGif = "Plants/SplitPea/SplitPea.gif";
    normalGif = "Plants/SplitPea/SplitPea.gif";
    toolTip = tr("双向射手向前后两个方向发射豌豆<br/>向右射速：正常<br/>向左射速：2倍<br/><br/>\n双向射手是双发射手的远房表亲。别人都只有一个方向，它偏要两个方向都来一枪。");
    coolTime = 7.5;
}

SplitPeaInstance::SplitPeaInstance(const Plant *plant)
    : PeashooterInstance(plant),
      m_canTriggerRight(true), m_canTriggerLeft(true)
{
}

void SplitPeaInstance::initTrigger()
{
    // Right trigger: attackedLX → 880 (normal peashooter range), direction=0
    Trigger *rightT = new Trigger(this, attackedLX, 880, 0, 0);
    triggers[row].append(rightT);
    plantProtoType->scene->addTrigger(row, rightT);
    // Left trigger: 100 → attackedLX (behind the plant, zombies that passed), direction=1
    Trigger *leftT = new Trigger(this, 100, attackedLX, 1, 1);
    triggers[row].append(leftT);
    plantProtoType->scene->addTrigger(row, leftT);
}

void SplitPeaInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (zombieInstance->altitude <= 0) return;
    bool isRight = (trigger->direction == 0);
    bool &canDir = isRight ? m_canTriggerRight : m_canTriggerLeft;
    if (!canDir) return;
    canDir = false;

    QUuid myUuid = uuid;
    int dir = trigger->direction;
    QSharedPointer<std::function<void(QUuid)> > triggerChain(new std::function<void(QUuid)>);
    *triggerChain = [this, triggerChain, myUuid, dir] (QUuid zombieUuid) {
        (new Timer(picture, 1400, [this, zombieUuid, triggerChain, myUuid, dir] {
            PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
            if (!self || self != this) return;
            ZombieInstance *zombie = plantProtoType->scene->getZombie(zombieUuid);
            bool found = false;
            if (zombie && zombie->hp > 0 && zombie->altitude > 0) {
                for (auto i : triggers[zombie->row]) {
                    if (i->direction == dir && zombie->ZX >= i->from && zombie->ZX <= i->to) {
                        firePea->play();
                        if (dir == 0) fireRight(); else fireLeft();
                        (*triggerChain)(zombie->uuid);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                bool &canDir2 = (dir == 0) ? m_canTriggerRight : m_canTriggerLeft;
                canDir2 = true;
            }
        }))->start();
    };
    firePea->play();
    if (isRight) fireRight(); else fireLeft();
    (*triggerChain)(zombieInstance->uuid);
}

void SplitPeaInstance::fireRight()
{
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX - 40,
                picture->y() + 5, picture->zValue() + 2, 0, 5.0))->start();
}

void SplitPeaInstance::fireLeft()
{
    (new Bullet(plantProtoType->scene, 0, row, attackedLX, attackedLX + 10,
                picture->y() + 5, picture->zValue() + 2, 1, 10.0))->start();
}

void SplitPeaInstance::normalAttack(ZombieInstance *zombieInstance)
{
    // Not used directly — triggerCheck handles per-direction firing
}

// SunShroom（阳光菇）：两阶段成长，白天睡觉，幼年产15阳光，成年产25阳光
SunShroom::SunShroom()//阳光菇
{
    eName = "oSunShroom";
    cName = tr("阳光菇");
    beAttackedPointR = 45;
    sunNum = 25;
    night = true;
    cardGif = "Card/Plants/SunShroom.png";
    staticGif = "Plants/SunShroom/SunShroom2.gif";
    normalGif = "Plants/SunShroom/SunShroom2.gif";
    toolTip = tr("阳光菇是夜间植物，白天会睡觉<br/>花费：25<br/>幼年：每次产15阳光<br/>成年（120秒后）：每次产25阳光<br/>产阳光间隔：约24秒<br/><br/>\n阳光菇小时候很小气，一次只给15阳光。等它长成大人了才会变得和向日葵一样大方。不过白天它就呼呼大睡了。");
    coolTime = 7.5;
}

SunShroomInstance::SunShroomInstance(const Plant *plant)
    : PlantInstance(plant),
      babyGif("Plants/SunShroom/SunShroom2.gif"),
      adultGif("Plants/SunShroom/SunShroom.gif"),
      sleepGif("Plants/SunShroom/SunShroomSleep.gif"),
      grownAge(0), isAdult(false)
{
}

bool SunShroomInstance::isDaytime()
{
    if (m_awake) return false; // 被咖啡豆唤醒后恢复产阳光
    return !plantProtoType->scene->nightMode();
}
//阳光菇成长逻辑：幼年阶段 120s 后变为成年阶段，成长时间的设置
void SunShroomInstance::tryGrow()
{
    if (!isAdult && grownAge >= 120000) { // 120s = adult
        isAdult = true;
        // 成长动画切换不受昼夜影响（计时器在 birth() 中独立运行）
        if (!isDaytime() || m_awake)
            picture->setMovie(adultGif);
        else
            picture->setMovie(sleepGif); // 白天休眠用 sleepGif
        picture->start();
    }
}

void SunShroomInstance::wakeUp()
{
    PlantInstance::wakeUp();
    picture->setMovie(isAdult ? adultGif : babyGif);
    picture->start();
}

void SunShroomInstance::onDayNightChanged(bool isNight)
{
    Q_UNUSED(isNight);
    if (m_awake) return;
    if (isDaytime()) {
        picture->setMovie(sleepGif);
        picture->start();
    } else {
        picture->setMovie(isAdult ? adultGif : babyGif);
        picture->start();
    }
}

void SunShroomInstance::birth(int c, int r)
{
    PlantInstance::birth(c, r);
    if (isDaytime()) {
        picture->setMovie(sleepGif);
        picture->start();
    }

    // ---- 循环成长计时器（使用 QSharedPointer） ----
    QSharedPointer<std::function<void()>> growLoop(new std::function<void()>);
    *growLoop = [this, growLoop] {
        if (isAdult) return;
        grownAge += 1000;
        tryGrow();
        if (!isAdult) {
            (new Timer(picture, 1000, *growLoop))->start();
        }
    };
    (new Timer(picture, 1000, *growLoop))->start();
}
void SunShroomInstance::initTrigger()
{
    // Wait 24s between sun productions (like SunFlower's 24s loop)
    // Start producing after initial 5s delay
    QUuid myUuid = uuid;
    (new Timer(picture, 5000, [this, myUuid] {
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;
        QSharedPointer<std::function<void(void)> > generateSun(new std::function<void(void)>);
        *generateSun = [this, generateSun, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            // Daytime — sleep, skip production
            if (isDaytime()) {
                (new Timer(picture, 24000, [this, generateSun, myUuid] {
                    PlantInstance *self6 = plantProtoType->scene->getPlant(myUuid);
                    if (!self6 || self6 != this) return;
                    (*generateSun)();
                }))->start();
                return;
            }
            int sunAmount = isAdult ? 25 : 15;
            picture->setMovieOnNewLoop(plantProtoType->normalGif, [this, generateSun, myUuid, sunAmount] {
                PlantInstance *self3 = plantProtoType->scene->getPlant(myUuid);
                if (!self3 || self3 != this) return;
                (new Timer(picture, 1000, [this, generateSun, myUuid, sunAmount] {
                    PlantInstance *self4 = plantProtoType->scene->getPlant(myUuid);
                    if (!self4 || self4 != this) return;
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(sunAmount);
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
                    // After sun produced, show appropriate animation (baby vs adult)
                    picture->setMovieOnNewLoop(isAdult ? adultGif : babyGif, [this, generateSun, myUuid] {
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

// DoomShroom（毁灭菇）
DoomShroom::DoomShroom()
{
    eName = "oDoomShroom";
    cName = tr("毁灭菇");
    beAttackedPointR = 30;
    sunNum = 5;
    coolTime = 0;
    night = true;
    cardGif = "Card/Plants/DoomShroom.png";
    staticGif = "Plants/DoomShroom/DoomShroom.gif";
    normalGif = "Plants/DoomShroom/DoomShroom.gif";
    toolTip = tr("毁灭菇是夜间植物，白天会睡觉<br/>效果：3×3范围秒杀<br/>花费：125<br/>冷却：50秒<br/>特性：爆炸后留下弹坑，本局无法再种植");
}

DoomShroomInstance::DoomShroomInstance(const Plant *plant)
    : PlantInstance(plant),
      sleepGif("Plants/DoomShroom/Sleep.gif"),
      boomGif("Plants/DoomShroom/Boom.png"),
      beginBoomGif("Plants/DoomShroom/BeginBoom.gif"),
      m_exploded(false)
{
}

bool DoomShroomInstance::isDaytime()
{
    if (m_awake) return false;
    return !plantProtoType->scene->nightMode();
}

void DoomShroomInstance::doExplosion()
{
    if (m_exploded) return;
    m_exploded = true;

    int c = col, r = row;
    Coordinate &coord = plantProtoType->scene->getCoordinate();
    qreal cellCX = coord.getX(c);
    qreal cellCY = coord.getY(r);

    // --- 1. 隐藏植物本体 ---
    picture->setVisible(false);

    // --- 2. 秒杀 3×3 网格内所有僵尸 → boomDie ---
    qreal rangeHalf = 150.0;
    for (int tr = qMax(1, r - 1); tr <= qMin(coord.rowCount(), r + 1); ++tr) {
        QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRowRange(
            tr, cellCX - rangeHalf, cellCX + rangeHalf);
        for (auto z : zombies) {
            if (z->hp > 0 && !z->goingDie) z->boomDie();
        }
    }

    // --- 3. 单个蘑菇云：Boom.png 为 10 帧水平序列，取第 1 帧，中心与格子对齐 ---
    QPixmap boomFull = gImageCache->load(boomGif);
    int frameW = boomFull.width() / 10;   // 10 帧水平排列，单帧宽度
    int frameH = boomFull.height();
    QPixmap boomSingle = boomFull.copy(0, 0, frameW, frameH);
    QGraphicsPixmapItem *boomEffect = new QGraphicsPixmapItem(boomSingle);
    // =====【蘑菇云垂直调整】改 -200 后面的值：负更大=上调 =====
    boomEffect->setPos(cellCX - frameW / 2, cellCY - frameH / 2 - 180);
    boomEffect->setZValue(200);
    plantProtoType->scene->addItem(boomEffect);

    // --- 4. 蘑菇云显示 1.5s 后 → 生成弹坑 → 永久禁止种植 → 自毁 ---
    (new Timer(plantProtoType->scene, 1500, [this, boomEffect, c, r, cellCX, cellCY] {
        delete boomEffect;
        plantProtoType->scene->addCrater(c, r);

        bool nightScene = plantProtoType->scene->getGameLevelData()->dKind == 0;
        QString craterPath = nightScene
            ? "Plants/DoomShroom/crater10.png"
            : "Plants/DoomShroom/crater11.png";
        QPixmap craterFull = gImageCache->load(craterPath);
        QPixmap craterPm = nightScene
            ? craterFull
            : craterFull.copy(0, 0, craterFull.width() / 2, craterFull.height());
        // =====【弹坑位置调整点】===== 改 +30(右移) / -40(上移) 调整弹坑位置
        QGraphicsPixmapItem *craterItem = new QGraphicsPixmapItem(craterPm);
        craterItem->setPos(cellCX - craterPm.width() / 2 + 30,
                           cellCY - craterPm.height() / 2 - 40);
        craterItem->setZValue(0);
        plantProtoType->scene->addToGame(craterItem);

        plantProtoType->scene->plantDie(this);
    }))->start();
}

void DoomShroomInstance::initTrigger()
{
    if (isDaytime()) { picture->setMovie(sleepGif); picture->start(); return; }
    picture->setMovie(beginBoomGif);
    picture->start();
    (new Timer(picture, 2200, [this] { doExplosion(); }))->start();
}

void DoomShroomInstance::wakeUp()
{
    PlantInstance::wakeUp();
    // 被唤醒后立即开始起爆
    picture->setMovie(beginBoomGif);
    picture->start();
    (new Timer(picture, 2200, [this] { doExplosion(); }))->start();
}

void DoomShroomInstance::onDayNightChanged(bool isNight)
{
    Q_UNUSED(isNight);
    if (m_awake) return;
    if (isDaytime()) {
        // 切换为白天 → 休眠（不引爆）
        picture->setMovie(sleepGif);
        picture->start();
        canTrigger = false;
    } else {
        // 切换为黑夜 → 苏醒（不引爆，仅切换动画）
        picture->setMovie(plantProtoType->normalGif);
        picture->start();
        canTrigger = true;
    }
}

// CoffeeBean（咖啡豆）：仅能种在白天休眠的蘑菇上，唤醒后自毁
CoffeeBean::CoffeeBean()
{
    eName = "oCoffeeBean";
    cName = tr("咖啡豆");
    pKind = 0; // 不同于蘑菇的 pKind=1，避免放置时杀死底层蘑菇
    sunNum = 75;
    coolTime = 7.5;
    cardGif = "Card/Plants/CoffeeBean.png";
    staticGif = "Plants/CoffeeBean/CoffeeBean.gif";
    normalGif = "Plants/CoffeeBean/CoffeeBean.gif";
    toolTip = tr("咖啡豆可以唤醒白天睡觉的蘑菇<br/>花费：75<br/>只能种在睡觉的蘑菇上<br/><br/>\n咖啡豆是蘑菇家族最好的朋友。每天早上它会准时递给蘑菇们一杯热咖啡，让它们清醒过来开始干活。");
}

bool CoffeeBean::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > 5) return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y)) return false;
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (!plants.contains(1)) return false; // need a plant on this cell
    PlantInstance *target = plants[1];
    // Must be a night plant AND currently sleeping (daytime + not yet awake)
    if (!target->plantProtoType->night) return false;
    if (!target->m_awake && !scene->nightMode()) return true; // sleeping shroom
    return false; // already awake or not daytime
}

CoffeeBeanInstance::CoffeeBeanInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

void CoffeeBeanInstance::initTrigger()
{
    // 获取目标蘑菇
    QMap<int, PlantInstance *> plants = plantProtoType->scene->getPlant(col, row);
    if (!plants.contains(1)) {
        plantProtoType->scene->plantDie(this);
        return;
    }

    PlantInstance *target = plants[1];
    // 二次验证：必须是夜间植物且处于睡眠状态（白天未唤醒）
    if (!target->plantProtoType->night || target->m_awake) {
        plantProtoType->scene->plantDie(this);
        return;
    }

    QUuid targetUuid = target->uuid;

    // 提升咖啡豆图层
    picture->setZValue(picture->zValue() + 10);

    // 播放动画（必须是单次播放）
    picture->setMovie("Plants/CoffeeBean/CoffeeBeanEat.gif");
    picture->start();

    // 动画结束 → 唤醒蘑菇 → 自毁
    QObject::connect(picture, &MoviePixmapItem::finished, [this, targetUuid] {
        PlantInstance *t = plantProtoType->scene->getPlant(targetUuid);
        if (t && !t->m_awake) {
            t->wakeUp();
        }
        plantProtoType->scene->plantDie(this);
    });
}
// ==================== 大嘴花（Chomper） ====================
Chomper::Chomper()
{
    eName = "oChomper";
    cName = tr("Chomper");
    beAttackedPointR = 45;
    sunNum = 150;
    coolTime = 7.5;
    cardGif = "Card/Plants/Chomper.png";
    staticGif = "Plants/Chomper/0.gif";
    normalGif = "Plants/Chomper/Chomper.gif";
    toolTip = tr("大嘴花可以一口吞掉一整只僵尸，但是他们消化僵尸的时候很脆弱。<br/>伤害：巨大<br/>范围：非常近<br/>特点：消化时间很长<br/><br/>大嘴花几乎可以去“恐怖小店”，来表演它的绝技了，不过他的经纪人压榨了他太多的钱，所以他没去成。尽管如此，大嘴花没有怨言，只说了句这只是交易的一部分。");
}

ChomperInstance::ChomperInstance(const Plant *plant)
    : PlantInstance(plant),
      isChewing(false),
      chewTimer(0),
      chewGif("Plants/Chomper/ChomperDigest.gif"),
      attackGif("Plants/Chomper/ChomperAttack.gif")
{
}

void ChomperInstance::initTrigger()
{
    Trigger *trigger = new Trigger(this, attackedLX, attackedLX + 60, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void ChomperInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (isChewing || zombieInstance->altitude <= 0 || zombieInstance->goingDie)
        return;
    if (!canTrigger) return;

    if (zombieInstance->attackedLX >= trigger->from && zombieInstance->attackedLX <= trigger->to) {
        canTrigger = false;
        picture->setMovie(attackGif);
        picture->start();
        zombieInstance->crushDie();  // 秒杀僵尸
        startChewing();
    }
}

void ChomperInstance::startChewing()
{
    isChewing = true;
    chewTimer = 30000;  // 30 秒消化

    picture->setMovie(chewGif);
    picture->start();

    QSharedPointer<std::function<void()>> chewLoop = QSharedPointer<std::function<void()>>::create();
    *chewLoop = [this, chewLoop] {
        if (!isChewing) return;
        chewTimer -= 1000;
        if (chewTimer <= 0) {
            finishChewing();
        } else {
            (new Timer(picture, 1000, *chewLoop))->start();
        }
    };
    (new Timer(picture, 1000, *chewLoop))->start();
}

void ChomperInstance::finishChewing()
{
    isChewing = false;
    picture->setMovie(plantProtoType->normalGif);
    picture->start();
    canTrigger = true;
}

void ChomperInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    if (isChewing) attack *= 2;
    PlantInstance::getHurt(zombie, aKind, attack);
}
// ==================== 高坚果（Tallnut） ====================
Tallnut::Tallnut()
{
    eName = "oTallnut";
    cName = tr("高坚果");
    hp = 8000;                      // 高血量（坚果墙为 4000）
    beAttackedPointR = 45;
    sunNum = 125;
    coolTime = 24.5;                // 冷却 24.5 秒
    cardGif = "Card/Plants/TallNut.png";
    staticGif = "Plants/TallNut/0.gif";
    normalGif = "Plants/TallNut/TallNut.gif";
    toolTip = tr("高坚果是重型壁垒植物，而且不会被跳过。<br/>韧性：非常高<br/>特殊：不会被跨过或越过<br/><br/>人们想知道，坚果墙和高坚果是否在竞争。高坚果以男中音的声调大声笑了。“我们之间怎么会存在竞争关系？我们是哥们儿。你知道坚果墙为我做了什么吗……”高坚果的声音越来越小，他狡黠地笑着。");
    // 注意：高坚果不攻击，所以无需设置攻击相关属性
}
TallnutInstance::TallnutInstance(const Plant *plant)
    : PlantInstance(plant),
      hurtStatus(0),
      crackedGif1("Plants/TallNut/TallnutCracked1.gif"),
      crackedGif2("Plants/TallNut/TallnutCracked2.gif")
{
}

void TallnutInstance::initTrigger()
{
    // 高坚果不攻击，不需要触发器
    // 但为了保持结构，可以留空或只调用基类（但基类会添加默认触发器，我们不希望如此）
    // 所以这里什么都不做，即不添加任何 trigger
}

void TallnutInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    PlantInstance::getHurt(zombie, aKind, attack);
    if (hp > 0) {
        // 根据血量显示不同裂痕
        if (hp < 2667) {           // 约 1/3 血量
            if (hurtStatus < 2) {
                hurtStatus = 2;
                picture->setMovie(crackedGif2);
                picture->start();
            }
        }
        else if (hp < 5334) {      // 约 2/3 血量
            if (hurtStatus < 1) {
                hurtStatus = 1;
                picture->setMovie(crackedGif1);
                picture->start();
            }
        }
    }
}
// ==================== 三线射手（Threepeater） ====================
Threepeater::Threepeater()
{
    eName = "oThreepeater";
    cName = tr("三线射手");
    beAttackedPointR = 51;
    sunNum = 325;
    coolTime = 7.5;
    cardGif = "Card/Plants/Threepeater.png";
    staticGif = "Plants/Threepeater/0.gif";
    normalGif = "Plants/Threepeater/Threepeater.gif";
    toolTip = tr("三线射手可以在三条线上同时射出豌豆。<br/>伤害：普通(每颗)<br/>范围：三线<br/><br/>三线射手喜欢读书，下棋和在公园里呆坐。他也喜欢演出，特别是现代爵士乐。“我正在寻找我生命中的另一半，”他说。三线射手最爱的数字是5。");
}
ThreepeaterInstance::ThreepeaterInstance(const Plant *plant)
    : PeashooterInstance(plant)
{
}

void ThreepeaterInstance::initTrigger()
{
    // 使用循环定时器检测，间隔 1400ms（与豌豆射手攻击间隔一致）
    QUuid myUuid = uuid;
    (new Timer(picture, 500, [this, myUuid] {
        PlantInstance *self = plantProtoType->scene->getPlant(myUuid);
        if (!self || self != this) return;

        QSharedPointer<std::function<void()>> loop = QSharedPointer<std::function<void()>>::create();
        *loop = [this, loop, myUuid] {
            PlantInstance *self2 = plantProtoType->scene->getPlant(myUuid);
            if (!self2 || self2 != this) return;
            checkAndFire();
            (new Timer(picture, 1400, *loop))->start();
        };
        (*loop)();  // 立即执行第一次检测
    }))->start();
}

void ThreepeaterInstance::checkAndFire()
{
    // 检测三行（本行、上一行、下一行）是否存在僵尸
    bool hasZombie = false;
    for (int off = -1; off <= 1; ++off) {
        int tr = row + off;
        if (tr < 1 || tr > plantProtoType->scene->getCoordinate().rowCount())
            continue;
        QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(tr);
        for (auto *z : zombies) {
            // 僵尸存活且在攻击范围内（右侧未越过屏幕边缘）
            if (z->hp > 0 && !z->goingDie && z->attackedLX > attackedLX && z->attackedLX < 880) {
                hasZombie = true;
                break;
            }
        }
        if (hasZombie) break;
    }

    if (!hasZombie) return;  // 三行均无僵尸，不攻击

    // 播放一次攻击音效（豌豆射手音效）
    firePea->play();

    // 同时向三行各发射一颗豌豆
    fireLine(0);
    fireLine(-1);
    fireLine(1);
}

void ThreepeaterInstance::fireLine(int rowOffset)
{
    int targetRow = row + rowOffset;
    if (targetRow < 1 || targetRow > plantProtoType->scene->getCoordinate().rowCount())
        return;

    // 获取目标行的 Y 坐标（确保子弹对齐该行）
    Coordinate &coord = plantProtoType->scene->getCoordinate();
    qreal targetY = coord.getY(targetRow) -50;  // 与豌豆射手子弹高度偏移一致//改三颗豌豆的高度

    // 创建子弹（类型 0 为普通豌豆），目标行设为 targetRow
    (new Bullet(plantProtoType->scene, 0, targetRow, attackedLX, attackedLX - 40,
                targetY, picture->zValue() + 2, 0))->start();
}



// ==================== 双胞向日葵 ====================
TwinSunflower::TwinSunflower()
{
    eName = "oTwinSunflower";
    cName = tr("双胞向日葵");
    beAttackedPointR = 53;
    sunNum = 150;
    coolTime = 7.5;
    cardGif = "Card/Plants/TwinSunflower.png";
    staticGif = "Plants/TwinSunflower/TwinSunflower1.gif";//静态动图
    normalGif = "Plants/TwinSunflower/TwinSunflower1.gif";//
    toolTip = tr("双胞向日葵的阳光产量是普通向日葵的两倍。\n阳光产量：双倍\n只能种在普通向日葵上\n\n这是一个疯狂的夜晚，禁忌的科学技术，让双胞向日葵来到了这个世界。电闪雷鸣狂风怒吼都在表示着这个世界对他的拒绝。但是一切无济于事，双子向日葵他却仍然活着！");
}

bool TwinSunflower::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > 5)
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    // 仅能种在普通向日葵上（非空地、非其他植物）
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    if (!plants.contains(1)) return false; // 必须有 pKind=1 植物
    PlantInstance *p = plants[1];
    return (p->plantProtoType->eName == "oSunflower");
}

TwinSunflowerInstance::TwinSunflowerInstance(const Plant *plant)
    : SunFlowerInstance(plant)
{
    // 覆盖基类的 lightedGif，使用双胞向日葵自己的发光动画
    lightedGif = "Plants/TwinSunflower/TwinSunflower1.gif";
}

void TwinSunflowerInstance::birth(int c, int r)
{
    // 先删除该格已存在的普通向日葵，再放置自己
    QMap<int, PlantInstance *> plants = plantProtoType->scene->getPlant(c, r);
    if (plants.contains(1)) {
        PlantInstance *old = plants[1];
        if (old->plantProtoType->eName == "oSunflower") {
            plantProtoType->scene->plantDie(old);
        }
    }
    PlantInstance::birth(c, r);
}

void TwinSunflowerInstance::initTrigger()
{
    // 产 200 阳光（普通向日葵 100 的 2 倍），继承同样的循环逻辑
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

                    //双胞向日葵产生阳光值的设定
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(200);
                    //
                    
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

// ==================== 魅惑菇（Hypno-shroom） ====================
HypnoShroom::HypnoShroom()
{
    eName = "oHypnoShroom";
    cName = tr("魅惑菇");
    hp = 300;                     // 与普通植物相同
    beAttackedPointR = 45;
    sunNum = 5;
    coolTime = 0;                // 冷却 30 秒
    night = true;                 // 夜间植物
    cardGif = "Card/Plants/HypnoShroom.png";
    staticGif = "Plants/HypnoShroom/0.gif";
    normalGif = "Plants/HypnoShroom/HypnoShroom.gif";
    toolTip = tr("当僵尸吃下魅惑菇后，他将会掉转方向为你作战。<br/>使用方法：单独使用，接触生效<br/>特点：让一只僵尸为你作战<br/>白天睡觉<br/><br/>魅惑菇声称：“僵尸们是我们的朋友，他们被严重误解了，僵尸们在我们的生态环境里扮演着重要角色。我们可以也应当更努力地让他们学会用我们的方式来思考。”");
    // 不攻击，所以无需设置攻击相关属性
}
HypnoShroomInstance::HypnoShroomInstance(const Plant *plant)
    : PlantInstance(plant),
      sleepGif("Plants/HypnoShroom/HypnoShroomSleep.gif")
{
}

bool HypnoShroomInstance::isDaytime()
{
    if (m_awake) return false;  // 被咖啡豆唤醒后不睡觉

    //光敏传感器接口，切换为白天模式时，魅惑菇进入睡眠状态
    return !plantProtoType->scene->nightMode();
}

void HypnoShroomInstance::birth(int c, int r)
{
    PlantInstance::birth(c, r);
    if (isDaytime()) {
        picture->setMovie(sleepGif);//白天睡觉
        picture->start();//启动动图
    }
}

//咖啡豆唤醒接口
void HypnoShroomInstance::wakeUp()
{
    PlantInstance::wakeUp();
    picture->setMovie(plantProtoType->normalGif);
    picture->start();
}

//白天夜晚切换接口
void HypnoShroomInstance::onDayNightChanged(bool isNight)
{
    Q_UNUSED(isNight);
    if (m_awake) return;
    if (isDaytime()) {
        picture->setMovie(sleepGif);
        picture->start();
    } else {
        picture->setMovie(plantProtoType->normalGif);
        picture->start();
    }
}

void HypnoShroomInstance::getHurt(ZombieInstance *zombie, int aKind, int attack)
{
    // 如果正在睡眠，或者僵尸无效，不触发
    if (isDaytime() || !zombie || zombie->goingDie)
        return;

    // 只响应普通啃咬攻击（aKind == 0）
    if (aKind == 0 && hp > 0) {
        // 魅惑该僵尸
        zombie->hypnotize();
        // 植物自毁（被吃掉）
        plantProtoType->scene->plantDie(this);
        return;
    }
    // 其他伤害（如爆炸）按正常处理
    PlantInstance::getHurt(zombie, aKind, attack);
}




//向日葵
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
                    auto sunGifAndOnFinished = plantProtoType->scene->newSun(100);
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
                            z->boomDie();
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

// Jalapeno（火爆辣椒）：秒杀整行，爆炸动画，自毁
Jalapeno::Jalapeno()
{
    eName = "oJalapeno";
    cName = tr("火爆辣椒");
    beAttackedPointR = 30;
    sunNum = 0;
    coolTime = 5; // 冷却时间修改
    cardGif = "Card/Plants/Jalapeno.png";
    staticGif = "Plants/Jalapeno/Jalapeno.gif";
    normalGif = "Plants/Jalapeno/Jalapeno.gif";
    toolTip = tr("火爆辣椒会烧毁整行僵尸<br/>效果：秒杀整行<br/>花费：125<br/><br/>\n火爆辣椒有一颗火爆脾气，沾上一点就会把整行都烧成灰烬。它强烈的辣味能让僵尸们再也不想靠近。");
}

JalapenoInstance::JalapenoInstance(const Plant *plant)
    : PlantInstance(plant)
{
}

void JalapenoInstance::initTrigger()
{
    // Hide the plant sprite (we show full-row explosion instead)
    picture->setVisible(false);

    // =====【火爆辣椒 爆炸位置调整点】=====
    // 整行火焰覆盖左右 80→880，垂直限制在当前行内不越界
    // =====
    Coordinate &coord = plantProtoType->scene->getCoordinate();
    qreal rowCenter = coord.getY(row);//火焰调整的位置
    qreal rowTop =  rowCenter - 80;   // ↑【火焰上调】减小此值(如-90)火焰更靠上
    qreal rowBot = rowCenter + 20;    // ↓【火焰下调】增大此值(如+30)火焰更靠下
    qreal rowH   = rowBot - rowTop;   // 行高 ≈ 100px

    // 将火图缩放到整行尺寸
    QPixmap firePixmap = gImageCache->load("Plants/Jalapeno/JalapenoAttack.gif");
    qreal rowLeft = 80.0, rowRight = 880.0;
    firePixmap = firePixmap.scaled((int)(rowRight - rowLeft), (int)rowH,
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QGraphicsPixmapItem *fireRow = new QGraphicsPixmapItem(firePixmap);
    fireRow->setPos(rowLeft, rowTop);
    fireRow->setZValue(200);
    plantProtoType->scene->addItem(fireRow);

    // Kill all zombies on this row with boomDie
    QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRow(row);
    for (auto *z : zombies) {
        if (z->hp > 0 && !z->goingDie)
            z->boomDie();
    }

    // Explosion sound effect
    QMediaPlayer *player = new QMediaPlayer(plantProtoType->scene);
    player->setMedia(QUrl("qrc:/audio/cherrybomb.mp3"));
    player->play();

    // Auto-destroy after animation
    (new Timer(plantProtoType->scene, 1000, [this, fireRow] {
        delete fireRow;
        plantProtoType->scene->plantDie(this);
    }))->start();
}

void JalapenoInstance::triggerCheck(ZombieInstance *, Trigger *)
{
    // Instant-use plant — no trigger-based attack
}

// Squash（倭瓜）：放置后待机，僵尸进入范围跳起砸下秒杀并自毁
Squash::Squash()
{
    eName = "oSquash";
    cName = tr("倭瓜");
    beAttackedPointR = 45;
    sunNum = 50;
    coolTime = 20;
    cardGif = "Card/Plants/Squash.png";
    staticGif = "Plants/Squash/Squash.gif";
    normalGif = "Plants/Squash/Squash.gif";
    canEat = true;
    toolTip = tr("倭瓜会跳起来砸扁靠近的僵尸<br/>效果：秒杀该格僵尸<br/>花费：50<br/><br/>\n倭瓜是植物家族里最重的战士。它虽然不爱说话，但是只要它往僵尸身上一坐，就知道它的分量了。");
}

SquashInstance::SquashInstance(const Plant *plant)
    : PlantInstance(plant),
      attackGif("Plants/Squash/SquashAttack.gif")
{
}

void SquashInstance::initTrigger()
{
    // Trigger on the plant's own cell — zombie within 1 tile triggers the squash
    Trigger *trigger = new Trigger(this, attackedLX - 40, attackedRX + 40, 0, 0);
    triggers.insert(row, QList<Trigger *>{ trigger });
    plantProtoType->scene->addTrigger(row, trigger);
}

void SquashInstance::triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger)
{
    if (zombieInstance->altitude <= 0) return;
    if (!canTrigger) return;
    canTrigger = false;

    qreal origX = picture->pos().x();
    qreal origY = picture->pos().y();

    // Phase 1: Jump up
    Animate(picture).move(QPointF(origX, origY - 60)).duration(200).shape(QTimeLine::EaseOutCurve).finish([this, origX, origY] {
        // Phase 2: L/R squash anticipation — each frame 1 second interval
        picture->setPixmap(gImageCache->load("Plants/Squash/SquashL.PNG"));
        (new Timer(picture, 1000, [this, origX, origY] {
            picture->setPixmap(gImageCache->load("Plants/Squash/SquashR.png"));
            (new Timer(picture, 1000, [this, origX, origY] {
                // Phase 3: Smash down with attack animation
                picture->setMovie(attackGif);
                picture->start();
                Animate(picture).move(QPointF(origX, origY + 10)).scale(1.2).duration(200).shape(QTimeLine::EaseInCurve).finish([this] {
                    // Phase 4: Kill zombies in range and self-destruct
                    QList<ZombieInstance *> zombies = plantProtoType->scene->getZombieOnRowRange(
                        row, attackedLX - 40, attackedRX + 40);
                    for (auto *z : zombies) {
                        if (z->hp > 0 && !z->goingDie)
                            z->crushDie();
                    }
                    plantProtoType->scene->plantDie(this);
                });
            }))->start();
        }))->start();
    });
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

// ==================== 荷叶（LilyPad） ====================
LilyPad::LilyPad()
{
    eName = "oLilyPad";
    cName = tr("荷叶");
    hp = 300;
    pKind = 0;  // 占水域 base 位，其他植物种在它上面
    sunNum = 25;
    beAttackedPointR = 53;
    canEat = true;   // 僵尸可以啃咬荷叶
    cardGif = "Card/Plants/LilyPad.png";
    staticGif = "Plants/LilyPad/LilyPad.gif";
    normalGif = "Plants/LilyPad/LilyPad.gif";
    toolTip = tr("Allows non-aquatic plants to be placed on water");
}

bool LilyPad::canGrow(int x, int y) const
{
    if (x < 1 || x > 9 || y < 1 || y > scene->getCoordinate().rowCount())
        return false;
    if (scene->isCrater(x, y) || scene->isTombstone(x, y))
        return false;
    int groundType = scene->getGameLevelData()->LF[y];
    if (groundType != 2) return false; // 仅水域
    QMap<int, PlantInstance *> plants = scene->getPlant(x, y);
    return !plants.contains(0); // 无荷叶即可种
}

LilyPadInstance::LilyPadInstance(const Plant *plant)
    : PlantInstance(plant)
{}

Bullet::Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue, int direction, qreal speed)
        : scene(scene), type(type), row(row), direction(direction), from(from), speed(speed), isFire(false)
        // type:0豌豆 1冰豆 2孢子 3毒气; speed:默认5,左向10
{
    count = 0;
    QString picName;
    if (type == 3)
        picName = QString("Plants/FumeShroom/FumeShroomBullet.gif");
    else if (type == 2)
        picName = QString("Plants/ShroomBullet.gif");
    else if (type == 1)
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

    // ===== Torchwood conversion (first entry only) =====
    tryTorchwoodConvert();

    ZombieInstance *zombie = nullptr;
    if (direction == 0) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto iter = zombies.end(); iter-- != zombies.begin() && (*iter)->attackedLX <= from;) {
            if ((*iter)->hp > 0 && (*iter)->attackedRX >= from) { zombie = *iter; break; }
        }
    } else {
        // direction == 1: bullet moving right→left, check all zombies
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp > 0 && !z->goingDie && z->attackedLX <= from && z->attackedRX >= from) {
                zombie = z; break;
            }
        }
    }
    // Type 3 (fume): penetrate all zombies in range
    if (type == 3) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp > 0 && !z->goingDie && z->attackedLX >= from && z->attackedLX <= from + 320)
                z->getPea(20, direction, type);  // type 此时为 3
        }
        (new Timer(scene, 500, [this] { delete this; }))->start();
        return;
    }
    if (zombie && zombie->altitude == 1) {
        // Base damage (fire peas do double)
        int dmg = isFire ? 40 : 20;
        zombie->getPea(dmg, direction, type);
        // Type 1 (snow pea, not fire): apply slow
        if (type == 1 && !isFire) {
            zombie->applySlow(0.5, 5000);
        }
        // Fire pea splash damage
        if (isFire) {
            doSplashDamage();
        }
        // Hit effect
        picture->setPos(picture->pos() + QPointF(28, 0));
        if (type == 2)
            picture->setPixmap(gImageCache->load("Plants/ShroomBulletHit.gif"));
        else
            picture->setPixmap(gImageCache->load("Plants/PeaBulletHit.gif"));
        (new Timer(scene, 100, [this] { delete this; }))->start();
    }
    else {
        qreal step = direction ? -speed : speed;
        from += step;
        if (from < 900 && from > 100) {
            picture->setPos(picture->pos() + QPointF(step, 0));
            (new Timer(scene, 10, [this] { move(); }))->start();
        }
        else
            delete this;
    }
}

// ===== Torchwood: convert pea to fire pea (one-time, first entry only) =====
void Bullet::tryTorchwoodConvert()
{
    if (isFire || type >= 2) return; // already fire, or not a pea type
    int col = scene->getCoordinate().getCol(from);
    QMap<int, PlantInstance *> plants = scene->getPlant(col, row);
    if (!plants.contains(1) || plants[1]->plantProtoType->eName != "oTorchwood") return;

    isFire = true; // mark before changing to prevent re-entry
    // Remove slow if was snow pea, keep type for base damage calc
    // PB -> fire: direction 0→PB10.gif, direction 1→PB11.gif
    QString fireGif = direction == 0
        ? QString("Plants/PB10.gif") : QString("Plants/PB11.gif");
    picture->setPixmap(gImageCache->load(fireGif));
}

// ===== Fire pea splash: hit nearby zombies on same row =====
void Bullet::doSplashDamage()
{
    // Show splash fire animation at hit point
    QGraphicsPixmapItem *splash = new QGraphicsPixmapItem(
        gImageCache->load("Plants/Torchwood/SputteringFire.gif"));
    splash->setPos(picture->pos());
    splash->setZValue(picture->zValue() + 1);
    scene->addItem(splash);
    // Auto-destroy after 500ms (animation should be ~500ms)
    (new Timer(scene, 500, [splash] { delete splash; }))->start();

    // Splash damage to nearby zombies on same row (~60px radius)
    QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
    qreal hitX = picture->pos().x();
    for (auto *z : zombies) {
        if (z->hp <= 0 || z->goingDie) continue;
        qreal dist = qAbs(z->attackedLX - hitX);
        if (dist < 60) {
            z->getPea(10, direction, 0);  // 溅射视为普通豌豆; 
        }
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
    else if (eName == "oSplitPea")//双向射手
        plant = new SplitPea;
    else if (eName == "oPuffShroom")//小喷菇
        plant = new PuffShroom;
    else if (eName == "oScaredyShroom")//胆小菇
        plant = new ScaredyShroom;
    else if (eName == "oFumeShroom")//大喷菇
        plant = new FumeShroom;
    else if (eName == "oTorchwood")//火炬树桩
        plant = new Torchwood;
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
    else if (eName == "oJalapeno")//火爆辣椒
        plant = new Jalapeno;
    else if (eName == "oSquash")//倭瓜
        plant = new Squash;
    else if (eName == "oSunShroom")//阳光菇
        plant = new SunShroom;
    else if (eName == "oDoomShroom")//毁灭菇
        plant = new DoomShroom;
    else if (eName == "oCoffeeBean")//咖啡豆
        plant = new CoffeeBean;
    else if (eName == "oChomper")//食人花
        plant = new Chomper;
    else if (eName == "oTallnut")//高坚果
        plant = new Tallnut;
    else if (eName == "oThreepeater")//三线射手
        plant = new Threepeater;
    else if (eName == "oHypnoShroom")//魅惑菇
        plant = new HypnoShroom;
    else if (eName == "oTwinSunflower")//双胞向日葵
        plant = new TwinSunflower;
    else if (eName == "oLilyPad")//荷叶
        plant = new LilyPad;
    else if (eName == "oSeaShroom")//海蘑菇
        plant = new SeaShroom;
    else if (eName == "oTangleKlep")//缠绕水草
        plant = new TangleKelp;
    else if (eName == "oSpikeweed")//地刺
        plant = new Spikeweed;
    else if (eName == "oSpikerock")//地刺王
        plant = new Spikerock;
    else if (eName == "oPumpkinHead")//南瓜头
        plant = new PumpkinHead;
    if (plant) {
        plant->scene = scene;
        plant->update();
    }
    return plant;
}

PlantInstance *PlantInstanceFactory(const Plant *plant)
{
    if (plant->eName == "oPeashooter")//豌豆射手
        return new PeashooterInstance(plant);
    if (plant->eName == "oRepeater")//双发射手
        return new RepeaterInstance(plant);
    if (plant->eName == "oGatlingPea")//机枪射手
        return new GatlingPeaInstance(plant);
    if (plant->eName == "oSplitPea")//双向射手
        return new SplitPeaInstance(plant);
    if (plant->eName == "oPuffShroom")//小喷菇
        return new PuffShroomInstance(plant);
    if (plant->eName == "oScaredyShroom")//胆小菇
        return new ScaredyShroomInstance(plant);
    if (plant->eName == "oFumeShroom")//大喷菇
        return new FumeShroomInstance(plant);
    if (plant->eName == "oTorchwood")//火炬树桩
        return new TorchwoodInstance(plant);
    if (plant->eName == "oSnowPea")//寒冰射手
        return new SnowPeaInstance(plant);
    if (plant->eName == "oSunflower")//向日葵
        return new SunFlowerInstance(plant);
    if (plant->eName == "oCherryBomb")//樱桃炸弹
        return new CherryBombInstance(plant);
    if (plant->eName == "oJalapeno")//火爆辣椒
        return new JalapenoInstance(plant);
    if (plant->eName == "oSquash")//倭瓜
        return new SquashInstance(plant);
    if (plant->eName == "oSunShroom")//阳光菇
        return new SunShroomInstance(plant);
    if (plant->eName == "oDoomShroom")//毁灭菇
        return new DoomShroomInstance(plant);
    if (plant->eName == "oWallNut")//坚果墙
        return new WallNutInstance(plant);
    if (plant->eName == "oLawnCleaner")//小车
        return new LawnCleanerInstance(plant);
    if (plant->eName == "oChomper")//食人花
        return new ChomperInstance(plant);
    if (plant->eName == "oTallnut")//高坚果
        return new TallnutInstance(plant);
    if (plant->eName == "oThreepeater")//三线射手
        return new ThreepeaterInstance(plant);
    if (plant->eName == "oHypnoShroom")//魅惑菇
        return new HypnoShroomInstance(plant);
    if (plant->eName == "oTwinSunflower")//双胞向日葵
        return new TwinSunflowerInstance(plant);
    if (plant->eName == "oLilyPad")//荷叶
        return new LilyPadInstance(plant);
    if (plant->eName == "oSeaShroom")//海蘑菇
        return new SeaShroomInstance(plant);
    if (plant->eName == "oTangleKlep")//缠绕水草
        return new TangleKelpInstance(plant);
    if (plant->eName == "oSpikeweed")//地刺
        return new SpikeweedInstance(plant);
    if (plant->eName == "oSpikerock")//地刺王
        return new SpikerockInstance(plant);
    if (plant->eName == "oPumpkinHead")//南瓜头
        return new PumpkinHeadInstance(plant);
    return new PlantInstance(plant);
}



