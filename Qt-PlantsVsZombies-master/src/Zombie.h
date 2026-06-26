//
// Created by sun on 8/26/16.
//

#ifndef PLANTS_VS_ZOMBIES_ZOMBIE_H
#define PLANTS_VS_ZOMBIES_ZOMBIE_H


#include <QtCore>
#include <QtWidgets>
#include <QtMultimedia>

class MoviePixmapItem;
class GameScene;
class PlantInstance;

class Zombie
{
    Q_DECLARE_TR_FUNCTIONS(Zombie)
public:
    Zombie();
    virtual ~Zombie() {}

    QString eName, cName;

    int width, height;

    int hp, level;
    qreal speed;
    int aKind, attack;
    bool canSelect, canDisplay;

    QString cardGif, staticGif, normalGif, attackGif, lostHeadGif,
            lostHeadAttackGif, headGif, dieGif, boomDieGif, standGif;
    // 受伤阶段1（如断臂）：当HP低于damagePoint1时触发
    QString damageGif1, damageAttackGif1;

    int beAttackedPointL, beAttackedPointR;
    int breakPoint, sunNum;
    int damagePoint1;  // 第一损伤阶段HP阈值（原版std1 = 2/3 maxHP）
    qreal coolTime;

    virtual bool canPass(int row) const;

    void update();

    GameScene *scene;
};

class Zombie1: public Zombie
{
    Q_DECLARE_TR_FUNCTIONS(Zombie1)
public:
    Zombie1();
};

class Zombie2: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(Zombie2)
public:
    Zombie2();
};

class Zombie3: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(Zombie3)
public:
    Zombie3();
};

class FlagZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(FlagZombie)
public:
    FlagZombie();
};

class ZombieInstance
{
public:
    ZombieInstance(const Zombie *zombie);
    virtual ~ZombieInstance();

    // 对僵尸施加减速：乘数（例如 0.5 表示减速到 50%），持续时间以毫秒为单位
    virtual void applySlow(qreal multiplier, int durationMs);

    virtual void birth(int row);
    virtual void checkActs();
    virtual void judgeAttack();
    virtual void normalAttack(PlantInstance *plant);
    virtual void crushDie();
    virtual void getPea(int attack, int direction);
    virtual void getHit(int attack);
    virtual void autoReduceHp();
    virtual void normalDie();
    virtual void boomDie();
    virtual void ashDie();
    virtual void playNormalballAudio();

    QUuid uuid;
    int hp;
    qreal speed;      // 当前实际移动速度（已应用所有减速）
    qreal baseSpeed;  // 基础速度（僵尸原始速度，用于计算减速）
    QList<qreal> slowMultipliers; // 活跃的减速乘数列表（相乘后作用于 baseSpeed）
    int altitude;
    bool beAttacked, isAttacking, goingDie;
    bool damageStage1; // 是否已进入第一损伤阶段（如断臂）

    qreal X, ZX;
    qreal attackedLX, attackedRX;
    int row;
    const Zombie *zombieProtoType;

    QString normalGif, attackGif;

    QGraphicsPixmapItem *shadowPNG;
    MoviePixmapItem *picture;
    QMediaPlayer *attackMusic, *hitMusic;
};

class OrnZombie1: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(OrnZombie1)
public:
    int ornHp;
    QString ornLostNormalGif, ornLostAttackGif;
};

class OrnZombieInstance1: public ZombieInstance
{
public:
    OrnZombieInstance1(const Zombie *zombie);
    const OrnZombie1 *getZombieProtoType();
    virtual void getHit(int attack);

    int ornHp;
    int originalOrnHp;  // 饰品原始HP，用于计算损伤阶段
    bool hasOrnaments;
};

class ConeheadZombie: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(ConeheadZombie)
public:
    ConeheadZombie();
};

class ConeheadZombieInstance: public OrnZombieInstance1
{
public:
    ConeheadZombieInstance(const Zombie *zombie);
    virtual void playNormalballAudio();

};

class BucketheadZombie: public ConeheadZombie
{
    Q_DECLARE_TR_FUNCTIONS(BucketheadZombie)
public:
    BucketheadZombie();
};

class BucketheadZombieInstance: public OrnZombieInstance1
{
public:
    BucketheadZombieInstance(const Zombie *zombie);
    virtual void playNormalballAudio();

};

class PoleVaultingZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(PoleVaultingZombie)
public:
    PoleVaultingZombie();
};

class PoleVaultingZombieInstance: public ZombieInstance
{
public:
    PoleVaultingZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
    virtual void playNormalballAudio() override;
private:
    bool hasPole;
    bool jumping;
    QMediaPlayer *poleVaultMusic;
};

class NewspaperZombie: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(NewspaperZombie)
public:
    NewspaperZombie();
};

class NewspaperZombieInstance: public OrnZombieInstance1
{
public:
    NewspaperZombieInstance(const Zombie *zombie);
    virtual void getHit(int attack) override;
    virtual void playNormalballAudio() override;
    virtual void birth(int row) override;
private:
    bool isAngry;
    QMediaPlayer *angryMusic;
};

class FootballZombie: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(FootballZombie)
public:
    FootballZombie();
};

class FootballZombieInstance: public OrnZombieInstance1
{
public:
    FootballZombieInstance(const Zombie *zombie);
    virtual void playNormalballAudio() override;
};

// ---------- 铁网门僵尸 ----------
class ScreenDoorZombie: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(ScreenDoorZombie)
public:
    ScreenDoorZombie();
};

class ScreenDoorZombieInstance: public OrnZombieInstance1
{
public:
    ScreenDoorZombieInstance(const Zombie *zombie);
    virtual void playNormalballAudio() override;
};

// ---------- 小丑僵尸 ----------
class JackinTheBoxZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(JackinTheBoxZombie)
public:
    JackinTheBoxZombie();
};

class JackinTheBoxZombieInstance: public ZombieInstance
{
public:
    JackinTheBoxZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
private:
    bool exploded;
    int walkTicks;
};

// ---------- 舞王僵尸 ----------
class DancingZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(DancingZombie)
public:
    DancingZombie();
};

class DancingZombieInstance: public ZombieInstance
{
public:
    DancingZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
    virtual void normalDie() override;
private:
    void spawnAllBackupDancers();
    int walkDistance;   // 已行走距离（像素）
    int danceTimer;     // 跳舞计时
    int replenishCooldown; // 补充伴舞冷却帧数
    QList<QUuid> backupDancerUuids;
    bool hasSummoned;
};

// ---------- 伴舞僵尸 ----------
class BackupDancer: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(BackupDancer)
public:
    BackupDancer();
};

class BackupDancerInstance: public ZombieInstance
{
public:
    BackupDancerInstance(const Zombie *zombie);
};

// ---------- 潜水僵尸 ----------
class SnorkelZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(SnorkelZombie)
public:
    SnorkelZombie();
};

class SnorkelZombieInstance: public ZombieInstance
{
public:
    SnorkelZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
    virtual void getHit(int attack) override;
private:
    bool submerged;
    int visCheckTimer;
    void updateVisibility();
};

// ---------- 海豚骑士僵尸 ----------
class DolphinRiderZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(DolphinRiderZombie)
public:
    DolphinRiderZombie();
};

class DolphinRiderZombieInstance: public ZombieInstance
{
public:
    DolphinRiderZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
private:
    bool jumped;
    int jumpTimer;
};

// ---------- 冰车僵尸 ----------
class ZomboniZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(ZomboniZombie)
public:
    ZomboniZombie();
};

class ZomboniZombieInstance: public ZombieInstance
{
public:
    ZomboniZombieInstance(const Zombie *zombie);
    virtual void checkActs() override;
    virtual void applySlow(qreal multiplier, int durationMs) override;
    virtual void boomDie() override;
    virtual void ashDie() override;
private:
    void leaveIceTrail();
    int iceTrailTimer;
};

Zombie *ZombieFactory(GameScene *scene, const QString &ename);
ZombieInstance *ZombieInstanceFactory(const Zombie *zombie);

#endif //PLANTS_VS_ZOMBIES_ZOMBIE_H
