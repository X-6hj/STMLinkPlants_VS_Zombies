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

GameLevelData_1::GameLevelData_1()
{
    backgroundImage = "interface/background1.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 2000;  // 【测试】原值: 100
    canSelectCard = true;
    showScroll = true;
    eName = "1";
    cName = tr("Level 1-1");
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb", "oPotatoMine" };
    // 【测试】原值 zombieData: { {"oZombie3",1,1,{}}, {"oConeheadZombie",3,3,{}}, {"oBucketheadZombie",3,3,{}}, {"oPoleVaultingZombie",2,5,{}} }
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
                   { "oZomboni", 1, 3, {} } };
    flagNum = 8;  // 【测试】原值: 10
    largeWaveFlag = { 8 };  // 【测试】原值: { 10 }
    // 【测试】原值: ({ 3, 5, 9 }, { 1, 2, 3, 15 })
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2, 3, 4, 5, 6, 7 }, { 1, 1, 2, 2, 3, 3, 5, 10 });
}

GameLevelData_2::GameLevelData_2()
{
    dKind = 0; // 夜晚
    produceSun = false; // 夜晚不掉落阳光
    backgroundImage = "interface/background2.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 2000;  // 【测试】原值: 200
    canSelectCard = true;
    showScroll = true;
    eName = "2";
    cName = tr("Level 2-1 (Night)");
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb", "oPotatoMine" };
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
                   { "oZomboni", 1, 3, {} } };
    flagNum = 8;  // 【测试】原值: 12
    largeWaveFlag = { 8 };  // 【测试】原值: { 12 }
    // 【测试】原值: ({ 3, 5, 7, 9 }, { 1, 2, 3, 4, 20 })
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2, 3, 4, 5, 6, 7 }, { 1, 1, 2, 2, 3, 3, 5, 10 });
}

GameLevelData_3::GameLevelData_3()
{
    backgroundImage = "interface/background3.jpg";
    backgroundMusic = "qrc:/audio/UraniwaNi.mp3";
    sunNum = 2000;  // 【测试】原值: 50
    canSelectCard = true;
    showScroll = true;
    eName = "3";
    cName = tr("Level 1-3");
    pName = { "oPeashooter", "oRepeater", "oGatlingPea", "oPuffShroom", "oScaredyShroom", "oFumeShroom", "oSnowPea", "oSunflower", "oWallNut", "oCherryBomb", "oPotatoMine" };
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
                   { "oZomboni", 1, 3, {} } };
    flagNum = 10;  // 【测试】原值: 15
    largeWaveFlag = { 8, 10 };  // 【测试】原值: { 8, 15 }
    // 【测试】原值: ({ 3, 5, 7, 9, 11, 13 }, { 1, 2, 3, 4, 5, 6, 30 })
    flagToSumNum = QPair<QList<int>, QList<int> >({ 1, 2, 3, 4, 5, 6, 7, 8, 9 }, { 1, 1, 2, 2, 3, 3, 5, 5, 8, 10 });
}

GameLevelData *GameLevelDataFactory(const QString &eName)
{
    if (eName == "1")
        return new GameLevelData_1;
    if (eName == "2")
        return new GameLevelData_2;
    if (eName == "3")
        return new GameLevelData_3;
    return nullptr;
}
