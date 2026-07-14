//
// Created by sun on 8/26/16.
//关卡设置文件

#include "GameLevelData.h"
#include "GameScene.h"
#include "ImageManager.h"
#include "Timer.h"

GameLevelData::GameLevelData() : cardKind(0),
                                 dKind(1),
                                 sunNum(50),
                                 backgroundImage("interface/background1.jpg"),
                                 LF{ 0, 1, 1, 1, 1, 1 },
                                 canSelectCard(true),
                                 staticCard(true),
                                 showScroll(true),
                                 produceSun(true),
                                 hasShovel(true),
                                 maxSelectedCards(8),
                                 coord(0),
                                 flagNum(0)
{}

void  GameLevelData::loadAccess(GameScene *gameScene)
{
    gameScene->loadAcessFinished();
}

void GameLevelData::startGame(GameScene *gameScene)
{
    initLawnMower(gameScene);
    gameScene->prepareGrowPlants( [this, gameScene] {
        gameScene->beginBGM();
        gameScene->beginMonitor();
        gameScene->beginCool();
        if (produceSun)
            gameScene->beginSun(25);
        (new Timer(gameScene, 3000/*15000*/, [gameScene] {
            gameScene->beginZombies();
        }))->start();
    });
}

void GameLevelData::initLawnMower(GameScene *gameScene)
{
    for (int i = 0; i < LF.size(); ++i) {
        if (LF[i] == 1)
            gameScene->customSpecial("oLawnCleaner", -1, i);
        else if (LF[i] == 2)
            gameScene->customSpecial("oPoolCleaner", -1, i);
    }
}

void GameLevelData::endGame(GameScene *gameScene)
{

}


// ======================================================================
//              【测试模式】所有关卡数据 - 方便快速测试全部僵尸
//  ======================================================================
//  改动说明：
//  1. 所有关卡初始阳光改为 2000
//  2. 所有关卡加入全部僵尸类型，firstFlag=1 让僵尸尽快出现
//  3. flagNum 减小，flagToSumNum 缩短，让僵尸出现更密集
//  注意：flagToSumNum.second 必须比 first 多一个元素（最后一个用于兜底）
//  ======================================================================
//level1-1
GameLevelData_1::GameLevelData_1()
{
    eName = "1";
    cName = tr("Level 1-1");
    backgroundImage = "interface/background1unsodded1.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;  // 入门友好
    canSelectCard = true;
    showScroll = true;
    // 仅第3行可用：LF=1表示可种植/僵尸可通行，0=不可用
    // 行:   0  1  2  3  4  5
    LF = { 0, 0, 0, 1, 0, 0 };
    pName = { "oPeashooter", "oSunflower", "oWallNut", "oCherryBomb", "oSnowPea", "oRepeater" ,"oJalapeno" };
    // 僵尸配置：仅普通僵尸，数量少，间隔长，入门难度
    zombieData = { { "oZombie", 3, 1, {} } };
    flagNum = 3;
    largeWaveFlag = { 3 };
    flagToSumNum = QPair<QList<int>, QList<int>>({ 1, 2 }, { 1, 2, 8 });
}
//level2-1 夜晚
GameLevelData_2::GameLevelData_2()
{
    dKind = 0; // 夜晚
    produceSun = false; // 夜晚不掉落阳光
    backgroundImage = "interface/background2.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;  // 夜晚初始阳光
    canSelectCard = true;
    showScroll = true;
    eName = "6";
    cName = tr("Level 2-1 (Night)");
     pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom", 
        "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead",
        "oGloomShroom","oStarfruit","oPotatoMine","oFlowerPot"
    };
    // 僵尸配置：夜晚关，种类丰富但密度适中（无水域僵尸）
    zombieData = { { "oZombie3", 6, 1, {} },
                   { "oFlagZombie", 0, 1, {} },
                   { "oConeheadZombie", 5, 3, {} },
                   { "oBucketheadZombie", 4, 5, {} },
                   { "oNewspaperZombie", 3, 4, {} },
                   { "oScreenDoorZombie", 2, 6, {} },
                   { "oPoleVaultingZombie", 2, 5, {} },
                   { "oJackinTheBoxZombie", 2, 7, {} } };
    flagNum = 12;
    largeWaveFlag = { 12 };
    flagToSumNum = QPair<QList<int>, QList<int>>({ 3, 5, 7, 9 }, { 1, 2, 3, 4, 20 });
}
//level1-2
GameLevelData_3::GameLevelData_3()
{
    backgroundImage = "interface/background1unsodded2.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;  // 【测试撑杆僵尸】
    canSelectCard = true;
    showScroll = true;
    eName = "2";
    cName = tr("Level 1-2");
    // 仅第2-4行可用：LF=1表示可种植/僵尸可通行，0=不可用
    // 行:   0  1  2  3  4  5
    LF = { 0, 0, 1, 1, 1, 0 };
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb", "oSpikeweed", "oSpikerock" };
    // 僵尸配置：【测试撑杆僵尸】第一波开始出现，便于观察跳跃动画
    zombieData = { { "oZombie", 3, 2, {} },
                   { "oPoleVaultingZombie", 4, 1, {} },  // 第一波就出，数量4只
                   { "oConeheadZombie", 2, 5, {} } };
    flagNum = 8;
    largeWaveFlag = { 8 };
    flagToSumNum = QPair<QList<int>, QList<int>>({ 2, 4, 6 }, { 1, 2, 3, 18 });
}
// ========== Level 1-4 (泳池关) ==========
GameLevelData_4::GameLevelData_4()
{
    eName = "4";
    cName = tr("Level 1-4 (Pool)");
    dKind = 1;
    produceSun = true;
    coord = 1;  // 6行泳池布局
    backgroundImage = "interface/background3.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;  // 【演示】快速部署
    // LF: 0=不可用, 1=陆地, 2=水域
    // 行:   0  1  2  3  4  5  6
    LF = { 0, 1, 1, 2, 2, 1, 1 };

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom",
        "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead",
        "oGloomShroom","oStarfruit","oPotatoMine","oFlowerPot"
    };

    // 僵尸配置：海豚/撑杆前期出现，整体降低数量
    // 陆地僵尸 → LF==1（第 1,2,5,6 行）；水域僵尸 → LF==2（第 3,4 行）
    zombieData = {
        // ---- 陆地僵尸 ----
        { "oZombie",            2, 1, {} },
        { "oFlagZombie",        0, 1, {} },
        { "oConeheadZombie",    1, 2, {} },
        { "oBucketheadZombie",  1, 3, {} },
        { "oPoleVaultingZombie",2, 1, {} },   // 前期撑杆跳
        { "oFootballZombie",    1, 3, {} },   // 足球僵尸
        { "oImp",               2, 1, {} },   // 前期小鬼
        // ---- 水域僵尸（仅本关生成） ----
        { "oDuckyTubeZombie1",      2, 1, {} },
        { "oDuckyTubeZombie2",      2, 2, {} },
        { "oDuckyTubeZombie3",      1, 3, {} },
        { "oSnorkelZombie",         1, 2, {} },
    };

    flagNum = 5;  // 5波快速通关
    largeWaveFlag = { 3, 5 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 1, 2, 3, 4 },
        { 1, 2, 3, 4, 10 }
    );
}

// ========== Level 1-3（草坪关） ==========
GameLevelData_5::GameLevelData_5()
{
    eName = "3";
    cName = tr("Level 1-3");
    dKind = 1;              // 白天（光敏传感器触发黑夜）
    produceSun = true;
    backgroundImage = "interface/background1.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;  // 【演示】快速部署

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom",
        "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead",
        "oGloomShroom","oStarfruit","oPotatoMine","oFlowerPot"
    };

    // 【演示】缩短时长：波数减半，僵尸密度压缩，种类保留
    zombieData = {
        { "oZombie",            3, 1, {} },
        { "oFlagZombie",        0, 1, {} },
        { "oConeheadZombie",    3, 1, {} },
        { "oBucketheadZombie",  2, 2, {} },
        { "oNewspaperZombie",   1, 2, {} },
        { "oScreenDoorZombie",  1, 3, {} },
        { "oPoleVaultingZombie",1, 2, {} },
        { "oFootballZombie",    1, 3, {} },
        { "oImp",               2, 3, {} },
    };

    flagNum = 6;  // 【演示】原值12
    largeWaveFlag = { 3, 6 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 1, 2, 3, 4, 5 },
        { 1, 2, 3, 4, 5, 10 }  // 【演示】缩短间隔
    );
}

// ========== Level 2-2 (Night) ==========
GameLevelData_6::GameLevelData_6()
{
    eName = "7";                       // 内部标识符
    cName = tr("Level 2-2 (Night)");   // 显示名称
    dKind = 0;                         // 夜晚
    produceSun = false;                // 夜晚不掉落阳光
    backgroundImage = "interface/background4.jpg";  // 新背景
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";   // 可更换夜晚音乐
    sunNum = 1000;                      // 夜晚初始阳光较多

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    // 可用植物（夜晚可用的植物，建议包含蘑菇和咖啡豆）
    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea",
        "oPuffShroom", "oScaredyShroom", "oFumeShroom",
        "oTorchwood", "oSplitPea", "oSnowPea",
        "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower"
    };

    // 僵尸配置：夜晚第2关，种类增多，密度提升
    zombieData = {
        { "oZombie3",           6, 1, {} },
        { "oFlagZombie",        0, 1, {} },
        { "oConeheadZombie",    5, 2, {} },
        { "oBucketheadZombie",  4, 4, {} },
        { "oNewspaperZombie",   3, 3, {} },
        { "oScreenDoorZombie",  3, 5, {} },
        { "oPoleVaultingZombie",2, 5, {} },
        { "oFootballZombie",    2, 6, {} },
        { "oJackinTheBoxZombie",2, 6, {} },
        { "oDancingZombie",     1, 8, {} },
        { "oImp",               3, 5, {} },
    };

    flagNum = 14;
    largeWaveFlag = { 7, 14 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 2, 4, 6, 8, 10, 12 },
        { 1, 2, 3, 4, 5, 6, 25 }
    );
}

// ========== Level 1-5 ==========
GameLevelData_7::GameLevelData_7()
{
    eName = "5";
    cName = tr("Level 1-5 (Roof)");
    dKind = 1;              // 白天
    produceSun = true;
    backgroundImage = "interface/background5.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 1000;

    // LF=3 屋顶地面，强制花盆承载规则
    // 行:  0  1  2  3  4  5
    LF = { 0, 3, 3, 3, 3, 3 };

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom", 
        "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead",
        "oGloomShroom","oStarfruit","oPotatoMine","oFlowerPot"
    };

    // 僵尸配置：屋顶关，陆地僵尸密度提高（无水域僵尸）
    zombieData = {
        { "oZombie",            5, 1, {} },
        { "oZombie3",           4, 1, {} },
        { "oFlagZombie",        0, 1, {} },
        { "oConeheadZombie",    5, 2, {} },
        { "oBucketheadZombie",  4, 4, {} },
        { "oScreenDoorZombie",  3, 4, {} },
        { "oNewspaperZombie",   3, 3, {} },
        { "oPoleVaultingZombie",3, 4, {} },
        { "oFootballZombie",    2, 6, {} },
        { "oJackinTheBoxZombie",2, 6, {} },
        { "oDancingZombie",     1, 8, {} },
        { "oImp",               3, 5, {} },
    };

    flagNum = 16;
    largeWaveFlag = { 8, 16 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11, 13 },
        { 1, 2, 3, 4, 5, 6, 25 }
    );
}

// ========== Level 2-3 (Night) - BOSS ==========
GameLevelData_8::GameLevelData_8()
{
    eName = "8";
    cName = tr("Level 2-3 (Night)");
    dKind = 0;                  // 夜晚
    produceSun = false;
    backgroundImage = "interface/background6boss.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";  // 可更换更刺激的音乐
    sunNum = 1000;               // BOSS 关初始阳光较多

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    // BOSS 关可用植物可以更丰富
    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom", 
        "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead",
        "oGloomShroom","oStarfruit","oPotatoMine","oFlowerPot"
    };

    // BOSS 关：全种类陆地僵尸，最高密度，最早刷新，最短间隔
    zombieData = {
        { "oZombie",            8, 1, {} },
        { "oZombie3",           6, 1, {} },
        { "oFlagZombie",        0, 1, {} },
        { "oConeheadZombie",    6, 1, {} },
        { "oBucketheadZombie",  5, 2, {} },
        { "oScreenDoorZombie",  4, 3, {} },
        { "oNewspaperZombie",   4, 2, {} },
        { "oPoleVaultingZombie",3, 3, {} },
        { "oFootballZombie",    3, 4, {} },
        { "oJackinTheBoxZombie",3, 4, {} },
        { "oDancingZombie",     2, 5, {} },
        { "oZomboni",           2, 7, {} },
        { "oImp",               4, 4, {} },
    };

    flagNum = 20;
    largeWaveFlag = { 10, 20 };
    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 2, 4, 6, 8, 10, 12, 14, 16, 18 },
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 30 }
    );
}


GameLevelData *GameLevelDataFactory(const QString &eName)
{
    // 关卡顺序(1→2→3→4→5→6→7→8→菜单):
    // L1-1→L1-2→L1-3→L1-4(Pool)→L1-5→L2-1(N)→L2-2(N)→L2-3(N)
    if (eName == "1") return new GameLevelData_1;
    if (eName == "2") return new GameLevelData_3;
    if (eName == "3") return new GameLevelData_5;
    if (eName == "4") return new GameLevelData_4;
    if (eName == "5") return new GameLevelData_7;
    if (eName == "6") return new GameLevelData_2;
    if (eName == "7") return new GameLevelData_6;
    if (eName == "8") return new GameLevelData_8;
    return nullptr;
}
