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
    sunNum = 150;  // 入门友好
    canSelectCard = true;
    showScroll = true;
    // 仅第3行可用：LF=1表示可种植/僵尸可通行，0=不可用
    // 行:   0  1  2  3  4  5
    LF = { 0, 0, 0, 1, 0, 0 };
    pName = { "oPeashooter", "oSunflower", "oWallNut", "oCherryBomb", "oSnowPea", "oRepeater" ,"oJalapeno" };
    // 仅普通僵尸，数量少
    zombieData = { { "oZombie", 3, 1, {} } };//从第一波起共3个普通僵尸
    flagNum = 3;
    largeWaveFlag = { 3 };
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2 }, { 1, 2, 5 });
}
//level2-1 夜晚
GameLevelData_2::GameLevelData_2()
{
    dKind = 0; // 夜晚
    produceSun = false; // 夜晚不掉落阳光
    backgroundImage = "interface/background2.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 2000;  // 【测试】原值: 200
    canSelectCard = true;
    showScroll = true;
    eName = "6";
    cName = tr("Level 2-1 (Night)");
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb" };
    // 【测试】原值 zombieData: { {"oZombie3",3,1,{}}, {"oConeheadZombie",5,3,{}}, {"oBucketheadZombie",4,5,{}}, {"oNewspaperZombie",3,4,{}} }
    zombieData = { { "oZombie3", 2, 1, {} },
                   { "oFlagZombie", 1, 1, {} },
                   { "oConeheadZombie", 2, 1, {} },
                   { "oBucketheadZombie", 2, 1, {} },
                   { "oPoleVaultingZombie", 2, 1, {} },
                   { "oNewspaperZombie", 2, 1, {} },
                   { "oFootballZombie", 1, 2, {} },
                   { "oScreenDoorZombie", 2, 1, {} },
                   { "oJackinTheBoxZombie", 2, 1, {} },
                   { "oDancingZombie", 1, 2, {} },
                   { "oSnorkelZombie", 2, 1, {} },
                   { "oDolphinRiderZombie", 2, 1, {} },
                   { "oZomboni", 1, 3, {} },
                   { "oImp", 3, 1, {} },
                   { "oDuckyTubeZombie1", 2, 1, {} },
                   { "oDuckyTubeZombie2", 2, 1, {} },
                   { "oDuckyTubeZombie3", 2, 1, {} } };
    flagNum = 8;  // 【测试】原值: 12
    largeWaveFlag = { 8 };  // 【测试】原值: { 12 }
    // 【测试】原值: ({ 3, 5, 7, 9 }, { 1, 2, 3, 4, 20 })
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2, 3, 4, 5, 6, 7 }, { 1, 1, 2, 2, 3, 3, 5, 10 });
}
//level1-2
GameLevelData_3::GameLevelData_3()
{
    backgroundImage = "interface/background1unsodded2.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 2000;  // 【测试】原值: 50
    canSelectCard = true;
    showScroll = true;
    eName = "2";
    cName = tr("Level 1-2");
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb", "oSpikeweed", "oSpikerock" };
    // 【测试】原值 zombieData: { {"oZombie3",3,1,{}}, {"oConeheadZombie",6,3,{}}, {"oBucketheadZombie",5,5,{}}, {"oFootballZombie",2,7,{}} }
    zombieData = { { "oZombie3", 2, 1, {} },
                   { "oFlagZombie", 1, 1, {} },
                   { "oConeheadZombie", 2, 1, {} },
                   { "oBucketheadZombie", 2, 1, {} },
                   { "oPoleVaultingZombie", 2, 1, {} },
                   { "oNewspaperZombie", 2, 1, {} },
                   { "oFootballZombie", 1, 2, {} },
                   { "oScreenDoorZombie", 2, 1, {} },
                   { "oJackinTheBoxZombie", 2, 1, {} },
                   { "oDancingZombie", 1, 2, {} },
                   { "oSnorkelZombie", 2, 1, {} },
                   { "oDolphinRiderZombie", 2, 1, {} },
                   { "oZomboni", 1, 3, {} },
                   { "oImp", 3, 1, {} },
                   { "oDuckyTubeZombie1", 2, 1, {} },
                   { "oDuckyTubeZombie2", 2, 1, {} },
                   { "oDuckyTubeZombie3", 2, 1, {} } };
    flagNum = 10;  // 【测试】原值: 15
    largeWaveFlag = { 8, 10 };  // 【测试】原值: { 8, 15 }
    // 【测试】原值: ({ 3, 5, 7, 9, 11, 13 }, { 1, 2, 3, 4, 5, 6, 30 })
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2, 3, 4, 5, 6, 7, 8, 9 }, { 1, 1, 2, 2, 3, 3, 5, 5, 8, 10 });
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
    sunNum = 100;
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
        "oHypnoShroom", "oTwinSunflower", "oLilyPad", "oSeaShroom", "oTangleKlep", "oSpikeweed", "oSpikerock", "oPumpkinHead"
    };

    // 陆地僵尸（canPass → LF==1，生成在 1,2,5,6 行）
    // 水域僵尸（canPass → LF==2，生成在 3,4 行）
    zombieData = {
        // ---- 陆地僵尸 ----
        { "oZombie",        6, 1, {} },
        { "oFlagZombie",    0, 1, {} },    // 仅大波出现
        { "oConeheadZombie",4, 3, {} },
        { "oBucketheadZombie", 3, 5, {} },
        { "oScreenDoorZombie", 2, 4, { 7, 9 } },
        { "oNewspaperZombie",  2, 6, {} },
        // ---- 水域僵尸 ----
        { "oDuckyTubeZombie1",   4, 1, {} },
        { "oDuckyTubeZombie2",   3, 3, {} },
        { "oDuckyTubeZombie3",   2, 5, {} },
        { "oSnorkelZombie",      3, 2, {} },
        { "oDolphinRiderZombie", 2, 4, {} },
    };

    flagNum = 15;
    largeWaveFlag = { 8, 15 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11 },
        { 1, 2, 3, 4, 5, 25 }
    );
}

// ========== Level 1-3（草坪关） ==========
GameLevelData_5::GameLevelData_5()
{
    eName = "3";
    cName = tr("Level 1-3");
    dKind = 1;
    produceSun = true;
    backgroundImage = "interface/background1.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 100;

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    // 与第4关相同，可增加难度（例如增加僵尸数量或提前出现）
    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom",
        "oScaredyShroom", "oFumeShroom", "oTorchwood", "oSplitPea",
        "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower", "oSpikeweed", "oSpikerock", "oPumpkinHead"
    };

    zombieData = {
        { "oZombie", 10, 1, {} },
        { "oConeheadZombie", 8, 3, {} },
        { "oBucketheadZombie", 5, 5, {} },
        { "oScreenDoorZombie", 4, 4, { 6, 8, 10 } }
    };

    flagNum = 18;
    largeWaveFlag = { 9, 18 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11, 13 },
        { 1, 2, 3, 4, 5, 6, 30 }
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
    sunNum = 200;                      // 夜晚初始阳光较多

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

    // 僵尸配置（比 Level 2-1 更难的变体）
    zombieData = {
        { "oZombie", 10, 1, {} },
        { "oConeheadZombie", 8, 3, {} },
        { "oBucketheadZombie", 6, 5, {} },
        { "oScreenDoorZombie", 4, 4, { 6, 8, 10 } },
        { "oFlagZombie", 0, 0, {} }   // 旗帜僵尸自动在大波出现，无需手动配置
    };

    flagNum = 18;
    largeWaveFlag = { 9, 18 };

    // 波数→等级和映射
    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11, 13 },
        { 1, 2, 3, 4, 5, 6, 30 }
    );
}

// ========== Level 1-5 ==========
GameLevelData_7::GameLevelData_7()
{
    eName = "5";
    cName = tr("Level 1-5");
    dKind = 1;              // 白天
    produceSun = true;
    backgroundImage = "interface/background5.jpg";  // 新背景
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 100;

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea",
        "oPuffShroom", "oScaredyShroom", "oFumeShroom",
        "oTorchwood", "oSplitPea", "oSnowPea",
        "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower"
    };

    // 僵尸数据（难度介于 Level 1-4 和 Level 1-5 之间，可自行调整）
    zombieData = {
        { "oZombie", 9, 1, {} },
        { "oConeheadZombie", 7, 3, {} },
        { "oBucketheadZombie", 5, 5, {} },
        { "oScreenDoorZombie", 3, 4, { 6, 8, 10 } }
    };

    flagNum = 16;
    largeWaveFlag = { 8, 16 };

    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11 },
        { 1, 2, 3, 4, 5, 25 }
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
    sunNum = 250;               // BOSS 关初始阳光较多

    canSelectCard = true;
    showScroll = true;
    hasShovel = true;
    maxSelectedCards = 8;

    // BOSS 关可用植物可以更丰富
    pName = {
        "oPeashooter", "oRepeater", "oGatlingPea",
        "oPuffShroom", "oScaredyShroom", "oFumeShroom",
        "oTorchwood", "oSplitPea", "oSnowPea",
        "oSunflower", "oWallNut", "oCherryBomb",
        "oJalapeno", "oSquash", "oSunShroom", "oDoomShroom",
        "oCoffeeBean", "oChomper", "oTallnut", "oThreepeater",
        "oHypnoShroom", "oTwinSunflower"
    };

    // BOSS 关僵尸配置（数量多、种类丰富、出现早）
    zombieData = {
        { "oZombie", 15, 1, {} },
        { "oConeheadZombie", 12, 2, {} },
        { "oBucketheadZombie", 8, 3, {} },
        { "oScreenDoorZombie", 6, 4, { 5, 7, 9, 11 } }
    };

    flagNum = 22;               // 更多波数
    largeWaveFlag = { 10, 20 }; // 两次大波
    flagToSumNum = QPair<QList<int>, QList<int>>(
        { 3, 5, 7, 9, 11, 13, 15 },
        { 1, 2, 3, 4, 5, 6, 7, 35 }
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
