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
    virtual void getPea(int attack, int direction, int type);
    virtual void getHit(int attack);
    virtual void autoReduceHp();
    virtual void normalDie();
    virtual void boomDie();
    virtual void ashDie();
    virtual void playNormalballAudio();

    // 共享音频播放器池（避免每个僵尸实例创建独立的QMediaPlayer）
    static QMediaPlayer *getSharedAudioPlayer();

        // ===== 魅惑相关（用于魅惑菇） =====
    bool isHypnotized;                    // 是否被魅惑（友方）
    QUuid attackTargetZombieUuid;        // 普通僵尸攻击被魅惑僵尸的目标 UUID
    QUuid hypnotizedTargetUuid;          // 被魅惑僵尸攻击普通僵尸的目标 UUID
    int hypnotizedAttackTick;            // 被魅惑僵尸攻击计数（用于每秒伤害）
    void hypnotize();                    // 被魅惑时调用


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

    // ===== 水域僵尸入水逻辑（DuckyTube1/2/3 使用） =====
    bool enteredWater;           // 是否已完成入水（默认 false，仅水域僵尸使用）
    QString landNormalGif;       // 入水前陆地行走 GIF（如 Walk1.gif）
    QString waterNormalGif;      // 水中行走 GIF（如 Walk2.gif）
    QString landAttackGif;       // 入水前陆地攻击 GIF（如 Attack.gif，与水中通用）
    void triggerWaterEntry();    // 触发入水：水花 + 音效 + 切换水中 GIF
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
    QGraphicsColorizeEffect *ornDamageEffect;  // 复用饰品损伤染色效果
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
    virtual void getHit(int attack) override;
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
    virtual void birth(int row) override;  // 出场动画
    virtual void checkActs() override;
    virtual void playNormalballAudio() override;
    virtual void crushDie() override;   // 持杆时免疫大嘴花吞噬
private:
    bool hasPole;          // 是否持有撑杆
    bool jumping;          // 正在跳跃中
    int poleWalkFrame;     // 持杆行走帧 (0/1 切换)
    int poleWalkTimer;     // 行走动画切换计时
    void updatePoleWalk(); // 持杆行走动画循环
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
    virtual void birth(int row) override;
    virtual void checkActs() override;
    virtual void getHit(int attack) override;
    virtual void getPea(int attack, int direction, int type) override;
    virtual void applySlow(qreal multiplier, int durationMs) override;
    virtual void playNormalballAudio() override;
private:
    bool isAngry;           // 报纸损毁后暴怒状态
    int walkFrame;          // 行走动画帧索引 (0/1 切换)
    int walkTimer;          // 行走动画切换计时
    void updateWalkAnim();  // 行走动画循环（根据状态切换不同GIF组）
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
    virtual void getHit(int attack) override;
    virtual void playNormalballAudio() override;
private:
    bool helmetLost;  // 头盔是否已掉落，用于触发冲锋加速
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
    int explosionFrames; // 随机自爆倒计时帧数
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
    bool isAnyBackupAttacking();
    int walkDistance;   // 已行走距离（像素）
    int danceTimer;     // 舞蹈周期计时（帧）
    int replenishCooldown; // 补充伴舞冷却帧数
    QList<QUuid> backupDancerUuids;
    bool hasSummoned;
    bool isDancingPhase; // 当前是否在原地跳舞阶段
    static const int DANCE_FORWARD_FRAMES = 144;  // 前进阶段帧数（2.4s @ 60fps）
    static const int DANCE_STILL_FRAMES = 132;    // 原地跳舞帧数（2.2s @ 60fps）
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
    virtual void birth(int row) override;
};

// ---------- 潜水僵尸 ----------
class SnorkelZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(SnorkelZombie)
public:
    SnorkelZombie();
    virtual bool canPass(int row) const override;
};

class SnorkelZombieInstance: public ZombieInstance
{
public:
    SnorkelZombieInstance(const Zombie *zombie);
    virtual void birth(int row) override;
    virtual void checkActs() override;
    virtual void judgeAttack() override;
    virtual void getPea(int attack, int direction, int type) override;
private:
    bool submerged;         // 当前是否潜水
    bool transitioning;     // 正在播放浮出/潜水过渡动画
    bool jumping;           // 正在跳跃翻越南瓜头
    int visCheckTimer;
    void updateVisibility();
    void tryPumpkinJump();  // 检测南瓜头并触发翻越
};

// ---------- 海豚骑士僵尸 ----------
class DolphinRiderZombie: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(DolphinRiderZombie)
public:
    DolphinRiderZombie();
    virtual bool canPass(int row) const override;
};

class DolphinRiderZombieInstance: public ZombieInstance
{
public:
    DolphinRiderZombieInstance(const Zombie *zombie);
    virtual void birth(int row) override;
    virtual void checkActs() override;
    virtual void judgeAttack() override;
private:
    bool jumped;            // 已完成海豚跳跃（仅一次）
    bool jumpingPumpkin;    // 正在播放翻越南瓜头动画
    void tryPumpkinJump();  // 检测南瓜头并触发 Jump→Jump2→Jump3 翻越序列
};

// ---------- 小鬼僵尸 ----------
class Imp: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(Imp)
public:
    Imp();
};

class ImpInstance: public ZombieInstance
{
public:
    ImpInstance(const Zombie *zombie);
    virtual void getHit(int attack) override;
};

// ---------- 鸭子僵尸 ----------
class DuckyTubeZombie1: public Zombie1
{
    Q_DECLARE_TR_FUNCTIONS(DuckyTubeZombie1)
public:
    DuckyTubeZombie1();
    virtual bool canPass(int row) const override;
};

class DuckyTubeZombie1Instance: public ZombieInstance
{
public:
    DuckyTubeZombie1Instance(const Zombie *zombie);
    virtual void birth(int row) override;
    virtual void checkActs() override;
};

// ---------- 路障鸭子僵尸 ----------
class DuckyTubeZombie2: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(DuckyTubeZombie2)
public:
    DuckyTubeZombie2();
    virtual bool canPass(int row) const override;
};

class DuckyTubeZombie2Instance: public OrnZombieInstance1
{
public:
    DuckyTubeZombie2Instance(const Zombie *zombie);
    virtual void birth(int row) override;
    virtual void checkActs() override;
    virtual void playNormalballAudio() override;
};

// ---------- 铁桶鸭子僵尸 ----------
class DuckyTubeZombie3: public OrnZombie1
{
    Q_DECLARE_TR_FUNCTIONS(DuckyTubeZombie3)
public:
    DuckyTubeZombie3();
    virtual bool canPass(int row) const override;
};

class DuckyTubeZombie3Instance: public OrnZombieInstance1
{
public:
    DuckyTubeZombie3Instance(const Zombie *zombie);
    virtual void birth(int row) override;
    virtual void checkActs() override;
    virtual void playNormalballAudio() override;
};

// ---------- 冰车僵尸 ----------
// 免疫冰冻减速、碾压植物、生成永久冰道、Spikeweed互毁、爆炸摧毁周围植物
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
    void crushPlants();            // 碾压本格植物
    void updateWalkAnimation();    // 行走动画循环 0→1→2→3→4 / 受损→5
    int iceTrailTimer;
    int walkAnimIndex;             // 行走动画帧索引
    int walkAnimTimer;             // 行走动画切换计时
    bool damaged;                  // 是否进入受损状态
    QSet<QPair<int,int>> iceCells; // 已生成冰道的格子（静态共享，防止重复）
};

Zombie *ZombieFactory(GameScene *scene, const QString &ename);
ZombieInstance *ZombieInstanceFactory(const Zombie *zombie);

#endif //PLANTS_VS_ZOMBIES_ZOMBIE_H
