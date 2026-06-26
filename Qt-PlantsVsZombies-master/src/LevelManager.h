//
// LevelManager — manages win/lose conditions and level transitions
//

#ifndef PLANTS_VS_ZOMBIES_LEVELMANAGER_H
#define PLANTS_VS_ZOMBIES_LEVELMANAGER_H

#include <QtCore>
#include <QtWidgets>

class GameScene;
class GameLevelData;

enum class GameState {
    PLAYING,
    WIN,
    LOSE
};

class LevelManager : public QObject
{
    Q_OBJECT

public:
    LevelManager(GameScene *scene, GameLevelData *levelData);

    // Called every monitor tick; returns current game state
    GameState checkWinLose();

    // Whether the game is still running
    bool isPlaying() const { return state == GameState::PLAYING; }

    GameState getState() const { return state; }

private:
    void triggerLose();
    void triggerWin();

    bool hasActiveLawnmowerOnRow(int row) const;
    bool allWavesSpawned() const;
    bool allZombiesDead() const;

    GameScene *scene;
    GameLevelData *levelData;
    GameState state;
    bool gameOver;
};

#endif // PLANTS_VS_ZOMBIES_LEVELMANAGER_H
