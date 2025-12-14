#pragma once
#include <SFML/Graphics.hpp>
#include "Unit.h"

class Projectile {
public:
    // 构造函数：
    // startX, startY: 子弹生成的起始坐标
    // target: 追踪的目标单位指针
    // damage: 造成的伤害值
    Projectile(float startX, float startY, Unit* target, float damage = 10.f);
    ~Projectile();

    // 每帧更新：计算飞行、碰撞检测
    void update(float dt);
    
    // 渲染
    void render(sf::RenderWindow& window);
    
    // 检查子弹是否还处于活跃状态（是否击中或失效）
    bool isActive() const { return m_active; }

private:
    sf::Sprite m_sprite;
    Unit* m_target; // 追踪的目标
    float m_speed;  // 飞行速度
    float m_damage; // 伤害值
    bool m_active;  // true=飞行中, false=应被销毁
};