#pragma once
#include "Unit.h"

// 塔的类型
enum class TowerType {
    PRINCESS, // 公主塔 (小基地)
    KING      // 国王塔 (大基地)
};

class Tower : public Unit {
public:
    Tower(float x, float y, Team team, TowerType type);
    virtual ~Tower() {}

    // 重写 update：塔不移动，但会发射子弹
    virtual void update(float dt, const std::vector<Unit*>& allUnits, std::vector<Projectile*>& projectiles, const std::vector<std::vector<int>>& mapData) override;

    // 判断是否为国王塔
    bool isKing() const { return m_type == TowerType::KING; }

private:
    TowerType m_type;
    
    // 塔的攻击逻辑
    void shoot(Unit* target, std::vector<Projectile*>& projectiles);
};