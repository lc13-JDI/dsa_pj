#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <deque> // 使用双端队列存储路径
#include "Game.h" // 需要知道 TILE_SIZE

// 阵营枚举
enum Team {
    TEAM_A, // 红色方 (上方)
    TEAM_B  // 蓝色方 (下方)
};

// --- 基类 Unit ---
class Unit {
public:
    Unit(float x, float y, Team team);
    virtual ~Unit() {} 

    // 核心函数
    // dt = delta time (上一帧到这一帧经过的时间，秒)
    virtual void update(float dt, const std::vector<Unit*>& allUnits); 
    virtual void render(sf::RenderWindow& window);

    // 设置移动目标
    void setTarget(float tx, float ty, const std::vector<std::vector<int>>& mapData);
    
    // 获取状态
    bool isAlive() const { return m_hp > 0; }
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }

    Team getTeam() const { return m_team; }

    // 战斗接口
    void takeDamage(float damage);
    bool isDead() const { return m_hp <= 0; }

protected:
    sf::RectangleShape m_shape; // 暂时用方块代表士兵
    
    Team m_team;
    float m_hp;
    float m_maxHp; // 记录最大血量
    float m_atk;
    float m_speed;   // 像素/秒
    float m_range;   // 攻击距离

    // 攻击冷却
    float m_attackInterval; // 攻击间隔(秒)
    float m_attackTimer;    // 计时器


    // 移动相关
    sf::Vector2f m_targetPos;
    bool m_hasTarget;

    // 辅助函数：移动逻辑
    void moveTowardsTarget(float dt);

    // --- 寻路相关 ---
    // 存储一系列要走过的世界坐标点
    std::deque<sf::Vector2f> m_pathQueue; 
    
    // 辅助：沿着路径移动
    void followPath(float dt);

    // 寻找最近敌人的辅助函数
    Unit* findClosestEnemy(const std::vector<Unit*>& allUnits);
};

// --- 派生类 1: Tank (肉盾/巨人) ---
class Tank : public Unit {
public:
    Tank(float x, float y, Team team);
};

// --- 派生类 2: Melee (近战/骑士) ---
class Melee : public Unit {
public:
    Melee(float x, float y, Team team);
};

// --- 派生类 3: Ranged (远程/弓箭手) ---
class Ranged : public Unit {
public:
    Ranged(float x, float y, Team team);
};