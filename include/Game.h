#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <thread> 
#include <mutex> 
#include <atomic> // 用于线程安全的 bool


class Unit; // 前向声明
class Projectile;

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
    static const int COLS = 25;      // 25列

private:
    // SFML 窗口
    sf::RenderWindow m_window;

    // 地图数据：二维数组
    // 使用 vector<vector<int>> 方便管理，存储的是 TileType 的整数值
    std::vector<std::vector<int>> m_mapData;

    // 1. 单位列表 (使用指针实现多态)
    std::vector<Unit*> m_units; 

    // 2. 子弹列表
    std::vector<Projectile*> m_projectiles;

    // 废墟列表 (存储已被摧毁的塔的废墟图)
    std::vector<sf::Sprite> m_ruins;

    // 背景精灵
    sf::Sprite m_bgSprite;

    // UI 文本
    sf::Text m_gameOverText;
    bool m_gameOver; // 游戏是否结束
    
    // --- 多线程相关 ---
    std::thread m_logicThread; // 逻辑线程
    std::mutex m_mutex;        // 数据锁 (保护 m_units)
    std::atomic<bool> m_running; // 线程运行标志
    
    // 逻辑循环函数 (将在单独线程中运行)
    void logicLoop();

    // 初始化功能
    void initWindow();
    void initMap();
    void initUnits(); // 初始化测试单位
    void initTowers();
    void initUI(); // 初始化 UI 元素

    // 核心循环逻辑
    void processEvents();
   // update 函数不再被主线程调用，而是被 logicLoop 调用
    void update(float dt); 
    void render();
};