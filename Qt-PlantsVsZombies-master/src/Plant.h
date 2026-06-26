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

    bool contains(const QPointF &pos);

    const Plant *plantProtoType;

    QUuid uuid;
    int row, col;
    int hp;
    bool canTrigger;
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
protected:
    QString sleepGif;
    bool isDaytime();
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
private:
    QString lightedGif;
};

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

class PotatoMine: public Plant
{
    Q_DECLARE_TR_FUNCTIONS(PotatoMine)
public:
    PotatoMine();
};

class PotatoMineInstance: public PlantInstance
{
public:
    PotatoMineInstance(const Plant *plant);
    virtual void birth(int c, int r) override;
    virtual void initTrigger() override;
    virtual void triggerCheck(ZombieInstance *zombieInstance, Trigger *trigger) override;
    virtual void normalAttack(ZombieInstance *zombieInstance) override;
private:
    QString notReadyGif, mashGif, explosionGif;
    bool isArmed;
    bool exploded;
};

class Bullet
{
public:
    Bullet(GameScene *scene, int type, int row, qreal from, qreal x, qreal y, qreal zvalue,  int direction);
    ~Bullet();
    void start();
private:
    void move();

    GameScene *scene;
    int count, type, row, direction;
    qreal from;
    QGraphicsPixmapItem *picture;
};

Plant *PlantFactory(GameScene *scene, const QString &eName);
PlantInstance *PlantInstanceFactory(const Plant *plant);

#endif //PLANTS_VS_ZOMBIES_PLANT_H
