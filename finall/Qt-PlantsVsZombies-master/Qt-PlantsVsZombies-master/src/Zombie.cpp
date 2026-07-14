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
    : hp(270), level(1), speed(0.24),  // 1.5px/100ms → 0.24px/16ms (60fps)
      aKind(0), attack(100),
      canSelect(true), canDisplay(true),
      beAttackedPointL(82), beAttackedPointR(156),
      breakPoint(90), sunNum(0),
      damagePoint1(0),  // 0 = 不启用第一损伤阶段
      coolTime(0)
{}

bool Zombie::canPass(int row) const
{
    // 陆地僵尸可在陆地(LF=1)和屋顶(LF=3)通行
    int groundType = scene->getGameLevelData()->LF[row];
    return groundType == 1 || groundType == 3;
}

void Zombie::update()
{
    // 使用 normalGif（实际播放的动画）的尺寸，而非 staticGif（静态图），
    // 确保阴影位置和Y坐标计算与实际渲染内容一致
    QPixmap pic = gImageCache->load(normalGif);
    width = pic.width();
    height = pic.height();
}

Zombie1::Zombie1()
{
    eName = "oZombie";
    cName = tr("Zombie");
    damagePoint1 = hp * 2 / 3;  // std1 = 180
    breakPoint = hp / 3;  // std2 = 90
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
    speed = 0.352;  // 2.2px/100ms → 0.352px/16ms (60fps)
    beAttackedPointR = 160;
    breakPoint = hp / 3;  // std2 = 90
    QString path = "Zombies/FlagZombie/";
    cardGif = "Card/Zombies/FlagZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "FlagZombie.gif";
    attackGif = path + "FlagZombieAttack.gif";
    lostHeadGif = path + "FlagZombieLostHead.gif";
    lostHeadAttackGif = path + "FlagZombieLostHeadAttack.gif";
    headGif = path + "FlagZombieHead.gif";
    dieGif = path + "ZombieDie.gif";  // 断头死亡动画（与普通僵尸共用）
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

// 共享音频播放器池：避免每个僵尸实例创建独立的QMediaPlayer
static QList<QMediaPlayer*> s_audioPool;
static int s_audioPoolIndex = 0;
static QObject *s_audioPoolParent = nullptr;

QMediaPlayer *ZombieInstance::getSharedAudioPlayer()
{
    // 初始化池（懒加载）
    if (!s_audioPoolParent) {
        s_audioPoolParent = new QObject;  // 全局父对象，应用退出时自动清理
        for (int i = 0; i < 4; ++i) {
            s_audioPool.append(new QMediaPlayer(s_audioPoolParent));
        }
    }
    // 轮询获取播放器
    QMediaPlayer *player = s_audioPool[s_audioPoolIndex];
    s_audioPoolIndex = (s_audioPoolIndex + 1) % s_audioPool.size();
    return player;
}

ZombieInstance::ZombieInstance(const Zombie *zombie)
    : zombieProtoType(zombie), picture(new MoviePixmapItem)
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

    isHypnotized = false;
    attackTargetZombieUuid = QUuid();
    hypnotizedTargetUuid = QUuid();
    hypnotizedAttackTick = 0;

    // 水域僵尸入水逻辑默认关闭（仅 DuckyTube1/2/3 子类启用）
    enteredWater = true;
    landNormalGif.clear();
    waterNormalGif.clear();
    landAttackGif.clear();
}

// 触发入水：水花 + 音效 + 切换水中 GIF（由 DuckyTube1/2/3 的 checkActs 调用）
void ZombieInstance::triggerWaterEntry()
{
    enteredWater = true;

    // 播放入水音效
    QMediaPlayer *player = getSharedAudioPlayer();
    player->stop();
    player->setMedia(QUrl("qrc:/audio/zombie_entering_water.mp3"));
    player->play();

    // 水花动画：splash.png 是 776x88 精灵图，含 8 帧（每帧 97x88）
    // 逐帧播放单帧，避免整图显示导致出现多个水花团
    QPixmap fullSplash = gImageCache->load("interface/splash.png");
    const int splashFrames = 8;
    const int frameW = fullSplash.width() / splashFrames;  // 97
    const int frameH = fullSplash.height();                 // 88

    QGraphicsPixmapItem *splash = new QGraphicsPixmapItem;
    splash->setPixmap(fullSplash.copy(0, 0, frameW, frameH));
    qreal splashX = picture->x() + zombieProtoType->width * 0.5 - frameW * 0.5;
    qreal splashY = picture->y() + zombieProtoType->height - frameH * 0.5;
    splash->setPos(splashX, splashY);
    splash->setZValue(picture->zValue() + 2);
    zombieProtoType->scene->addToGame(splash);

    // 逐帧动画：每 80ms 切换一帧，共 8 帧（640ms），结束后自动清除
    QTimer *splashTimer = new QTimer(picture);
    QSharedPointer<int> frame(new int(0));
    QObject::connect(splashTimer, &QTimer::timeout, [splash, fullSplash, frameW, frameH, frame, splashTimer]() {
        if (gPaused) return;
        (*frame)++;
        if (*frame >= 8) {
            splashTimer->stop();
            if (splash->scene()) splash->scene()->removeItem(splash);
            delete splash;
            splashTimer->deleteLater();
            return;
        }
        splash->setPixmap(fullSplash.copy(*frame * frameW, 0, frameW, frameH));
    });
    splashTimer->start(80);

    // 切换水中行走 GIF（Walk2.gif），攻击/死亡 GIF 保持通用（Attack.gif / Die.gif）
    normalGif = waterNormalGif;
    // attackGif 保持陆地攻击 GIF（Attack.gif 通用，无需切换）
    // 若 OrnZombie 饰品已掉落，则保持 ornLostNormalGif（水中受损姿态）
    OrnZombieInstance1 *ornInstance = dynamic_cast<OrnZombieInstance1 *>(this);
    if (ornInstance && !ornInstance->hasOrnaments) {
        normalGif = ornInstance->getZombieProtoType()->ornLostNormalGif;
        attackGif = ornInstance->getZombieProtoType()->ornLostAttackGif;
    }
    picture->setMovie(normalGif);
    picture->start();
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
    shadowPNG->setPos(zombieProtoType->width * 0.5 - 48, zombieProtoType->height - 22);
    shadowPNG->setFlag(QGraphicsItem::ItemStacksBehindParent);
    shadowPNG->setParentItem(picture);
    picture->start();
    zombieProtoType->scene->addToGame(picture);
}
//魅惑菇修改
void ZombieInstance::checkActs()
{
    if (hp < 1) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;   // speed 为负，实际向右
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();  // 攻击其他僵尸
        return;
    }

    // ---- 原有逻辑：正常向左移动 ----
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
// 魅惑菇修改部分
void ZombieInstance::judgeAttack()
{
    // ---- 被魅惑的僵尸：攻击其他未魅惑的僵尸 ----
    if (isHypnotized) {
        QList<ZombieInstance *> zombies = zombieProtoType->scene->getZombieOnRow(row);
        ZombieInstance *target = nullptr;
        for (auto *z : zombies) {
            if (z == this || z->isHypnotized || z->goingDie || z->hp <= 0 || !z->beAttacked) continue;
            if (qAbs(z->attackedLX - attackedLX) < 50 && z->attackedLX > attackedLX) {
                target = z;
                break;
            }
        }

        if (target) {
            if (!isAttacking) {
                isAttacking = true;
                picture->setMovie(attackGif);
                picture->start();
            }
            // 攻击循环（每秒造成100伤害）
            if (hypnotizedTargetUuid.isNull()) {
                hypnotizedTargetUuid = target->uuid;
                QUuid myUuid = uuid;
                QUuid targetUuid = target->uuid;
                QSharedPointer<std::function<void()>> attackLoop = QSharedPointer<std::function<void()>>::create();
                *attackLoop = [this, myUuid, targetUuid, attackLoop] {
                    ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
                    if (!self || self != this || goingDie) {
                        hypnotizedTargetUuid = QUuid();
                        isAttacking = false;
                        picture->setMovie(normalGif);
                        picture->start();
                        return;
                    }
                    ZombieInstance *t = zombieProtoType->scene->getZombie(targetUuid);
                    if (t && !t->goingDie && t->hp > 0 && t->beAttacked &&
                        qAbs(t->attackedLX - attackedLX) < 50 && t->attackedLX > attackedLX) {
                        t->getHit(100);
                        if (t->hp <= 0 || t->goingDie) {
                            hypnotizedTargetUuid = QUuid();
                            isAttacking = false;
                            picture->setMovie(normalGif);
                            picture->start();
                            return;
                        }
                        (new Timer(picture, 1000, *attackLoop))->start();
                    } else {
                        hypnotizedTargetUuid = QUuid();
                        isAttacking = false;
                        picture->setMovie(normalGif);
                        picture->start();
                    }
                };
                (*attackLoop)(); // 立即执行第一次攻击
            }
            return;
        } else {
            if (!hypnotizedTargetUuid.isNull()) {
                hypnotizedTargetUuid = QUuid();
                isAttacking = false;
                picture->setMovie(normalGif);
                picture->start();
            }
            return;
        }
    }

    // ---- 普通僵尸：优先攻击被魅惑的僵尸 ----
    QList<ZombieInstance *> zombies = zombieProtoType->scene->getZombieOnRow(row);
    ZombieInstance *hypnoTarget = nullptr;
    for (auto *z : zombies) {
        if (z != this && z->isHypnotized && !z->goingDie && z->hp > 0 && z->beAttacked) {
            if (qAbs(z->attackedLX - attackedLX) < 50) {
                hypnoTarget = z;
                break;
            }
        }
    }

    if (hypnoTarget) {
        if (!isAttacking) {
            isAttacking = true;
            picture->setMovie(attackGif);
            picture->start();
        }
        if (attackTargetZombieUuid.isNull()) {
            attackTargetZombieUuid = hypnoTarget->uuid;
            QUuid myUuid = uuid;
            QUuid targetUuid = hypnoTarget->uuid;
            QSharedPointer<std::function<void()>> attackLoop = QSharedPointer<std::function<void()>>::create();
            *attackLoop = [this, myUuid, targetUuid, attackLoop] {
                ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
                if (!self || self != this || goingDie) {
                    attackTargetZombieUuid = QUuid();
                    isAttacking = false;
                    picture->setMovie(normalGif);
                    picture->start();
                    return;
                }
                ZombieInstance *target = zombieProtoType->scene->getZombie(targetUuid);
                if (target && !target->goingDie && target->hp > 0 && target->beAttacked &&
                    qAbs(target->attackedLX - attackedLX) < 50) {
                    target->getHit(zombieProtoType->attack);
                    if (target->hp <= 0 || target->goingDie) {
                        attackTargetZombieUuid = QUuid();
                        isAttacking = false;
                        picture->setMovie(normalGif);
                        picture->start();
                        return;
                    }
                    (new Timer(picture, 1000, *attackLoop))->start();
                } else {
                    attackTargetZombieUuid = QUuid();
                    isAttacking = false;
                    picture->setMovie(normalGif);
                    picture->start();
                }
            };
            (*attackLoop)();
        }
        return;
    } else {
        if (!attackTargetZombieUuid.isNull()) {
            attackTargetZombieUuid = QUuid();
            isAttacking = false;
            picture->setMovie(normalGif);
            picture->start();
        }
    }

    // ---- 原有逻辑：攻击植物（仅当没有被魅惑僵尸时） ----
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
        if (isAttacking) {
            picture->setMovie(attackGif);
        } else {
            picture->setMovie(normalGif);
        }
        picture->start();
    }
    if (isAttacking)
        normalAttack(plant);
}


void ZombieInstance::normalAttack(PlantInstance *plantInstance)
{
    //魅惑菇添加：清除魅惑攻击目标
    hypnotizedTargetUuid = QUuid();
    attackTargetZombieUuid = QUuid();

    QMediaPlayer *player = getSharedAudioPlayer();
    if (qrand() % 2)
        player->setMedia(QUrl("qrc:/audio/chomp.mp3"));
    else
        player->setMedia(QUrl("qrc:/audio/chompsoft.mp3"));
    player->play();
    QUuid myUuid = uuid;
    (new Timer(this->picture, 500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        QMediaPlayer *p = getSharedAudioPlayer();
        if (qrand() % 2)
            p->setMedia(QUrl("qrc:/audio/chomp.mp3"));
        else
            p->setMedia(QUrl("qrc:/audio/chompsoft.mp3"));
        p->play();
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

//魅惑菇部分
void ZombieInstance::hypnotize()
{
    if (goingDie || isHypnotized) return;
    isHypnotized = true;

    // 掉头：速度取反（原本正数向左，负数向右）
    speed = -baseSpeed;

    // 保持 beAttacked = true（允许其他僵尸攻击它）

    // 如果正在攻击植物，停止攻击动画
    if (isAttacking) {
        isAttacking = false;
        picture->setMovie(normalGif);
        picture->start();
    }
}

void ZombieInstance::crushDie()
{
    //魅惑菇添加
    hypnotizedTargetUuid = QUuid();
    attackTargetZombieUuid = QUuid();
    isAttacking = false;
    //


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

void ZombieInstance::getPea(int attack, int direction, int type)
{
    Q_UNUSED(direction);
    Q_UNUSED(type);
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
        qreal oldH = picture->boundingRect().height();
        if (isAttacking)
            picture->setMovie(zombieProtoType->lostHeadAttackGif);
        else
            picture->setMovie(zombieProtoType->lostHeadGif);
        picture->start();
        qreal newH = picture->boundingRect().height();
        if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
            picture->setY(picture->y() + oldH - newH);
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
            qreal oldH = picture->boundingRect().height();
            if (isAttacking)
                picture->setMovie(zombieProtoType->damageAttackGif1);
            else
                picture->setMovie(zombieProtoType->damageGif1);
            picture->start();
            qreal newH = picture->boundingRect().height();
            if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                picture->setY(picture->y() + oldH - newH);
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
            // 粒子飞散动画：使用单个循环定时器替代多个一次性定时器
            qreal angle = (qrand() % 360) * M_PI / 180.0;
            qreal dist = 20 + qrand() % 30;
            QPointF target = center + QPointF(cos(angle) * dist, sin(angle) * dist - 15);
            int steps = 8;
            int stepDelay = 60;
            QTimer *particleTimer = new QTimer(zombieProtoType->scene);
            QSharedPointer<int> step(new int(0));
            QObject::connect(particleTimer, &QTimer::timeout, [particleTimer, particle, center, target, steps, step] {
                if (gPaused) return;
                (*step)++;
                qreal progress = (qreal)(*step) / steps;
                particle->setPos(center + (target - center) * progress);
                particle->setOpacity(1.0 - progress);
                if (*step >= steps) {
                    particleTimer->stop();
                    if (particle->scene())
                        particle->scene()->removeItem(particle);
                    delete particle;
                    particleTimer->deleteLater();
                }
            });
            particleTimer->start(stepDelay);
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
    //魅惑菇添加
    hypnotizedTargetUuid = QUuid();
    attackTargetZombieUuid = QUuid();
    isAttacking = false;
    //

    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    qreal oldH = picture->boundingRect().height();
    picture->setMovie(zombieProtoType->dieGif);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);
    QUuid myUuid = uuid;
    // 播放死亡动画2秒后直接移除，无淡化
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::boomDie()
{
    //魅惑菇添加
    hypnotizedTargetUuid = QUuid();
    attackTargetZombieUuid = QUuid();
    isAttacking = false;
    //


    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    beAttacked = false;
    if (shadowPNG)
        shadowPNG->setPixmap(QPixmap());
    QString boomGif = zombieProtoType->boomDieGif.isEmpty() ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    qreal oldH = picture->boundingRect().height();
    picture->setMovie(boomGif);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);
    QUuid myUuid = uuid;
    // 播放爆炸动画1500ms后直接移除
    (new Timer(picture, 1500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::ashDie()
{
    //魅惑菇添加
    hypnotizedTargetUuid = QUuid();
    attackTargetZombieUuid = QUuid();
    isAttacking = false;
    //

    
    if (goingDie)
        return;
    goingDie = true;
    hp = 0;
    beAttacked = false;
    if (shadowPNG)
        shadowPNG->setPixmap(QPixmap());
    // 使用 BoomDie.gif 播放化为灰烬的动画
    QString boomGif = zombieProtoType->boomDieGif.isEmpty() ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    qreal oldH = picture->boundingRect().height();
    picture->setMovie(boomGif);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);
    QUuid myUuid = uuid;
    // 播放灰烬动画1500ms后直接移除
    (new Timer(picture, 1500, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (self && self == this)
            zombieProtoType->scene->zombieDie(this);
    }))->start();
}

void ZombieInstance::playNormalballAudio()
{
    QMediaPlayer *player = getSharedAudioPlayer();
    player->stop();
    switch (qrand() % 3) {
        case 0: player->setMedia(QUrl("qrc:/audio/splat1.mp3")); break;
        case 1: player->setMedia(QUrl("qrc:/audio/splat2.mp3")); break;
        default: player->setMedia(QUrl("qrc:/audio/splat3.mp3")); break;
    }
    player->play();
}


OrnZombieInstance1::OrnZombieInstance1(const Zombie *zombie)
    : ZombieInstance(zombie), ornDamageEffect(nullptr)
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
            // 记录切换前的高度，用于补偿Y坐标
            qreal oldH = picture->boundingRect().height();
            picture->setMovie(isAttacking ? attackGif : normalGif);
            picture->start();
            // 饰品GIF通常比无饰品GIF高，调整Y坐标保持僵尸脚部位置不变
            qreal newH = picture->boundingRect().height();
            if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                picture->setY(picture->y() + oldH - newH);
        } else {
            // 饰品损伤阶段：根据剩余HP比例应用染色效果（复用同一effect对象）
            // 原版：损伤点1 = 2/3, 损伤点2 = 1/3
            int ornDmg1 = originalOrnHp * 2 / 3;  // 轻微损伤
            int ornDmg2 = originalOrnHp * 1 / 3;  // 严重损伤
            if (!ornDamageEffect) {
                ornDamageEffect = new QGraphicsColorizeEffect;
                ornDamageEffect->setEnabled(false);
            }
            if (ornHp < ornDmg2) {
                // 严重损伤：深色/金属色
                ornDamageEffect->setColor(QColor(100, 90, 80));
                ornDamageEffect->setStrength(0.5);
                ornDamageEffect->setEnabled(true);
            } else if (ornHp < ornDmg1) {
                // 轻微损伤：浅色
                ornDamageEffect->setColor(QColor(140, 130, 110));
                ornDamageEffect->setStrength(0.25);
                ornDamageEffect->setEnabled(true);
            } else {
                ornDamageEffect->setEnabled(false);
            }
            picture->setGraphicsEffect(ornDamageEffect);
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
    breakPoint = hp / 3;  // std2 = 90
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
    dieGif = "Zombies/Zombie/ZombieDie.gif";  // 断头死亡动画
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

ConeheadZombieInstance::ConeheadZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void ConeheadZombieInstance::getHit(int attack)
{
    if (hasOrnaments) {
        ornHp -= attack;
        if (ornHp < 1) {
            hp += ornHp;
            hasOrnaments = false;
            picture->setGraphicsEffect(nullptr);
            normalGif = getZombieProtoType()->ornLostNormalGif;
            attackGif = getZombieProtoType()->ornLostAttackGif;
            qreal oldH = picture->boundingRect().height();
            picture->setMovie(isAttacking ? attackGif : normalGif);
            picture->start();
            qreal newH = picture->boundingRect().height();
            if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                picture->setY(picture->y() + oldH - newH);
        } else {
            // 原版路障损伤阶段阈值：226（轻微损伤）、113（严重损伤）
            int ornDmg1 = 226;  // 原版精确值，非 2/3 公式
            int ornDmg2 = 113;  // 原版精确值，非 1/3 公式
            if (!ornDamageEffect) {
                ornDamageEffect = new QGraphicsColorizeEffect;
                ornDamageEffect->setEnabled(false);
            }
            if (ornHp < ornDmg2) {
                ornDamageEffect->setColor(QColor(100, 90, 80));
                ornDamageEffect->setStrength(0.5);
                ornDamageEffect->setEnabled(true);
            } else if (ornHp < ornDmg1) {
                ornDamageEffect->setColor(QColor(140, 130, 110));
                ornDamageEffect->setStrength(0.25);
                ornDamageEffect->setEnabled(true);
            } else {
                ornDamageEffect->setEnabled(false);
            }
            picture->setGraphicsEffect(ornDamageEffect);
        }
        picture->setOpacity(0.5);
        (new Timer(picture, 100, [this] {
            picture->setOpacity(1);
        }))->start();
    }
    else
        ZombieInstance::getHit(attack);
}

void ConeheadZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        player->setMedia(QUrl("qrc:/audio/plastichit.mp3"));
        player->play();
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
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        if (qrand() % 2)
            player->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            player->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        player->play();
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
    breakPoint = hp / 3;  // std2 = 90
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
    dieGif = "Zombies/Zombie/ZombieDie.gif";  // 断头死亡动画
    standGif = path + "1.gif";
    boomDieGif = path + "BoomDie.gif";
}

PoleVaultingZombie::PoleVaultingZombie()
{
    eName = "oPoleVaultingZombie";
    cName = tr("撑杆僵尸");
    hp = 340;
    damagePoint1 = 170;         // 半血外观破损
    damageGif1 = "Zombies/PoleVaultingZombie/1.gif";  // 破损外观
    speed = 0.512;              // 持杆快速（3.2px/100ms）
    beAttackedPointL = 215;
    beAttackedPointR = 260;
    level = 2;
    sunNum = 75;
    breakPoint = hp / 3;        // 113 → 断头状态
    QString path = "Zombies/PoleVaultingZombie/";
    cardGif = "Card/Zombies/PoleVaultingZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "PoleVaultingZombieWalk.gif";       // 丢杆后普通行走
    attackGif = path + "PoleVaultingZombieAttack.gif";     // 攻击动画
    lostHeadGif = path + "PoleVaultingZombieLostHeadWalk.gif";      // 断头行走
    lostHeadAttackGif = path + "PoleVaultingZombieLostHeadAttack.gif"; // 断头攻击
    headGif = path + "PoleVaultingZombieHead.gif";         // 掉落头部
    dieGif = path + "PoleVaultingZombieDie.gif";           // 死亡动画
    boomDieGif = path + "BoomDie.gif";                     // 爆炸死亡
    standGif = path + "1.gif";
}

PoleVaultingZombieInstance::PoleVaultingZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), hasPole(true), jumping(false),
      poleWalkFrame(0), poleWalkTimer(0),
      poleVaultMusic(new QMediaPlayer(picture))
{
    // 持杆时行走动画用 0.gif（覆盖原型 normalGif）
    this->normalGif = "Zombies/PoleVaultingZombie/0.gif";
    this->attackGif = zombie->attackGif;
    poleVaultMusic->setMedia(QUrl("qrc:/audio/polevault.mp3"));
}

void PoleVaultingZombieInstance::birth(int row)
{
    ZombieInstance::birth(row);
    // 始终使用 PoleVaultingZombie.gif 作为持杆行走动画
    normalGif = "Zombies/PoleVaultingZombie/PoleVaultingZombie.gif";
    picture->setMovie(normalGif);
    picture->start();
}

void PoleVaultingZombieInstance::updatePoleWalk()
{
    if (!hasPole || jumping || isAttacking || goingDie) return;
    poleWalkTimer++;
    if (poleWalkTimer >= 15) {  // 约每250ms切换帧
        poleWalkTimer = 0;
        poleWalkFrame = (poleWalkFrame + 1) % 2;
        QString path = "Zombies/PoleVaultingZombie/";
        normalGif = path + QString::number(poleWalkFrame) + ".gif";
        if (!isAttacking && !goingDie) {
            picture->setMovie(normalGif);
            picture->start();
        }
    }
}

void PoleVaultingZombieInstance::crushDie()
{
    // 持杆时不被大嘴花吞噬（撑杆卡住嘴巴）
    if (hasPole) return;
    ZombieInstance::crushDie();
}

void PoleVaultingZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();
        return;
    }

    // ---- 持杆行走动画循环 ----
    //updatePoleWalk();

    // ---- 撑杆跳跃 ----
    if (hasPole && !jumping) {
        int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
        if (col >= 1 && col <= 9) {
            auto plants = zombieProtoType->scene->getPlant(col, row);
            for (auto plant : plants.values()) {
                if (!plant->plantProtoType->canEat) continue;
                if (!(plant->attackedRX >= ZX && plant->attackedLX <= ZX)) continue;

                // 检查是否可跳跃的植物类型
                QString eName = plant->plantProtoType->eName;
                // 无法越过高坚果、地刺、地刺王
                if (eName == "oTallnut" || eName == "oSpikeweed" || eName == "oSpikerock")
                    break;  // 遇到这些植物不跳，转为普通啃食
                // 南瓜头：跳过南瓜直接攻击内部植物
                if (plant->plantProtoType->pKind == 2) continue; // 跳过南瓜头，看内部植物

                // 触发跳跃：依次播放 Jump.gif → Jump2.gif
                jumping = true;
                altitude = 2;  // 空中状态，免疫子弹/冰冻
                poleVaultMusic->stop();
                poleVaultMusic->play();

                QString path = "Zombies/PoleVaultingZombie/";
                qreal oldH = picture->boundingRect().height();
                picture->setMovie(path + "PoleVaultingZombieJump.gif");
                picture->start();
                qreal newH = picture->boundingRect().height();
                if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                    picture->setY(picture->y() + oldH - newH);

                // Jump.gif 播放中向前移动，然后衔接 Jump2.gif
                (new Timer(picture, 500, [this, path] {
                    // 跳跃前移约 1.5 格
                    ZX = attackedLX -= 130;
                    X -= 130;
                    attackedRX -= 130;
                    picture->setX(X);

                    qreal oldH2 = picture->boundingRect().height();
                    picture->setMovie(path + "PoleVaultingZombieJump2.gif");
                    picture->start();
                    qreal newH2 = picture->boundingRect().height();
                    if (oldH2 > 0 && newH2 > 0 && !qFuzzyCompare(oldH2, newH2))
                        picture->setY(picture->y() + oldH2 - newH2);

                    // Jump2.gif 播完后落地
                    (new Timer(picture, 500, [this] {
                        jumping = false;
                        hasPole = false;
                        altitude = 1;

                        this->normalGif = zombieProtoType->normalGif;
                        this->attackGif = zombieProtoType->attackGif;
                        qreal oldH3 = picture->boundingRect().height();
                        picture->setMovie(this->normalGif);
                        picture->start();
                        qreal newH3 = picture->boundingRect().height();
                        if (oldH3 > 0 && newH3 > 0 && !qFuzzyCompare(oldH3, newH3))
                            picture->setY(picture->y() + oldH3 - newH3);

                        speed = 0.24;
                        baseSpeed = 0.24;
                    }))->start();
                }))->start();
                return;
            }
        }
    }

    if (beAttacked && !isAttacking && !jumping) {
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
    hp = 270;                    // 本体血量
    ornHp = 150;                 // 报纸防具血量
    speed = 0.24;                // 持报阶段：普通速度
    level = 2;
    sunNum = 75;
    damagePoint1 = hp * 2 / 3;   // 180 → 第一损伤阶段
    breakPoint = hp / 3;         // 90 → 断头
    QString path = "Zombies/NewspaperZombie/";
    cardGif = "Card/Zombies/NewspaperZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "HeadWalk1.gif";             // 持报行走
    attackGif = path + "LostNewspaper.gif";        // 持报啃食
    damageGif1 = path + "HeadWalk0.gif";           // 持报受损行走
    damageAttackGif1 = path + "LostNewspaper.gif"; // 持报受损啃食
    ornLostNormalGif = path + "HeadWalk0.gif";     // 暴怒行走帧0
    ornLostAttackGif = path + "HeadAttack0.gif";   // 暴怒啃食帧0
    lostHeadGif = path + "LostHeadWalk0.gif";      // 无头行走帧0
    lostHeadAttackGif = path + "LostHeadAttack0.gif"; // 无头啃食帧0
    headGif = path + "Head.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

NewspaperZombieInstance::NewspaperZombieInstance(const Zombie *zombie)
    : OrnZombieInstance1(zombie), isAngry(false),
      walkFrame(0), walkTimer(0),
      angryMusic(new QMediaPlayer(picture))
{
    angryMusic->setMedia(QUrl("qrc:/audio/newspaper_rarrgh2.mp3"));
}

void NewspaperZombieInstance::birth(int row)
{
    OrnZombieInstance1::birth(row);
    // 出厂动画：HeadWalk1.gif 循环
    normalGif = "Zombies/NewspaperZombie/HeadWalk1.gif";
    attackGif = "Zombies/NewspaperZombie/LostNewspaper.gif";
    picture->setMovie(normalGif);
    picture->start();
}



void NewspaperZombieInstance::updateWalkAnim()
{
    if (goingDie) return;

    // 每帧都递增，即使攻击也更新帧（但攻击时只显示 attackGif）
    walkTimer++;
    // 将阈值从 12 减小到 6，加快切换（约 100ms 一帧）
    if (walkTimer < 6) return;
    walkTimer = 0;
    walkFrame = (walkFrame + 1) % 2;

    QString path = "Zombies/NewspaperZombie/";
if (hasOrnaments) {
    // 持报阶段：固定 HeadWalk1.gif，不循环切换
    normalGif = path + "HeadWalk1.gif";
    // 直接返回，不执行后续帧切换逻辑
    if (!isAttacking && !goingDie) {
        picture->setMovie(normalGif);
        picture->start();
    }
        return;
    } else if (isAngry && !damageStage1) {
        // 暴怒状态：行走动画循环 HeadWalk0/1
        normalGif = path + "HeadWalk" + QString::number(walkFrame) + ".gif";
        // 攻击动画也使用对应的 HeadAttack0/1（但实际播放时，攻击动作一般用单帧，这里保持同步更新）
        attackGif = path + "HeadAttack" + QString::number(walkFrame) + ".gif";
    } else {
        normalGif = path + "LostHeadWalk" + QString::number(walkFrame) + ".gif";
        attackGif = path + "LostHeadAttack" + QString::number(walkFrame) + ".gif";
    }

    // 只有非攻击且非死亡时才更新显示，否则显示攻击动画
    if (!isAttacking && !goingDie) {
        picture->setMovie(normalGif);
        picture->start();
    }
}

void NewspaperZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // 行走动画循环
    updateWalkAnim();

    ZombieInstance::checkActs();
}

void NewspaperZombieInstance::applySlow(qreal multiplier, int durationMs)
{
    // 持报阶段免疫冰冻/减速
    if (hasOrnaments) return;
    ZombieInstance::applySlow(multiplier, durationMs);
}

void NewspaperZombieInstance::getPea(int attack, int direction, int type)
{
    // 穿透攻击（type=3 毒气）和火球（attack>=40）直接命中本体，绕过报纸
    if (hasOrnaments && (type == 3 || attack >= 40)) {
        hp -= attack;
        playNormalballAudio();
        if (hp < zombieProtoType->breakPoint && !damageStage1)
            getHit(0);  // 触发断头判定
        if (hp < 1) normalDie();
        return;
    }
    ZombieInstance::getPea(attack, direction, type);
}

void NewspaperZombieInstance::getHit(int attack)
{
    if (hasOrnaments) {
        ornHp -= attack;
        if (ornHp < 1) {
            hp += ornHp;  // 溢出伤害转给本体
            hasOrnaments = false;
            picture->setGraphicsEffect(nullptr);

            if (!isAngry) {
                isAngry = true;
                // 失去报纸：播放撕报动画 LostNewspaper.gif，短暂停顿后暴怒
                QString path = "Zombies/NewspaperZombie/";
                qreal oldH = picture->boundingRect().height();
                picture->setMovie(path + "LostNewspaper.gif");
                picture->start();
                qreal newH = picture->boundingRect().height();
                if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                    picture->setY(picture->y() + oldH - newH);

                // 短暂停顿后进入暴怒状态
                (new Timer(picture, 600, [this, path] {
                    // 移速翻倍（约 2.6 倍 → 等同橄榄球僵尸速度）
                    baseSpeed *= 2.6;
                    speed = baseSpeed;
                    angryMusic->stop();
                    angryMusic->play();

                    walkFrame = 0;
                    walkTimer = 0;
                    normalGif = path + "HeadWalk0.gif";
                    attackGif = path + "HeadAttack0.gif";
                    if (!isAttacking && !goingDie) {
                        picture->setMovie(normalGif);
                        picture->start();
                    }
                }))->start();
                return;
            }
            // 已在暴怒状态
            normalGif = getZombieProtoType()->ornLostNormalGif;
            attackGif = getZombieProtoType()->ornLostAttackGif;
            qreal oldH = picture->boundingRect().height();
            picture->setMovie(isAttacking ? attackGif : normalGif);
            picture->start();
            qreal newH = picture->boundingRect().height();
            if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
                picture->setY(picture->y() + oldH - newH);
        } else {
            // 报纸受损阶段：颜色变化
            int ornDmg2 = originalOrnHp / 3;   // 50
            int ornDmg1 = originalOrnHp * 2 / 3; // 100
            if (!ornDamageEffect) {
                ornDamageEffect = new QGraphicsColorizeEffect;
                ornDamageEffect->setEnabled(false);
            }
            if (ornHp < ornDmg2) {
                ornDamageEffect->setColor(QColor(100, 90, 80));
                ornDamageEffect->setStrength(0.5);
                ornDamageEffect->setEnabled(true);
            } else if (ornHp < ornDmg1) {
                ornDamageEffect->setColor(QColor(140, 130, 110));
                ornDamageEffect->setStrength(0.25);
                ornDamageEffect->setEnabled(true);
            } else {
                ornDamageEffect->setEnabled(false);
            }
            picture->setGraphicsEffect(ornDamageEffect);
        }
        // 受击闪烁
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
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        player->setMedia(QUrl("qrc:/audio/plastichit.mp3"));
        player->play();
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
    speed = 0.4;  // 2.5px/100ms → 0.4px/16ms (60fps)
    level = 4;
    sunNum = 175;
    breakPoint = hp / 3;  // std2 = 90
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
    : OrnZombieInstance1(zombie), helmetLost(false)
{
}

void FootballZombieInstance::getHit(int attack)
{
    bool hadOrn = hasOrnaments;
    OrnZombieInstance1::getHit(attack);
    // 正版：头盔掉落瞬间触发冲锋加速（速度 ×1.4），模拟失去头盔后的爆发冲刺
    if (hadOrn && !hasOrnaments && !helmetLost) {
        helmetLost = true;
        baseSpeed *= 1.4;
        speed = baseSpeed;
    }
}

void FootballZombieInstance::playNormalballAudio()
{
    if (hasOrnaments) {
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        if (qrand() % 2)
            player->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            player->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        player->play();
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
    speed = 0.24;  // 1.5px/100ms → 0.24px/16ms (60fps)，原版速度与普通僵尸相同
    level = 3;
    sunNum = 125;
    breakPoint = hp / 3;  // std2 = 90
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
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        if (qrand() % 2)
            player->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            player->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        player->play();
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
    speed = 0.352;  // 2.2px/100ms → 0.352px/16ms (60fps)
    level = 3;
    sunNum = 100;
    beAttackedPointL = 80;
    beAttackedPointR = 160;
    breakPoint = hp / 3;  // 166
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
{
    // 原版：行走一段随机时间后自爆，不论是否靠近植物
    // 随机范围60~400帧（约1~6.7秒 @ 60fps）
    // 正版：自爆倒计时约 1~5 秒（60~300 帧 @ 60fps）
    explosionFrames = 60 + (qrand() % 241);
}

void JackinTheBoxZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;   // speed 为负，实际向右
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();  // 攻击其他僵尸
        return;
    }

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
        // 原版：随机时间后自爆，不论是否靠近植物
        walkTicks++;
        explosionFrames--;
        if (explosionFrames <= 0) {
            // 自爆触发：先播放开盒子动画，再爆炸
            exploded = true;
            picture->setMovie("Zombies/JackinTheBoxZombie/OpenBox.gif");
            picture->start();
            QUuid myUuid = uuid;
            (new Timer(picture, 800, [this, myUuid] {
                ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
                if (!self || self != this) return;
                picture->setMovie("Zombies/JackinTheBoxZombie/Boom.gif");
                picture->start();
                // 对周围植物造成伤害（3x3范围）
                QMediaPlayer *boomSound = new QMediaPlayer(picture);
                boomSound->setMedia(QUrl("qrc:/audio/cherrybomb.mp3"));
                boomSound->play();
                (new Timer(picture, 800, [this, myUuid] {
                    ZombieInstance *self2 = zombieProtoType->scene->getZombie(myUuid);
                    if (!self2 || self2 != this) return;
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
            }))->start();
            return;
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
    speed = 0.24;  // 1.5px/100ms → 0.24px/16ms (60fps)
    level = 4;
    sunNum = 150;
    beAttackedPointL = 80;
    beAttackedPointR = 160;
    breakPoint = hp / 3;  // 166
    QString path = "Zombies/DancingZombie/";
    cardGif = "Card/Zombies/DancingZombie.png";
    staticGif = path + "0.gif";
    // 初始使用太空滑步动画
    normalGif = path + "SlidingStep.gif";
    attackGif = path + "Attack.gif";
    // 损伤阶段使用不同的滑步动画
    damageGif1 = path + "DancingZombie1.gif";
    damageAttackGif1 = path + "Attack.gif";
    lostHeadGif = path + "LostHead.gif";
    lostHeadAttackGif = path + "LostHeadAttack.gif";
    headGif = path + "Head.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "0.gif";
}

DancingZombieInstance::DancingZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), walkDistance(0), danceTimer(0), replenishCooldown(0),
      hasSummoned(false), isDancingPhase(false)
{
    // 原版：初始太空滑步速度(约1.2s/格)，召唤后降速(约5.5s/格)
    speed = 1.04; // 初始快速前进 (6.5px/100ms → 1.04px/16ms)
    // 初始使用滑步动画
    this->normalGif = "Zombies/DancingZombie/SlidingStep.gif";
}

void DancingZombieInstance::spawnAllBackupDancers()
{
    if (goingDie) return;
    GameScene *scene = zombieProtoType->scene;
    Zombie *backupProto = scene->getZombieProtoType("oBackupDancer");
    if (!backupProto) return;

    // 显示聚光灯效果
    QGraphicsPixmapItem *spotlight = new QGraphicsPixmapItem(
        gImageCache->load("Zombies/DancingZombie/spotlight.png"));
    spotlight->setPos(picture->x() - 80, picture->y() - 120);
    spotlight->setZValue(picture->zValue() + 0.5);
    spotlight->setOpacity(0.6);
    scene->addToGame(spotlight);
    (new Timer(scene, 3000, [spotlight] {
        if (spotlight->scene())
            spotlight->scene()->removeItem(spotlight);
        delete spotlight;
    }))->start();

    // 多阶段召唤动画序列：Summon1 → Summon2 → Summon3 → Summon
    QUuid myUuid = uuid;
    auto spawnDancer = [this, scene, backupProto](int rowOffset, qreal xOffset) {
        int r = row + rowOffset;
        if (r < 1 || r > scene->getGameLevelData()->LF.size()) return;
        if (scene->getGameLevelData()->LF[r-1] != 1) return;
        ZombieInstance *backup = ZombieInstanceFactory(backupProto);
        backup->birth(r);
        backup->X = X + xOffset;
        backup->attackedLX = X + xOffset;
        backup->ZX = X + xOffset;
        backup->attackedRX = backup->X + backup->zombieProtoType->beAttackedPointR;
        backup->picture->setX(backup->X);
        backup->picture->setY(backup->picture->y());
        scene->addZombie(backup);
        backupDancerUuids.append(backup->uuid);
    };

    // 阶段1：播放Summon1.gif，召唤上方伴舞
    qreal oldH = picture->boundingRect().height();
    picture->setMovie("Zombies/DancingZombie/Summon1.gif");
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);
    (new Timer(picture, 500, [this, myUuid, spawnDancer] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        spawnDancer(-1, 0);  // 上方
    }))->start();

    // 阶段2：播放Summon2.gif，召唤下方伴舞
    (new Timer(picture, 1000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        qreal oldH2 = picture->boundingRect().height();
        picture->setMovie("Zombies/DancingZombie/Summon2.gif");
        picture->start();
        qreal newH2 = picture->boundingRect().height();
        if (oldH2 > 0 && newH2 > 0 && !qFuzzyCompare(oldH2, newH2))
            picture->setY(picture->y() + oldH2 - newH2);
    }))->start();
    (new Timer(picture, 1500, [this, myUuid, spawnDancer] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        spawnDancer(1, 0);  // 下方
    }))->start();

    // 阶段3：播放Summon3.gif，召唤后方伴舞
    (new Timer(picture, 2000, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        qreal oldH3 = picture->boundingRect().height();
        picture->setMovie("Zombies/DancingZombie/Summon3.gif");
        picture->start();
        qreal newH3 = picture->boundingRect().height();
        if (oldH3 > 0 && newH3 > 0 && !qFuzzyCompare(oldH3, newH3))
            picture->setY(picture->y() + oldH3 - newH3);
    }))->start();
    (new Timer(picture, 2500, [this, myUuid, spawnDancer] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        spawnDancer(0, -100);  // 后方左侧
    }))->start();

    // 阶段4：播放Summon.gif，召唤同排后方伴舞
    (new Timer(picture, 3000, [this, myUuid, spawnDancer] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        qreal oldH4 = picture->boundingRect().height();
        picture->setMovie("Zombies/DancingZombie/Summon.gif");
        picture->start();
        qreal newH4 = picture->boundingRect().height();
        if (oldH4 > 0 && newH4 > 0 && !qFuzzyCompare(oldH4, newH4))
            picture->setY(picture->y() + oldH4 - newH4);
        spawnDancer(0, -50);  // 后方右侧
    }))->start();

    // 召唤动画结束后切换到舞蹈动画
    (new Timer(picture, 3800, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        QString dancingGif = "Zombies/DancingZombie/Dancing.gif";
        this->normalGif = dancingGif;
        if (!isAttacking) {
            qreal oldH2 = picture->boundingRect().height();
            picture->setMovie(dancingGif);
            picture->start();
            qreal newH2 = picture->boundingRect().height();
            if (oldH2 > 0 && newH2 > 0 && !qFuzzyCompare(oldH2, newH2))
                picture->setY(picture->y() + oldH2 - newH2);
        }
    }))->start();
}

bool DancingZombieInstance::isAnyBackupAttacking()
{
    GameScene *scene = zombieProtoType->scene;
    for (const auto &uid : backupDancerUuids) {
        ZombieInstance *dancer = scene->getZombie(uid);
        if (dancer && dancer->isAttacking)
            return true;
    }
    return false;
}

void DancingZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;   // speed 为负，实际向右
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();  // 攻击其他僵尸
        return;
    }

    if (beAttacked && !isAttacking) {
        judgeAttack();
    }
    if (!isAttacking) {
        qreal oldX = X;

        if (!hasSummoned) {
            // 原版：太空滑步约3格后召唤
            attackedRX -= speed;
            ZX = attackedLX -= speed;
            X -= speed;
            picture->setX(X);
            if (attackedRX < -200) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
            walkDistance += qAbs(X - oldX);
            qreal gridWidth = 80.0; // 一格约80像素
            if (walkDistance >= gridWidth * 3.0) {
                hasSummoned = true;
                spawnAllBackupDancers();
                // 召唤后降速（原版：1.5s/格→5.5s/格，约3.67倍）
                speed = 0.288;  // 1.8px/100ms → 0.288px/16ms (60fps)
                baseSpeed = 0.288;
                danceTimer = 0;
                isDancingPhase = false; // 召唤后先进入前进阶段
            }
        } else {
            // 召唤后：舞蹈周期 = 前进阶段(2.4s) + 原地跳舞阶段(2.2s)
            danceTimer++;
            int cycleFrames = isDancingPhase ? DANCE_STILL_FRAMES : DANCE_FORWARD_FRAMES;

            // 编队同步：如果有伴舞在攻击，暂停前进
            bool formationBlocked = isAnyBackupAttacking();

            if (!isDancingPhase && !formationBlocked) {
                // 前进阶段：缓慢移动
                attackedRX -= speed;
                ZX = attackedLX -= speed;
                X -= speed;
                picture->setX(X);
                if (attackedRX < -200) {
                    zombieProtoType->scene->zombieDie(this);
                    return;
                }
            }
            // 原地跳舞阶段或编队同步中：不移动

            if (danceTimer >= cycleFrames) {
                // 切换阶段
                danceTimer = 0;
                isDancingPhase = !isDancingPhase;
            }

            // 周期性检查伴舞补充（每40帧）
            if (danceTimer % 40 == 0) {
                GameScene *scene = zombieProtoType->scene;
                // 清理已死亡的伴舞UUID
                for (int i = backupDancerUuids.size() - 1; i >= 0; --i) {
                    if (!scene->getZombie(backupDancerUuids[i])) {
                        backupDancerUuids.removeAt(i);
                    }
                }
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
    qreal oldH = picture->boundingRect().height();
    picture->setMovie(zombieProtoType->dieGif);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);
    QUuid myUuid = uuid;
    // 播放死亡动画2秒后直接移除
    (new Timer(picture, 2000, [this, myUuid] {
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
    damagePoint1 = hp * 2 / 3;  // 180
    speed = 0.24;  // 1.5px/100ms → 0.24px/16ms (60fps)，原版速度与普通僵尸相同
    sunNum = 0;
    beAttackedPointL = 80;
    beAttackedPointR = 140;
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
{
    this->normalGif = zombie->normalGif;
}

void BackupDancerInstance::birth(int row)
{
    // 先调用基类birth设置位置和阴影
    ZX = attackedLX = zombieProtoType->scene->getCoordinate().getX(11);
    X = attackedLX - zombieProtoType->beAttackedPointL;
    attackedRX = X + zombieProtoType->beAttackedPointR;
    this->row = row;

    Coordinate &coordinate = zombieProtoType->scene->getCoordinate();
    picture->setPos(X, coordinate.getY(row) - zombieProtoType->height - 10);
    picture->setZValue(3 * row + 1);
    shadowPNG = new QGraphicsPixmapItem(gImageCache->load("interface/plantShadow.png"));
    shadowPNG->setPos(zombieProtoType->width * 0.5 - 48, zombieProtoType->height - 22);
    shadowPNG->setFlag(QGraphicsItem::ItemStacksBehindParent);
    shadowPNG->setParentItem(picture);
    zombieProtoType->scene->addToGame(picture);

    // 播放Mound出场动画，出场后切换到正常行走
    picture->setMovie("Zombies/BackupDancer/Mound.gif");
    picture->start();
    QUuid myUuid = uuid;
    (new Timer(picture, 800, [this, myUuid] {
        ZombieInstance *self = zombieProtoType->scene->getZombie(myUuid);
        if (!self || self != this || goingDie) return;
        qreal oldH = picture->boundingRect().height();
        picture->setMovie("Zombies/BackupDancer/BackupDancer.gif");
        picture->start();
        qreal newH = picture->boundingRect().height();
        if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
            picture->setY(picture->y() + oldH - newH);
    }))->start();
}

// ===================== 潜水僵尸 =====================
bool SnorkelZombie::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 2; // 仅水域行
}

SnorkelZombie::SnorkelZombie()
{
    eName = "oSnorkelZombie";
    cName = tr("潜水僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // 180
    speed = 0.24;  // 1.5px/100ms → 0.24px/16ms (60fps)
    level = 2;
    sunNum = 50;
    beAttackedPointL = 80;
    beAttackedPointR = 160;
    breakPoint = 90;  // std2 = 90
    QString path = "Zombies/SnorkelZombie/";
    cardGif = "Card/Zombies/SnorkelZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";      // 陆地行走
    attackGif = path + "Attack.gif";     // 攻击（通用）
    lostHeadGif = path + "Walk1.gif";    // 掉头后不能潜水，水面行走
    lostHeadAttackGif = path + "Attack.gif";
    dieGif = path + "Die.gif";           // 死亡
    headGif = path + "Head.gif";         // 掉落的头部
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

SnorkelZombieInstance::SnorkelZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), submerged(true), transitioning(false), jumping(false), visCheckTimer(0)
{}

void SnorkelZombieInstance::birth(int row)
{
    ZombieInstance::birth(row);
    // 设置水陆动画路径
    QString path = "Zombies/SnorkelZombie/";
    landNormalGif = path + "Walk1.gif";
    waterNormalGif = path + "Walk2.gif";
    landAttackGif = path + "Attack.gif";
    // 根据出生行决定初始动画
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    if (groundType == 2) {
        // 出生在水域：直接用水下动画
        enteredWater = true;
        normalGif = waterNormalGif;
        attackGif = landAttackGif;
    } else {
        enteredWater = false;
        normalGif = landNormalGif;
        attackGif = landAttackGif;
    }
    picture->setMovie(normalGif);
    picture->start();
}

void SnorkelZombieInstance::updateVisibility()
{
    if (transitioning || jumping) return;  // 过渡/跳跃动画播放中，不处理
    // 掉头后无法潜水，始终水面行走
    if (damageStage1) {
        if (submerged) {
            submerged = false;
            picture->setOpacity(1.0);
            normalGif = landNormalGif;
        }
        return;
    }
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
        // 浮出水面：播放 Risk.gif（露头预警）后切换为水面行走
        submerged = false;
        transitioning = true;
        picture->setMovie("Zombies/SnorkelZombie/Risk.gif");
        picture->start();
        (new Timer(picture, 600, [this] {
            transitioning = false;
            picture->setOpacity(1.0);
            normalGif = (enteredWater) ? waterNormalGif : landNormalGif;
            picture->setMovie(normalGif);
            picture->start();
        }))->start();
    } else if (!nearPlant && !submerged) {
        // 潜入水中：播放 Sink.gif（溺水）后切换为半透明
        submerged = true;
        transitioning = true;
        picture->setMovie("Zombies/SnorkelZombie/Sink.gif");
        picture->start();
        (new Timer(picture, 600, [this] {
            transitioning = false;
            picture->setOpacity(0.15);
            normalGif = (enteredWater) ? waterNormalGif : landNormalGif;
            picture->setMovie(normalGif);
            picture->start();
        }))->start();
    }
}

void SnorkelZombieInstance::tryPumpkinJump()
{
    if (jumping) return;
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col < 1 || col > 9) return;
    auto plants = zombieProtoType->scene->getPlant(col, row);
    // 检查该格子是否有南瓜头（pKind=2）且保护着内部植物（pKind=1）
    if (!plants.contains(2) || !plants.contains(1)) return;
    PlantInstance *pumpkin = plants[2];
    PlantInstance *inner = plants[1];
    if (!pumpkin || !inner) return;
    if (!inner->plantProtoType->canEat) return;
    // 僵尸必须已到达南瓜头攻击范围
    if (!(pumpkin->attackedRX >= ZX && pumpkin->attackedLX <= ZX)) return;
    if (inner->hp <= 0) return;

    // 触发翻越：播放 Jump.gif，1.2s 后落地直接攻击内部植物
    jumping = true;
    transitioning = true;
    altitude = 2;  // 跳跃高度，翻越南瓜头
    picture->setMovie("Zombies/SnorkelZombie/Jump.gif");
    picture->start();
    (new Timer(picture, 1200, [this, inner] {
        jumping = false;
        transitioning = false;
        altitude = 1;  // 落地
        // 直接对内部植物造成一次攻击
        if (inner && inner->hp > 0) {
            inner->getHurt(this, zombieProtoType->aKind, zombieProtoType->attack);
        }
        normalGif = (enteredWater && !damageStage1) ? waterNormalGif : landNormalGif;
        picture->setMovie(normalGif);
        picture->start();
    }))->start();
}

void SnorkelZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;   // speed 为负，实际向右
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();
        return;
    }

    // ---- 水域/陆地行走动画切换 ----
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    bool onWater = (groundType == 2);

    if (onWater && !enteredWater) {
        // 进入水域：切换 Walk2.gif
        enteredWater = true;
        if (!submerged && !transitioning) {
            normalGif = waterNormalGif;
            if (!isAttacking && !goingDie && !jumping) {
                picture->setMovie(normalGif);
                picture->start();
            }
        }
    } else if (!onWater && enteredWater) {
        // 返回陆地：切换 Walk1.gif
        enteredWater = false;
        submerged = false;
        picture->setOpacity(1.0);
        if (!transitioning) {
            normalGif = landNormalGif;
            if (!isAttacking && !goingDie && !jumping) {
                picture->setMovie(normalGif);
                picture->start();
            }
        }
    }

    // ---- 每5帧检查一次可见性（潜水/浮出），减少性能开销 ----
    if (!jumping) {
        visCheckTimer++;
        if (visCheckTimer >= 5) {
            visCheckTimer = 0;
            updateVisibility();
        }
    }

    // ---- 南瓜头翻越检测 ----
    if (!submerged && !jumping && !transitioning) {
        tryPumpkinJump();
    }

    if (beAttacked && !isAttacking && !jumping) {
        judgeAttack();
    }
    if (!isAttacking && !jumping) {
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

void SnorkelZombieInstance::judgeAttack()
{
    if (jumping) return;  // 跳跃中不执行普通攻击

    // ---- 被魅惑的僵尸：攻击其他僵尸 ----
    if (isHypnotized) {
        ZombieInstance::judgeAttack();
        return;
    }

    // ---- 南瓜头翻越判定：若攻击目标是南瓜头，改为攻击内部植物 ----
    bool tempIsAttacking = false;
    PlantInstance *plant = nullptr;
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col >= 1 && col <= 9) {
        auto plants = zombieProtoType->scene->getPlant(col, row);
        QList<int> keys = plants.keys();
        qSort(keys.begin(), keys.end(), [](int a, int b) { return b < a; });
        for (auto key: keys) {
            PlantInstance *p = plants[key];
            if (p->plantProtoType->canEat && p->attackedRX >= ZX && p->attackedLX <= ZX) {
                // 如果首选目标是南瓜头(pKind=2)且存在内部植物(pKind=1)，跳过南瓜直接攻击内部
                if (p->plantProtoType->pKind == 2 && plants.contains(1)
                    && plants[1]->plantProtoType->canEat && plants[1]->hp > 0) {
                    tryPumpkinJump();
                    return;
                }
                plant = p;
                tempIsAttacking = true;
                break;
            }
        }
    }
    if (tempIsAttacking != isAttacking) {
        isAttacking = tempIsAttacking;
        if (isAttacking) {
            picture->setMovie(attackGif);
        } else {
            picture->setMovie(normalGif);
        }
        picture->start();
    }
    if (isAttacking)
        normalAttack(plant);
}

void SnorkelZombieInstance::getPea(int attack, int direction, int type)
{
    Q_UNUSED(direction);
    Q_UNUSED(type);
    // 正版：潜水状态下免疫豌豆直射（type 0/1/2/3 均为直射类子弹）
    if (submerged)
        return;
    ZombieInstance::getPea(attack, direction, type);
}

// 注：原 SnorkelZombieInstance::getHit override 已移除
// 潜水僵尸的爆炸/秒杀/地刺伤害走基类 ZombieInstance::getHit，符合正版行为
// （爆炸樱桃、窝瓜秒杀、土豆地雷等都能伤到水下僵尸）

// ===================== 海豚骑士僵尸 =====================
bool DolphinRiderZombie::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 2; // 仅水域行
}

DolphinRiderZombie::DolphinRiderZombie()
{
    eName = "oDolphinRiderZombie";
    cName = tr("海豚骑士僵尸");
    hp = 500;
    damagePoint1 = hp * 2 / 3;  // std1 = 333
    speed = 0.512;  // 3.2px/100ms → 0.512px/16ms (60fps)
    level = 2;
    sunNum = 75;
    beAttackedPointL = 120;
    beAttackedPointR = 240;
    breakPoint = hp / 3;  // std2 = 166
    QString path = "Zombies/DolphinRiderZombie/";
    cardGif = "Card/Zombies/DolphinRiderZombie.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";          // 陆地行走（入水前）
    attackGif = path + "Attack.gif";         // 攻击（通用）
    damageGif1 = path + "Walk1.gif";         // 轻伤陆地行走
    damageAttackGif1 = path + "Attack.gif";  // 轻伤攻击
    lostHeadGif = path + "Walk1.gif";        // 掉头后陆地行走（失去海豚）
    lostHeadAttackGif = path + "Attack.gif";
    dieGif = path + "Die.gif";               // 常规死亡
    headGif = path + "Head.gif";             // 掉落头部
    boomDieGif = path + "Die2.gif";          // 躯体分离死亡（爆炸/碾压）
    standGif = path + "1.gif";
}

DolphinRiderZombieInstance::DolphinRiderZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), jumped(false), jumpingPumpkin(false)
{
}

void DolphinRiderZombieInstance::birth(int row)
{
    ZombieInstance::birth(row);
    // 设置水陆动画路径
    QString path = "Zombies/DolphinRiderZombie/";
    landNormalGif = path + "Walk1.gif";     // 陆地行走（入水前）
    landAttackGif = path + "Attack.gif";
    waterNormalGif = path + "Walk2.gif";  // 水中动画固定 Walk2
    // 根据出生行决定初始动画
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    if (groundType == 2) {
        enteredWater = true;
        normalGif = waterNormalGif;
    } else {
        enteredWater = false;
        normalGif = landNormalGif;
    }
    attackGif = landAttackGif;
    picture->setMovie(normalGif);
    picture->start();
}

void DolphinRiderZombieInstance::tryPumpkinJump()
{
    if (jumped || jumpingPumpkin) return;
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col < 1 || col > 9) return;
    auto plants = zombieProtoType->scene->getPlant(col, row);
    // 必须有南瓜头(pKind=2)保护着内部可食用植物(pKind=1)
    if (!plants.contains(2) || !plants.contains(1)) return;
    PlantInstance *pumpkin = plants[2];
    PlantInstance *inner = plants[1];
    if (!inner->plantProtoType->canEat || inner->hp <= 0) return;
    if (!(pumpkin->attackedRX >= ZX && pumpkin->attackedLX <= ZX)) return;

    // 触发翻越：依次播放 Jump.gif → Jump2.gif → Jump3.gif
    jumped = true;
    jumpingPumpkin = true;
    altitude = 2;

    QString path = "Zombies/DolphinRiderZombie/";
    QString jump1 = path + "Jump.gif";
    QString jump2 = path + "Jump2.gif";
    QString jump3 = path + "Jump3.gif";

    qreal oldH = picture->boundingRect().height();
    picture->setMovie(jump1);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);

    (new Timer(picture, 400, [this, jump2, jump3] {
        qreal oldH2 = picture->boundingRect().height();
        picture->setMovie(jump2);
        picture->start();
        qreal newH2 = picture->boundingRect().height();
        if (oldH2 > 0 && newH2 > 0 && !qFuzzyCompare(oldH2, newH2))
            picture->setY(picture->y() + oldH2 - newH2);

        (new Timer(picture, 400, [this, jump3] {
            qreal oldH3 = picture->boundingRect().height();
            picture->setMovie(jump3);
            picture->start();
            qreal newH3 = picture->boundingRect().height();
            if (oldH3 > 0 && newH3 > 0 && !qFuzzyCompare(oldH3, newH3))
                picture->setY(picture->y() + oldH3 - newH3);

            (new Timer(picture, 400, [this] {
                // 跳跃完成：前移 130px 翻越南瓜头，攻击内部植物
                jumpingPumpkin = false;
                altitude = 1;
                ZX = attackedLX -= 130;
                X -= 130;
                attackedRX -= 130;
                picture->setX(X);
                qreal oldH4 = picture->boundingRect().height();
                // 落地后切换回当前行走动画
                int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
                bool onWater = (groundType == 2);
                normalGif = onWater ? waterNormalGif : landNormalGif;
                picture->setMovie(normalGif);
                picture->start();
                qreal newH4 = picture->boundingRect().height();
                if (oldH4 > 0 && newH4 > 0 && !qFuzzyCompare(oldH4, newH4))
                    picture->setY(picture->y() + oldH4 - newH4);
                // 跃过后速度大降（等同普通僵尸）
                speed = 0.24;
                baseSpeed = 0.24;
            }))->start();
        }))->start();
    }))->start();
}

void DolphinRiderZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();
        return;
    }

    // ---- 水域/陆地行走动画切换 ----
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    bool onWater = (groundType == 2);

    if (onWater && !enteredWater && !jumpingPumpkin) {
        // 入水动画：播放 Jump.gif（海豚跃入水中）
        enteredWater = true;
        QString path = "Zombies/DolphinRiderZombie/";
        qreal oldH = picture->boundingRect().height();
        picture->setMovie(path + "Jump.gif");
        picture->start();
        qreal newH = picture->boundingRect().height();
        if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
            picture->setY(picture->y() + oldH - newH);
        // 入水动画结束后切换 Walk2.gif 并加速
        (new Timer(picture, 600, [this, path] {
            waterNormalGif = path + "Walk2.gif";
            normalGif = waterNormalGif;
            speed = 0.72;
            baseSpeed = 0.72;
            if (!isAttacking && !goingDie) {
                picture->setMovie(normalGif);
                picture->start();
            }
        }))->start();
    } else if (!onWater && enteredWater && !jumpingPumpkin) {
        // 返回陆地：切换 Walk1.gif，恢复陆地速度
        enteredWater = false;
        normalGif = landNormalGif;
        speed = 0.512;
        baseSpeed = 0.512;
        if (!isAttacking && !goingDie) {
            picture->setMovie(normalGif);
            picture->start();
        }
    }

    // ---- 南瓜头翻越检测（跳跃动画中跳过移动和攻击） ----
    if (jumpingPumpkin) return;

    if (!jumped) {
        tryPumpkinJump();
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

void DolphinRiderZombieInstance::judgeAttack()
{
    if (jumpingPumpkin) return;

    // ---- 被魅惑的僵尸：攻击其他僵尸 ----
    if (isHypnotized) {
        ZombieInstance::judgeAttack();
        return;
    }

    // ---- 南瓜头翻越判定：遇南瓜头触发跳跃，否则正常啃食 ----
    bool tempIsAttacking = false;
    PlantInstance *plant = nullptr;
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col >= 1 && col <= 9) {
        auto plants = zombieProtoType->scene->getPlant(col, row);
        QList<int> keys = plants.keys();
        qSort(keys.begin(), keys.end(), [](int a, int b) { return b < a; });
        for (auto key: keys) {
            PlantInstance *p = plants[key];
            if (p->plantProtoType->canEat && p->attackedRX >= ZX && p->attackedLX <= ZX) {
                // 首选目标是南瓜头(pKind=2)且存在内部植物 → 触发翻越
                if (!jumped && p->plantProtoType->pKind == 2
                    && plants.contains(1) && plants[1]->plantProtoType->canEat
                    && plants[1]->hp > 0) {
                    tryPumpkinJump();
                    return;
                }
                plant = p;
                tempIsAttacking = true;
                break;
            }
        }
    }
    if (tempIsAttacking != isAttacking) {
        isAttacking = tempIsAttacking;
        if (isAttacking) {
            picture->setMovie(attackGif);
        } else {
            picture->setMovie(normalGif);
        }
        picture->start();
    }
    if (isAttacking)
        normalAttack(plant);
}

// ===================== 小鬼僵尸 =====================
Imp::Imp()
{
    eName = "oImp";
    cName = tr("小鬼僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // 180
    speed = 0.48;
    level = 1;
    sunNum = 25;
    beAttackedPointL = 80;   // 扩大攻击判定区
    beAttackedPointR = 160;  // 与普通僵尸一致
    breakPoint = 0;
    QString path = "Zombies/Imp/";
    cardGif = "Card/Zombies/Imp.png";
    staticGif = path + "0.gif";
    normalGif = path + "1.gif";
    attackGif = path + "Attack.gif";
    lostHeadGif = "Zombies/Zombie/ZombieLostHead.gif";
    lostHeadAttackGif = "Zombies/Zombie/ZombieLostHeadAttack.gif";
    headGif = "Zombies/Zombie/ZombieHead.gif";
    dieGif = path + "Die.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "1.gif";
}

ImpInstance::ImpInstance(const Zombie *zombie)
    : ZombieInstance(zombie)
{}

//修改死亡动画（软件组）
void ImpInstance::getHit(int attack)
{
    if (!beAttacked || goingDie)
        return;

    hp -= attack;

    // 血量归零 → 直接播放 dieGif，无断头
    if (hp <= 0) {
        normalDie();
        return;
    }

    // 普通受伤闪红（保留基类效果）
    picture->setOpacity(0.25);
    (new Timer(picture, 150, [this] {
        if (hp < zombieProtoType->breakPoint) return; // breakPoint=0，不会进入
        picture->setOpacity(1.0);
    }))->start();
}
// ===================== 鸭子僵尸 =====================
bool DuckyTubeZombie1::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 2; // 仅水域行
}

DuckyTubeZombie1::DuckyTubeZombie1()
{
    eName = "oDuckyTubeZombie1";
    cName = tr("鸭子僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // 180
    speed = 0.24;  // 1.5px/100ms → 0.24px/16ms (60fps)
    level = 1;
    sunNum = 25;
    beAttackedPointL = 80;
    beAttackedPointR = 160;
    breakPoint = 90;
    QString path = "Zombies/DuckyTubeZombie1/";
    cardGif = "Card/Zombies/DuckyTubeZombie1.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";
    attackGif = path + "Attack.gif";
    // 损伤阶段使用Walk2.gif（鸭子管受损）
    damageGif1 = path + "Walk2.gif";
    damageAttackGif1 = path + "Attack.gif";
    // 无断头动画，使用基础僵尸的
    lostHeadGif = "Zombies/Zombie/ZombieLostHead.gif";
    lostHeadAttackGif = "Zombies/Zombie/ZombieLostHeadAttack.gif";
    headGif = "Zombies/Zombie/ZombieHead.gif";
    dieGif = path + "Die.gif";
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

DuckyTubeZombie1Instance::DuckyTubeZombie1Instance(const Zombie *zombie)
    : ZombieInstance(zombie)
{}

void DuckyTubeZombie1Instance::birth(int row)
{
    ZombieInstance::birth(row);
    // 启用入水逻辑：Walk1.gif=陆地行走(带鸭子泳圈), Walk2.gif=水中行走, Attack.gif=通用攻击
    enteredWater = false;
    QString path = "Zombies/DuckyTubeZombie1/";
    landNormalGif = path + "Walk1.gif";
    waterNormalGif = path + "Walk2.gif";
    landAttackGif = path + "Attack.gif";
    normalGif = landNormalGif;
    attackGif = landAttackGif;
    picture->setMovie(normalGif);
    picture->start();
}

void DuckyTubeZombie1Instance::checkActs()
{
    // 根据当前行地面类型判定水域/陆地，自动切换行走动画
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    bool onWater = (groundType == 2);

    if (onWater && !enteredWater) {
        // 首次进入水域：播放入水特效 + 切换 Walk2.gif
        triggerWaterEntry();
    } else if (!onWater && enteredWater) {
        // 返回陆地：切回 Walk1.gif
        enteredWater = false;
        normalGif = landNormalGif;
        attackGif = landAttackGif;
        if (!isAttacking && !goingDie) {
            picture->setMovie(normalGif);
            picture->start();
        }
    }
    ZombieInstance::checkActs();
}

// ===================== 路障鸭子僵尸 =====================
bool DuckyTubeZombie2::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 2; // 仅水域行
}

DuckyTubeZombie2::DuckyTubeZombie2()
{
    eName = "oDuckyTubeZombie2";
    cName = tr("路障鸭子僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // 180
    ornHp = 370;
    level = 2;
    sunNum = 75;
    breakPoint = 90;
    QString path = "Zombies/DuckyTubeZombie2/";
    cardGif = "Card/Zombies/DuckyTubeZombie1.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";
    attackGif = path + "Attack.gif";
    // 本体损伤使用红色染色效果
    ornLostNormalGif = path + "Walk2.gif";
    ornLostAttackGif = path + "Attack.gif";
    lostHeadGif = "Zombies/Zombie/ZombieLostHead.gif";
    lostHeadAttackGif = "Zombies/Zombie/ZombieLostHeadAttack.gif";
    headGif = "Zombies/Zombie/ZombieHead.gif";
    dieGif = "Zombies/DuckyTubeZombie1/Die.gif";
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

DuckyTubeZombie2Instance::DuckyTubeZombie2Instance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void DuckyTubeZombie2Instance::birth(int row)
{
    OrnZombieInstance1::birth(row);
    // 启用入水逻辑：Walk1.gif=陆地行走(带鸭子泳圈), Walk2.gif=水中行走, Attack.gif=通用攻击
    enteredWater = false;
    QString path = "Zombies/DuckyTubeZombie2/";
    landNormalGif = path + "Walk1.gif";
    waterNormalGif = path + "Walk2.gif";
    landAttackGif = path + "Attack.gif";
    normalGif = landNormalGif;
    attackGif = landAttackGif;
    picture->setMovie(normalGif);
    picture->start();
}

void DuckyTubeZombie2Instance::checkActs()
{
    // 根据当前行地面类型判定水域/陆地，自动切换行走动画
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    bool onWater = (groundType == 2);

    if (onWater && !enteredWater) {
        triggerWaterEntry();
    } else if (!onWater && enteredWater) {
        enteredWater = false;
        normalGif = landNormalGif;
        attackGif = landAttackGif;
        if (!isAttacking && !goingDie) {
            picture->setMovie(normalGif);
            picture->start();
        }
    }
    OrnZombieInstance1::checkActs();
}

void DuckyTubeZombie2Instance::playNormalballAudio()
{
    if (hasOrnaments) {
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        player->setMedia(QUrl("qrc:/audio/plastichit.mp3"));
        player->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

// ===================== 铁桶鸭子僵尸 =====================
bool DuckyTubeZombie3::canPass(int row) const
{
    return scene->getGameLevelData()->LF[row] == 2; // 仅水域行
}

DuckyTubeZombie3::DuckyTubeZombie3()
{
    eName = "oDuckyTubeZombie3";
    cName = tr("铁桶鸭子僵尸");
    hp = 270;
    damagePoint1 = hp * 2 / 3;  // 180
    ornHp = 1100;
    level = 3;
    sunNum = 125;
    breakPoint = 90;
    QString path = "Zombies/DuckyTubeZombie3/";
    cardGif = "Card/Zombies/DuckyTubeZombie1.png";
    staticGif = path + "0.gif";
    normalGif = path + "Walk1.gif";
    attackGif = path + "Attack.gif";
    // 本体损伤使用红色染色效果
    ornLostNormalGif = path + "Walk2.gif";
    ornLostAttackGif = path + "Attack.gif";
    lostHeadGif = "Zombies/Zombie/ZombieLostHead.gif";
    lostHeadAttackGif = "Zombies/Zombie/ZombieLostHeadAttack.gif";
    headGif = "Zombies/Zombie/ZombieHead.gif";
    dieGif = "Zombies/DuckyTubeZombie1/Die.gif";
    boomDieGif = "Zombies/Zombie/BoomDie.gif";
    standGif = path + "1.gif";
}

DuckyTubeZombie3Instance::DuckyTubeZombie3Instance(const Zombie *zombie)
    : OrnZombieInstance1(zombie)
{}

void DuckyTubeZombie3Instance::birth(int row)
{
    OrnZombieInstance1::birth(row);
    // 启用入水逻辑：Walk1.gif=陆地行走(带鸭子泳圈), Walk2.gif=水中行走, Attack.gif=通用攻击
    enteredWater = false;
    QString path = "Zombies/DuckyTubeZombie3/";
    landNormalGif = path + "Walk1.gif";
    waterNormalGif = path + "Walk2.gif";
    landAttackGif = path + "Attack.gif";
    normalGif = landNormalGif;
    attackGif = landAttackGif;
    picture->setMovie(normalGif);
    picture->start();
}

void DuckyTubeZombie3Instance::checkActs()
{
    // 根据当前行地面类型判定水域/陆地，自动切换行走动画
    int groundType = zombieProtoType->scene->getGameLevelData()->LF[row];
    bool onWater = (groundType == 2);

    if (onWater && !enteredWater) {
        triggerWaterEntry();
    } else if (!onWater && enteredWater) {
        enteredWater = false;
        normalGif = landNormalGif;
        attackGif = landAttackGif;
        if (!isAttacking && !goingDie) {
            picture->setMovie(normalGif);
            picture->start();
        }
    }
    OrnZombieInstance1::checkActs();
}

void DuckyTubeZombie3Instance::playNormalballAudio()
{
    if (hasOrnaments) {
        QMediaPlayer *player = getSharedAudioPlayer();
        player->stop();
        if (qrand() % 2)
            player->setMedia(QUrl("qrc:/audio/shieldhit.mp3"));
        else
            player->setMedia(QUrl("qrc:/audio/shieldhit2.mp3"));
        player->play();
    }
    else
        OrnZombieInstance1::playNormalballAudio();
}

// ===================== 冰车僵尸 =====================
ZomboniZombie::ZomboniZombie()
{
    eName = "oZomboni";
    cName = tr("冰车僵尸");
    hp = 1350;
    damagePoint1 = hp * 2 / 3;  // 900 → 受损状态
    speed = 0.288;
    level = 4;
    sunNum = 175;
    beAttackedPointL = 120;
    beAttackedPointR = 240;
    breakPoint = 0;              // 不掉头
    aKind = 1;                   // 碾压攻击类型
    QString path = "Zombies/Zomboni/";
    cardGif = "Card/Zombies/Zomboni.png";
    staticGif = path + "0.gif";
    normalGif = path + "0.gif";
    damageGif1 = path + "5.gif"; // 受损状态
    attackGif = path + "0.gif";
    dieGif = path + "BoomDie.gif";
    boomDieGif = path + "BoomDie.gif";
    standGif = path + "0.gif";
}

ZomboniZombieInstance::ZomboniZombieInstance(const Zombie *zombie)
    : ZombieInstance(zombie), iceTrailTimer(0),
      walkAnimIndex(0), walkAnimTimer(0), damaged(false)
{}

void ZomboniZombieInstance::updateWalkAnimation()
{
    if (goingDie) return;
    QString path = "Zombies/Zomboni/";

    // 受损状态：播放 5.gif
    if (hp < zombieProtoType->damagePoint1 && !damaged) {
        damaged = true;
        picture->setMovie(path + "5.gif");
        picture->start();
        return;
    }

    // 非受损状态：循环 0→1→2→3→4
    if (!damaged) {
        walkAnimTimer++;
        if (walkAnimTimer >= 12) {  // 约每200ms切换帧
            walkAnimTimer = 0;
            walkAnimIndex = (walkAnimIndex + 1) % 5;
            picture->setMovie(path + QString::number(walkAnimIndex) + ".gif");
            picture->start();
        }
    }
}

void ZomboniZombieInstance::leaveIceTrail()
{
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col < 1 || col > 9) return;

    // 防止重复在同一个格子铺冰
    auto key = qMakePair(col, row);
    if (iceCells.contains(key)) return;
    iceCells.insert(key);

    // 永久冰道贴图
    Coordinate &coord = zombieProtoType->scene->getCoordinate();
    qreal cx = coord.getX(col);
    qreal cy = coord.getY(row);

    QGraphicsPixmapItem *ice = new QGraphicsPixmapItem(
        gImageCache->load("Zombies/Zomboni/ice.png"));
    ice->setPos(cx - 40, cy - 70);
    ice->setZValue(0);  // 底层
    zombieProtoType->scene->addToGame(ice);

    // 冰道边缘贴图
    QGraphicsPixmapItem *iceCap = new QGraphicsPixmapItem(
        gImageCache->load("Zombies/Zomboni/ice_cap.png"));
    iceCap->setPos(cx - 20, cy - 80);
    iceCap->setZValue(1);
    zombieProtoType->scene->addToGame(iceCap);

    // 永久禁止种植：添加弹坑标记
    zombieProtoType->scene->addCrater(col, row);
}

void ZomboniZombieInstance::crushPlants()
{
    int col = zombieProtoType->scene->getCoordinate().getCol(ZX);
    if (col < 1 || col > 9) return;
    auto plants = zombieProtoType->scene->getPlant(col, row);

    // 优先检查 Spikeweed/Spikerock：互毁
    if (plants.contains(1)) {
        QString eName = plants[1]->plantProtoType->eName;
        if (eName == "oSpikeweed" || eName == "oSpikerock") {
            // 地刺穿刺冰车 → 冰车立即爆车
            plants[1]->getHurt(this, 1, 1800);  // 摧毁地刺
            hp = 0;
            boomDie();
            return;
        }
    }

    // 碾压其他可食植物（含南瓜头 pKind=2）
    for (auto plant : plants.values()) {
        if (!plant->plantProtoType->canEat) continue;
        if (plant->attackedRX < ZX || plant->attackedLX > ZX) continue;

        // 土豆地雷已就绪时不碾压（会自行爆炸），未就绪则碾压
        if (plant->plantProtoType->eName == "oPotatoMine") {
            // 地雷有自己的触发机制，冰车直接驶过会被炸
            // 不做额外处理，让地雷 triggerCheck 自行处理
            continue;
        }

        // 碾压植物：aKind=1 碾压伤害 1800（秒杀）
        plant->getHurt(this, 1, 1800);
    }
}

void ZomboniZombieInstance::checkActs()
{
    if (hp < 1 || goingDie) return;

    // ---- 被魅惑的僵尸：向右移动 ----
    if (isHypnotized) {
        if (!isAttacking) {
            attackedLX -= speed;
            ZX = attackedLX;
            X = attackedLX - zombieProtoType->beAttackedPointL;
            picture->setX(X);
            if (attackedLX > 900) {
                zombieProtoType->scene->zombieDie(this);
                return;
            }
        }
        judgeAttack();
        return;
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

        // 行走动画循环
        updateWalkAnimation();

        // 每隔一段距离生成冰道
        iceTrailTimer++;
        if (iceTrailTimer >= 15) {
            iceTrailTimer = 0;
            leaveIceTrail();
        }

        // 碾压前方植物
        crushPlants();
    }
}

void ZomboniZombieInstance::applySlow(qreal multiplier, int durationMs)
{
    // 冰车僵尸免疫冰冻和减速
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

    QString boomGif = zombieProtoType->boomDieGif.isEmpty()
        ? zombieProtoType->dieGif : zombieProtoType->boomDieGif;
    qreal oldH = picture->boundingRect().height();
    picture->setMovie(boomGif);
    picture->start();
    qreal newH = picture->boundingRect().height();
    if (oldH > 0 && newH > 0 && !qFuzzyCompare(oldH, newH))
        picture->setY(picture->y() + oldH - newH);

    // 爆炸摧毁周围 3×3 范围内的植物
    Coordinate &coord = zombieProtoType->scene->getCoordinate();
    int deathCol = coord.getCol(ZX);
    for (int tr = qMax(1, row - 1); tr <= qMin(coord.rowCount(), row + 1); ++tr) {
        for (int tc = qMax(1, deathCol - 1); tc <= qMin(9, deathCol + 1); ++tc) {
            auto plants = zombieProtoType->scene->getPlant(tc, tr);
            for (auto it = plants.begin(); it != plants.end(); ++it) {
                it.value()->getHurt(nullptr, 1, 1800);
            }
        }
    }

    QUuid myUuid = uuid;
    (new Timer(picture, 1500, [this, myUuid] {
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
    else if (ename == "oZombie2")
        zombie = new Zombie2;
    else if (ename == "oZombie3")
        zombie = new Zombie3;
    else if (ename == "oFlagZombie")
        zombie = new FlagZombie;
    else if (ename == "oConeheadZombie")
        zombie = new ConeheadZombie;
    else if (ename == "oBucketheadZombie")
        zombie = new BucketheadZombie;
    else if (ename == "oPoleVaultingZombie")
        zombie = new PoleVaultingZombie;
    else if (ename == "oNewspaperZombie")
        zombie = new NewspaperZombie;
    else if (ename == "oFootballZombie")
        zombie = new FootballZombie;
    else if (ename == "oScreenDoorZombie")
        zombie = new ScreenDoorZombie;
    else if (ename == "oJackinTheBoxZombie")
        zombie = new JackinTheBoxZombie;
    else if (ename == "oDancingZombie")
        zombie = new DancingZombie;
    else if (ename == "oBackupDancer")
        zombie = new BackupDancer;
    else if (ename == "oSnorkelZombie")
        zombie = new SnorkelZombie;
    else if (ename == "oDolphinRiderZombie")
        zombie = new DolphinRiderZombie;
    else if (ename == "oZomboni")
        zombie = new ZomboniZombie;
    else if (ename == "oImp")
        zombie = new Imp;
    else if (ename == "oDuckyTubeZombie1")
        zombie = new DuckyTubeZombie1;
    else if (ename == "oDuckyTubeZombie2")
        zombie = new DuckyTubeZombie2;
    else if (ename == "oDuckyTubeZombie3")
        zombie = new DuckyTubeZombie3;
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
    else if (zombie->eName == "oBucketheadZombie")
        return new BucketheadZombieInstance(zombie);
    else if (zombie->eName == "oPoleVaultingZombie")
        return new PoleVaultingZombieInstance(zombie);
    else if (zombie->eName == "oNewspaperZombie")
        return new NewspaperZombieInstance(zombie);
    else if (zombie->eName == "oFootballZombie")//足球僵尸
        return new FootballZombieInstance(zombie);
    else if (zombie->eName == "oScreenDoorZombie")//铁门僵尸
        return new ScreenDoorZombieInstance(zombie);
    else if (zombie->eName == "oJackinTheBoxZombie")
        return new JackinTheBoxZombieInstance(zombie);
    else if (zombie->eName == "oDancingZombie")
        return new DancingZombieInstance(zombie);
    else if (zombie->eName == "oBackupDancer")
        return new BackupDancerInstance(zombie);
    else if (zombie->eName == "oSnorkelZombie")
        return new SnorkelZombieInstance(zombie);
    else if (zombie->eName == "oDolphinRiderZombie")
        return new DolphinRiderZombieInstance(zombie);
    else if (zombie->eName == "oZomboni")
        return new ZomboniZombieInstance(zombie);
    else if (zombie->eName == "oImp")
        return new ImpInstance(zombie);
    else if (zombie->eName == "oDuckyTubeZombie2")
        return new DuckyTubeZombie2Instance(zombie);
    else if (zombie->eName == "oDuckyTubeZombie3")
        return new DuckyTubeZombie3Instance(zombie);
    else if (zombie->eName == "oDuckyTubeZombie1")
        return new DuckyTubeZombie1Instance(zombie);
    return new ZombieInstance(zombie);
}


