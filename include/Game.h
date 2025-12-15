#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <thread> 
#include <mutex> 
#include <atomic> // 用于线程安全的 bool

// 前向声明
class Unit; 
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

// 兵种类型枚举
enum class UnitType {
    KNIGHT,
    GIANT,
    ARCHERS,
    PEKKA,
    VALKYRIE,
    DART_GOBLIN
};

// 游戏难度枚举
enum class Difficulty {
    EASY,   // 简单：圣水恢复慢，反应迟钝
    NORMAL, // 普通：圣水恢复同玩家，正常反应
    HARD    // 困难：圣水恢复快，反应迅速
};

// 卡牌结构体：包含逻辑数据和渲染对象
struct Card {
    UnitType type;          // 兵种类型
    int cost;               // 圣水消耗
    sf::Sprite sprite;      // 卡牌图标
    sf::RectangleShape slotShape; // 卡槽背景(用于显示选中状态)
    sf::FloatRect touchArea; // 点击区域
    sf::Text costText;      //卡牌上显示的费用数字
};

class Game {
public:

    Game();
    ~Game();
    
    // 运行游戏主循环
    void run();

    // 静态常量：定义地图大小
    static const int TILE_SIZE = 40; // 每个格子的像素大小
    static const int ROWS = 19;      // 行
    static const int COLS = 21;      // 列

    // UI 区域高度
    static const int UI_HEIGHT = 160; // 底部预留给卡牌和圣水条的高度

    // 设置游戏难度
    void setDifficulty(Difficulty level);

private:
    // SFML 窗口
    sf::RenderWindow m_window;

    // 地图数据：二维数组
    // 使用 vector<vector<int>> 方便管理，存储的是 TileType 的整数值
    std::vector<std::vector<int>> m_mapData;

    // --- 空间划分优化 ---
    // 一维数组模拟二维网格，索引 = row * COLS + col
    // 每个格子里存储该格子内的单位列表
    std::vector<std::vector<Unit*>> m_spatialGrid;

    // 1. 单位列表
    std::vector<Unit*> m_units; 

    // 2. 子弹列表
    std::vector<Projectile*> m_projectiles;

    // 3. 废墟列表 (存储已被摧毁的塔的废墟图)
    std::vector<sf::Sprite> m_ruins;

    // 4. 背景精灵
    sf::Sprite m_bgSprite;

    // 5. UI 文本
    sf::Text m_gameOverText;
    bool m_gameOver; // 游戏是否结束

    // 6. UI 成员
    sf::RectangleShape m_uiBg; // UI栏背景
    std::vector<Card> m_deck;  // 手牌列表

    // 圣水系统 UI
    sf::RectangleShape m_elixirBarBg; // 圣水槽背景
    sf::RectangleShape m_elixirBarFg; // 圣水槽前景 (紫色)
    sf::Sprite m_elixirIcon;          // 圣水滴图标
    sf::Text m_elixirStatusText;      // 显示 "4/10" 的文字

    // 难度选择 UI
    sf::Text m_difficultyText;

    // 圣水逻辑数值
    float m_elixir;
    float m_maxElixir;
    float m_elixirRate; // 回复速度 (点/秒)

     // 当前选中的卡牌索引 (-1 表示未选中)
    int m_selectedCardIndex;

    // --- AI (敌方) 资源与状态 ---
    float m_enemyElixir;
    float m_enemyMaxElixir;  // 之前漏掉了这个变量的定义
    float m_enemyElixirRate; // 敌方回费速度 (受难度影响)
    float m_aiThinkTimer;    // AI 思考计时器
    float m_aiReactionTime;  // AI 思考间隔 (受难度影响)
    Difficulty m_difficulty;
    
    // 多线程相关 
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

    // AI 核心逻辑
    void updateAI(float dt); 

    void render();
    // 专门负责绘制 UI
    void renderUI();

    // 处理鼠标点击逻辑
    void handleMouseClick(int x, int y);
    
    // 生成单位的辅助函数
    void spawnUnit(UnitType type, float x, float y, int team);
};