#include "Game.h"
#include <iostream>

int main()
{
    std::cout << "[Debug] Starting Battle Simulation..." << std::endl;

    // 创建并运行游戏实例
    Game game;
    game.run();

    return 0;
}