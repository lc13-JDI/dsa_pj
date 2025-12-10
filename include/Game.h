#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Unit.h"

// 地形类型枚举
enum TileType {
    GROUND = 0, // 平地 (草绿)
    RIVER,      // 河流 (蓝色)
    BRIDGE,     // 桥梁 (木色)
    MOUNTAIN,   // 山脉 (灰色)
    BASE_A,     // 甲方基地 (红色)
    BASE_B      // 乙方基地 (蓝色)
};

class Game {
public:
    // 构造函数
    Game();
    // 析构函数
    ~Game();
    
    // 运行游戏主循环
    void run();

    // 静态常量：定义地图大小
    static const int TILE_SIZE = 40; // 每个格子的像素大小
    static const int ROWS = 20;      // 20行
    static const int COLS = 15;      // 15列

private:
    // SFML 窗口
    sf::RenderWindow m_window;

    // 地图数据：二维数组
    // 使用 vector<vector<int>> 方便管理，存储的是 TileType 的整数值
    std::vector<std::vector<int>> m_mapData;

    // 1. 单位列表 (使用指针实现多态)
    std::vector<Unit*> m_units; 
    
    // 2. 时间控制
    sf::Clock m_dtClock;  // 创建一个时钟对象
    float m_deltaTime; // 存储时间间隔的变量

    // 初始化功能
    void initWindow();
    void initMap();
    void initUnits(); // 初始化测试单位

    // 核心循环逻辑
    void processEvents();
    void update();
    void render();
};