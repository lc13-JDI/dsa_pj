#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>

// 阵营枚举
enum Team {
    TEAM_A, // 红色方 (上方)
    TEAM_B  // 蓝色方 (下方)
};

// --- 基类 Unit ---
class Unit {
public:
    // 构造函数：需要位置、阵营、颜色
    Unit(float x, float y, Team team);
    virtual ~Unit() {} // 虚析构函数，保证子类正确销毁

    // 核心函数
    // dt = delta time (上一帧到这一帧经过的时间，秒)
    virtual void update(float dt); 
    virtual void render(sf::RenderWindow& window);

    // 设置移动目标
    void setTarget(float x, float y);
    
    // 获取状态
    bool isAlive() const { return m_hp > 0; }
    sf::Vector2f getPosition() const { return m_shape.getPosition(); }

protected:
    sf::RectangleShape m_shape; // 暂时用方块代表士兵
    
    Team m_team;
    float m_hp;
    float m_maxHp; // 记录最大血量
    float m_atk;
    float m_speed;   // 像素/秒
    float m_range;   // 攻击距离

    // 移动相关
    sf::Vector2f m_targetPos;
    bool m_hasTarget;

    // 辅助函数：移动逻辑
    void moveTowardsTarget(float dt);
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