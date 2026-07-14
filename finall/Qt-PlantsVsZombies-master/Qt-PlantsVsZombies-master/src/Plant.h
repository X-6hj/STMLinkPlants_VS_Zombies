//
// Created by sun on 8/26/16.
//

#ifndef PLANTS_VS_ZOMBIES_PLANT_H
#define PLANTS_VS_ZOMBIES_PLANT_H

#include <QtCore>
#include <QtWidgets>
#include <QtMultimedia>

class MoviePixmapItem;
class GameScene;
class ZombieInstance;
class Trigger;

class Plant
{
    Q_DECLARE_TR_FUNCTIONS(Plant)

public:
    Plant();
    virtual  ~Plant() {}

    QString eName, cName;
    int width, height;
    int hp, pKind, bKind;
    int beAttackedPointL, beAttackedPointR;
    int zIndex;
    QString cardGif, staticGif, normalGif;
    bool canEat, canSelect, night;
    double coolTime;
    int stature, sleep;
    int sunNum;
    QString toolTip;

    virtual double getDX() const;
    virtual double getDY(int x, int y) const;
    virtual bool canGrow(int x, int y) const;

    GameScene *scene;
    void update();
};

class PlantInstance
{
public:
    PlantInstance(const Plant *plant);
    virtual ~PlantInstance();

    virtual void birth(int c, int r);
    virtual void initTrigger();
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger);
    virtual void normalAttack(ZombieInstance *zombieInstance);
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack);
    
    virtual void wakeUp();  // 咖啡豆唤醒休眠植物
    virtual void onDayNightChanged(bool isNight);  // 光敏传感器触发昼夜切换时调用

    bool contains(const QPointF &pos);

    const Plant *plantProtoType;

    QUuid uuid;
    int row, col;
    int hp;
    bool canTrigger;
    bool m_awake; // 是否已被咖啡豆唤醒
    qreal attackedLX, attackedRX;
    QMap<int, QList<Trigger *> > triggers;

    QGraphicsPixmapItem *shadowPNG;
    MoviePixmapItem *picture;
};

class Peashooter: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Peashooter)
public:
    Peashooter();
};

class PeashooterInstance: public PlantInstance
{
public:
    PeashooterInstance(const Plant *plant);
    virtual void normalAttack(ZombieInstance *zombieInstance);
protected:
    QString bulletGif, bulletHitGif;
    QMediaPlayer *firePea;
};

class SnowPea: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(SnowPea)
public:
    SnowPea();
};

class SnowPeaInstance: public PeashooterInstance
{
public:
    SnowPeaInstance(const Plant *plant);
    virtual void normalAttack(ZombieInstance *zombieInstance);
};

// 双射豌豆射手（Repeater）：在短时间内发射两颗豌豆
class Repeater: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(Repeater)
public:
    Repeater();
};

class RepeaterInstance: public PeashooterInstance
{
public:
    RepeaterInstance(const Plant *plant);
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
};

// 机枪射手（GatlingPea）：一次发射四颗豌豆
class GatlingPea: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(GatlingPea)
public:
    GatlingPea();
};

class GatlingPeaInstance: public PeashooterInstance
{
public:
    GatlingPeaInstance(const Plant *plant);
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
};

// 双向射手（SplitPea）：向右射速正常，向左射速2倍
class SplitPea: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(SplitPea)
public:
    SplitPea();
};

class SplitPeaInstance: public PeashooterInstance
{
public:
    SplitPeaInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
private:
    void fireRight();
    void fireLeft();
    bool m_canTriggerRight;
    bool m_canTriggerLeft;
};

// 小喷菇（PuffShroom）：夜间植物，白天睡觉，短程攻击，免费
class PuffShroom: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(PuffShroom)
public:
    PuffShroom();
};

class PuffShroomInstance: public PeashooterInstance
{
public:
    PuffShroomInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
    virtual void onDayNightChanged(bool isNight) override;
protected:
    QString sleepGif;
    bool isDaytime();
};

// 海蘑菇（SeaShroom）：水生夜间植物，0阳光，仅水域种植，复用 PuffShroom 攻击/昼夜逻辑
class SeaShroom: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(SeaShroom)
public:
    SeaShroom();
    virtual bool canGrow(int x, int y) const override;
};

class SeaShroomInstance: public PuffShroomInstance
{
public:
    SeaShroomInstance(const Plant *plant);
};

// 缠绕水草（TangleKelp）：水生一次性植物，将靠近的水域僵尸拉入水中秒杀
class TangleKelp: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(TangleKelp)
public:
    TangleKelp();
    virtual bool canGrow(int x, int y) const override;
};

class TangleKelpInstance: public PlantInstance
{
public:
    TangleKelpInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
private:
    bool m_triggered;
    QString floatGif, attackGif, grabPng, splashPng;
};

// 地刺（Spikeweed）：陆地专用，僵尸踩过持续受伤，不可啃食
class Spikeweed: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Spikeweed)
public:
    Spikeweed();
};

class SpikeweedInstance: public PlantInstance
{
public:
    SpikeweedInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
};

// 地刺王（Spikerock）：升级版地刺，必须种在地刺上，耐久对应尖刺数量
class Spikerock: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Spikerock)
public:
    Spikerock();
    virtual bool canGrow(int x, int y) const override;
};

class SpikerockInstance: public SpikeweedInstance
{
public:
    SpikerockInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
private:
    void updateSprite();
};

// 南瓜头（PumpkinHead）：护甲植物，套在内部植物外层，僵尸优先啃食
class PumpkinHead: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(PumpkinHead)
public:
    PumpkinHead();
    virtual bool canGrow(int x, int y) const override;
};

class PumpkinHeadInstance: public PlantInstance
{
public:
    PumpkinHeadInstance(const Plant *plant);
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
private:
    void updateDamageSprite();
};

// 胆小菇（ScaredyShroom）：夜间植物，白天睡觉，全屏攻击，僵尸靠近时停止攻击并哭泣
class ScaredyShroom: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(ScaredyShroom)
public:
    ScaredyShroom();
};

class ScaredyShroomInstance: public PuffShroomInstance
{
public:
    ScaredyShroomInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
private:
    QString cryGif;
    bool isScared() const;
    void enterScared();
    void exitScared();
    bool m_scared;
};

// 大喷菇（FumeShroom）：夜间植物，4格范围喷射毒气，攻击动画与子弹同步
class FumeShroom: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(FumeShroom)
public:
    FumeShroom();
};

class FumeShroomInstance: public PuffShroomInstance
{
public:
    FumeShroomInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
private:
    QString attackGif;
    QString bulletGif;
};

// 忧郁菇（GloomShroom）：升级型蘑菇，必须种在大喷菇上，3×3范围群体孢子伤害，白天睡觉
class GloomShroom: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(GloomShroom)
public:
    GloomShroom();
    virtual bool canGrow(int x, int y) const override;
};

class GloomShroomInstance: public PuffShroomInstance
{
public:
    GloomShroomInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void birth(int c, int r) override;
private:
    void doSporeAttack();
};

// 火炬树桩（Torchwood）：将穿过它的豌豆变成火豌豆（PB-10.gif），攻击力翻倍
class Torchwood: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Torchwood)
public:
    Torchwood();
};

class TorchwoodInstance: public PlantInstance
{
public:
    TorchwoodInstance(const Plant *plant);
    virtual void initTrigger() override;
};

// 阳光菇（SunShroom）：两阶段成长，白天睡觉，幼年产15阳光，成年产25阳光
class SunShroom: public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(SunShroom)
public:
    SunShroom();
};

class SunShroomInstance: public PlantInstance
{
public:
    SunShroomInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
    virtual void wakeUp() override;
    virtual void onDayNightChanged(bool isNight) override;
private:
    QString babyGif, adultGif, sleepGif;
    int grownAge;          // ms since planted
    bool isAdult;          // ready to produce 25 sun?
    bool isDaytime();
    void tryGrow();
};

// 毁灭菇（DoomShroom）：夜晚爆炸，白天休眠，3x3秒杀，留下弹坑
class DoomShroom: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(DoomShroom)
public:
    DoomShroom();
};

class DoomShroomInstance: public PlantInstance
{
public:
    DoomShroomInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void wakeUp() override;
    virtual void onDayNightChanged(bool isNight) override;
private:
    QString sleepGif, boomGif, beginBoomGif;
    bool m_exploded;
    bool isDaytime();
    void doExplosion();
};

// 咖啡豆（CoffeeBean）：仅能种在白天休眠的蘑菇上，唤醒后自毁
class CoffeeBean: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(CoffeeBean)
public:
    CoffeeBean();
    virtual bool canGrow(int x, int y) const override;
};

class CoffeeBeanInstance: public PlantInstance
{
public:
    CoffeeBeanInstance(const Plant *plant);
    virtual void initTrigger() override;
};
//向日葵（SunFlower）：白天植物，产阳光
class SunFlower: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(SunFlower)
public:
    SunFlower();
};

class SunFlowerInstance: public PlantInstance
{
public:
    SunFlowerInstance(const Plant *plant);
    virtual void initTrigger();
protected:
    QString lightedGif;
};
//坚果墙（WallNut）：高血量防御植物，僵尸攻击时会逐渐破损
class WallNut: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(WallNut)
public:
    WallNut();
    virtual bool canGrow(int x, int y) const;
};

class WallNutInstance: public PlantInstance
{
public:
    WallNutInstance(const Plant *plant);
    virtual void initTrigger();
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack);
private:
    int hurtStatus;
    QString crackedGif1, crackedGif2;
};

//小车（LawnCleaner）：放置后待机，僵尸进入范围时冲向僵尸并自毁
class LawnCleaner: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(LawnCleaner)
public:
    LawnCleaner();
};

class CherryBomb: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(CherryBomb)
public:
    CherryBomb();
};

class CherryBombInstance: public PlantInstance
{
public:
    CherryBombInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
};

// 火爆辣椒（Jalapeno）：放置后立即秒杀整行僵尸，播放爆炸特效后自毁
class Jalapeno: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Jalapeno)
public:
    Jalapeno();
};

class JalapenoInstance: public PlantInstance
{
public:
    JalapenoInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
};

// 倭瓜（Squash）：放置后待机，僵尸进入范围时跳起砸下秒杀该格内所有僵尸并自毁
class Squash: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Squash)
public:
    Squash();
};

class SquashInstance: public PlantInstance
{
public:
    SquashInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
private:
    QString attackGif;
};
// 池塘清扫车（PoolCleaner）：放置后待机，僵尸进入范围时冲向僵尸并自毁
class LawnCleanerInstance: public PlantInstance
{
public:
    LawnCleanerInstance(const Plant *plant);
    virtual void initTrigger();
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger);
    virtual void normalAttack(ZombieInstance *zombieInstance);
};

class PoolCleaner: public LawnCleaner
{
    Q_DECLARE_TR_FUNCTIONS(PoolCleaner)
public:
    PoolCleaner();
};

class PoolCleanerInstance: public LawnCleanerInstance
{
public:
    PoolCleanerInstance(const Plant *plant);
};
// ==================== 大嘴花（Chomper） ====================
class Chomper : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Chomper)
public:
    Chomper();
};

class ChomperInstance : public PlantInstance
{
public:
    ChomperInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;

private:
    bool isChewing;
    qreal chewTimer;
    QString chewGif;
    QString attackGif;
    void startChewing();
    void finishChewing();
};
// ==================== 高坚果（Tallnut） ====================
class Tallnut : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Tallnut)
public:
    Tallnut();
};

class TallnutInstance : public PlantInstance
{
public:
    TallnutInstance(const Plant *plant);
    virtual void initTrigger() override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;

private:
    int hurtStatus;
    QString crackedGif1, crackedGif2;
};
// ==================== 三线射手（Threepeater） ====================
class Threepeater : public Peashooter
{
    Q_DECLARE_TR_FUNCTIONS(Threepeater)
public:
    Threepeater();
};

class ThreepeaterInstance : public PeashooterInstance
{
public:
    ThreepeaterInstance(const Plant *plant);
    virtual void initTrigger() override;

private:
    void fireLine(int rowOffset);
    void checkAndFire();  // 检测三行并发射
};


// ==================== 杨桃（Starfruit） ====================
// 五方向攻击：正左、正上、正下、右上、右下，每颗伤害等同普通豌豆
class Starfruit : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(Starfruit)
public:
    Starfruit();
};

class StarfruitInstance : public PlantInstance
{
public:
    StarfruitInstance(const Plant *plant);
    virtual void initTrigger() override;
private:
    void fireStar(int targetRow, qreal bulletX);
};

// ==================== 双胞向日葵（TwinSunflower） ====================
class TwinSunflower : public SunFlower
{
    Q_DECLARE_TR_FUNCTIONS(TwinSunflower)
public:
    TwinSunflower();
    virtual bool canGrow(int x, int y) const override;
};

class TwinSunflowerInstance : public SunFlowerInstance
{
public:
    TwinSunflowerInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
};

// ==================== 魅惑菇（Hypno-shroom） ====================
class HypnoShroom : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(HypnoShroom)
public:
    HypnoShroom();
};

class HypnoShroomInstance : public PlantInstance
{
public:
    HypnoShroomInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
    virtual void wakeUp() override;  // 咖啡豆唤醒
    virtual void onDayNightChanged(bool isNight) override;

private:
    QString sleepGif;
    bool isDaytime();
};

// ==================== 土豆地雷（PotatoMine） ====================
// 25阳光、15秒冷却，仅陆地。14秒埋地后就绪，僵尸踏入本格触发1×1高额爆炸
class PotatoMine : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(PotatoMine)
public:
    PotatoMine();
    virtual bool canGrow(int x, int y) const override;
};

class PotatoMineInstance : public PlantInstance
{
public:
    PotatoMineInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombie, Trigger *trigger) override;
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
private:
    bool armed;       // 是否已就绪（14秒后）
    bool exploding;   // 正在爆炸中
    void doExplosion();
};

// ==================== 荷叶（LilyPad） ====================
class LilyPad : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(LilyPad)
public:
    LilyPad();
    virtual bool canGrow(int x, int y) const override;
};

class LilyPadInstance : public PlantInstance
{
public:
    LilyPadInstance(const Plant *plant);
};

// ==================== 花盆（FlowerPot） ====================
// 屋顶关卡承载基座，25阳光，仅屋顶(LF=3)种植。被僵尸啃毁时上方植物同步销毁
class FlowerPot : public Plant
{
    Q_DECLARE_TR_FUNCTIONS(FlowerPot)
public:
    FlowerPot();
    virtual bool canGrow(int x, int y) const override;
};

class FlowerPotInstance : public PlantInstance
{
public:
    FlowerPotInstance(const Plant *plant);
    virtual void getHurt(ZombieInstance *zombie, int aKind, int attack) override;
};

// ==================== 豌豆子弹（Bullet） ====================
class Bullet
{
public:
    Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue,  int direction, qreal speed = 5.0);
    ~Bullet();
    void start();
private:
    void move();
    void doSplashDamage();
    void tryTorchwoodConvert();

    GameScene *scene;
    int count, type, row, direction;
    qreal from;
    qreal speed; // bullet speed (default 5.0, left-facing uses 10.0)
    bool isFire;
    QGraphicsPixmapItem *picture;
};

Plant *PlantFactory(GameScene *scene, const QString &eName);
PlantInstance *PlantInstanceFactory(const Plant *plant);

#endif //PLANTS_VS_ZOMBIES_PLANT_H
