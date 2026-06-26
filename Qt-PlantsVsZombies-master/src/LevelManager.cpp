//
// LevelManager implementation
//

#include "LevelManager.h"
#include "GameScene.h"
#include "GameLevelData.h"
#include "ImageManager.h"
#include "Animate.h"
#include "MainView.h"
#include "Timer.h"

// ---- Helpers (forward-declared) ----
static bool isLawnCleaner(const QString &eName) {
    return eName == "oLawnCleaner" || eName == "oPoolCleaner";
}

LevelManager::LevelManager(GameScene *s, GameLevelData *ld)
    : scene(s), levelData(ld), state(GameState::PLAYING), gameOver(false)
{
}

// ============================================================
// 核心胜负判定 — 每帧调用
// ============================================================
GameState LevelManager::checkWinLose()
{
    if (gameOver) return state;

    // --- 失败判定（优先级最高）---
    // 遍历所有行，检查是否有僵尸已到达房子左侧且无可用割草机
    for (int row = 1; row <= scene->getCoordinate().rowCount(); ++row) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp <= 0 || z->goingDie) continue;
            // 僵尸 x 坐标 <= -20 即进入房子
            if (z->attackedRX <= -20) {
                // 检查割草机是否还在（割草机由 trigger 系统自动触发）
                if (!hasActiveLawnmowerOnRow(row)) {
                    // 没有割草机 → 立即失败
                    triggerLose();
                    return state;
                }
                // 有割草机但僵尸已越过割草机触发范围 → 割草机可能已经触发过
                // 继续游戏，割草机会处理（如果还没触发的话）
            }
        }
    }

    // --- 胜利判定 ---
    // 条件：所有波次已生成 且 场上无存活僵尸
    if (allWavesSpawned() && allZombiesDead()) {
        triggerWin();
        return state;
    }

    return state;
}

// ============================================================
// 失败处理
// ============================================================
void LevelManager::triggerLose()
{
    if (gameOver) return;
    gameOver = true;
    state = GameState::LOSE;

    // 停止背景音乐
    // 淡入 ZombiesWon 画面
    QGraphicsPixmapItem *loseOverlay = new QGraphicsPixmapItem(
        gImageCache->load("interface/ZombiesWon.png"));
    loseOverlay->setPos(250, 50);
    loseOverlay->setZValue(200);
    loseOverlay->setOpacity(0.0);
    scene->addItem(loseOverlay);

    Animate(loseOverlay).fade(1.0).duration(1500).finish([this, loseOverlay] {
        // 等待 2 秒后重新开始本关
        (new Timer(scene, 2000, [this, loseOverlay] {
            scene->removeItem(loseOverlay);
            delete loseOverlay;
            // 保存当前关卡名称，重新进入
            QString currentEName = levelData->eName;
            gMainView->switchToGameScene(currentEName);
        }))->start();
    });
}

// ============================================================
// 胜利处理
// ============================================================
void LevelManager::triggerWin()
{
    if (gameOver) return;
    gameOver = true;
    state = GameState::WIN;

    // 淡入 trophy 画面
    QGraphicsPixmapItem *winOverlay = new QGraphicsPixmapItem(
        gImageCache->load("interface/trophy.png"));
    winOverlay->setPos(320, 120);
    winOverlay->setZValue(200);
    winOverlay->setOpacity(0.0);
    scene->addItem(winOverlay);

    Animate(winOverlay).fade(1.0).duration(1500).finish([this, winOverlay] {
        // 等待 2 秒后进入下一关
        (new Timer(scene, 2000, [this, winOverlay] {
            scene->removeItem(winOverlay);
            delete winOverlay;
            // 计算下一关卡编号
            QString currentEName = levelData->eName;
            int nextNum = currentEName.toInt() + 1;
            QString nextEName = QString::number(nextNum);
            // 如果下一关不存在，返回主菜单
            if (nextNum > 3) {
                gMainView->switchToMenuScene();
            } else {
                gMainView->switchToGameScene(nextEName);
            }
        }))->start();
    });
}

// ============================================================
// 检查指定行是否有可用的割草机
// ============================================================
bool LevelManager::hasActiveLawnmowerOnRow(int row) const
{
    // 割草机以植物形式存储在 plantPosition 中，列号为 -1
    auto plants = scene->getPlant(-1, row);
    for (auto it = plants.begin(); it != plants.end(); ++it) {
        PlantInstance *p = it.value();
        if (p && isLawnCleaner(p->plantProtoType->eName) && p->hp > 0) {
            return true;
        }
    }
    return false;
}

// ============================================================
// 所有波次是否已生成完毕
// ============================================================
bool LevelManager::allWavesSpawned() const
{
    // 所有 flag 波次已推进完毕
    int waveNum = scene->getWaveNum();
    int flagNum = scene->getFlagNum();
    if (waveNum < flagNum) return false;
    // 最后一波已触发，需要等待僵尸实际生成
    // 简化处理：最后一波触发后 waveNum >= flagNum
    return true;
}

// ============================================================
// 场上所有僵尸是否已死亡
// ============================================================
bool LevelManager::allZombiesDead() const
{
    for (int row = 1; row <= scene->getCoordinate().rowCount(); ++row) {
        QList<ZombieInstance *> zombies = scene->getZombieOnRow(row);
        for (auto *z : zombies) {
            if (z->hp > 0 && !z->goingDie) {
                return false;
            }
        }
    }
    return true;
}
