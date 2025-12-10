#include "Unit.h"
#include "Pathfinder.h" 
#include <cmath> // 用于 sqrt, atan2 等
#include <iostream>

Unit::Unit(float x, float y, Team team) 
    : m_team(team), m_attackTimer(0.f)
{
    // 默认属性 (作为一个兜底，子类会覆盖它)
    m_hp = 100.f;
    m_maxHp = 100.f;
    m_atk = 10.f;
    m_speed = 60.f; // 每秒移动 60 像素
    m_range = 60.f; 
    m_attackInterval = 1.0f; // 默认1秒打一次

    // 设置图形外观
    m_shape.setSize(sf::Vector2f(30.f, 30.f)); // 30x30 的方块
    m_shape.setOrigin(15.f, 15.f); // 设置中心点为原点
    m_shape.setPosition(x, y);

    // 根据阵营设置颜色
    if (m_team == TEAM_A) {
        m_shape.setFillColor(sf::Color::Red);
        // 加上黑色边框区分
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color::Black);
    } else {
        m_shape.setFillColor(sf::Color::Blue);
        m_shape.setOutlineThickness(2.f);
        m_shape.setOutlineColor(sf::Color::White);
    }
}

// 扣血逻辑
void Unit::takeDamage(float damage) {
    m_hp -= damage;
    // 简单的受击反馈：闪烁一下(变白) - 实际项目中最好用计时器控制颜色恢复
    // 这里为了简化，只做扣血
    if (m_hp < 0) m_hp = 0;
}

// 寻找最近敌人
Unit* Unit::findClosestEnemy(const std::vector<Unit*>& allUnits) {
    Unit* closest = nullptr;
    float minDist = 99999.f;

    for (auto other : allUnits) {
        if (!other) continue;
        if (other == this) continue;
        if (other->isDead()) continue;
        if (other->getTeam() == this->getTeam()) continue; // 不打队友

        sf::Vector2f diff = other->getPosition() - this->getPosition();
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        if (dist < minDist) {
            minDist = dist;
            closest = other;
        }
    }
    return closest;
}

// 【核心 AI 逻辑】
void Unit::update(float dt, const std::vector<Unit*>& allUnits) {
    // 0. 更新攻击计时器
    if (m_attackTimer > 0) m_attackTimer -= dt;

    // 1. 尝试寻找敌人
    Unit* target = findClosestEnemy(allUnits);
    float distToEnemy = 99999.f;

    if (target) {
        sf::Vector2f diff = target->getPosition() - this->getPosition();
        distToEnemy = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    }

    // 2. 决策：打还是走？
    if (target && distToEnemy <= m_range) {
        // --- 状态：攻击 ---
        // 既然在射程内，就不移动了，直接开火
        if (m_attackTimer <= 0) {
            target->takeDamage(m_atk);
            m_attackTimer = m_attackInterval; // 重置冷却
            // std::cout << "Unit attacked! Target HP: " << target->m_hp << std::endl;
        }
    } else {
        // --- 状态：移动 ---
        // 没敌人或者敌人在射程外，继续赶路
        if (!m_pathQueue.empty()) {
            followPath(dt);
        }
    }
}

// 计算路径
void Unit::setTarget(float tx, float ty, const std::vector<std::vector<int>>& mapData) {
    // 1. 将 像素坐标 转换为 网格坐标
    sf::Vector2f startPos = m_shape.getPosition();
    
    int startCol = static_cast<int>(startPos.x) / Game::TILE_SIZE;
    int startRow = static_cast<int>(startPos.y) / Game::TILE_SIZE;
    
    int endCol = static_cast<int>(tx) / Game::TILE_SIZE;
    int endRow = static_cast<int>(ty) / Game::TILE_SIZE;

    sf::Vector2i startNode(startCol, startRow);
    sf::Vector2i endNode(endCol, endRow);

    // 2. 调用 BFS 算法
    std::vector<sf::Vector2i> gridPath = Pathfinder::findPath(mapData, startNode, endNode);

    // 3. 将 网格路径 转换回 像素中心点，存入队列
    m_pathQueue.clear();
    for (const auto& node : gridPath) {
        // 目标点应该是格子的中心： col * 40 + 20
        float worldX = node.x * Game::TILE_SIZE + Game::TILE_SIZE / 2.0f;
        float worldY = node.y * Game::TILE_SIZE + Game::TILE_SIZE / 2.0f;
        m_pathQueue.push_back(sf::Vector2f(worldX, worldY));
    }
}

void Unit::followPath(float dt) {
    // 获取当前要去的下一个小目标点
    sf::Vector2f target = m_pathQueue.front();
    sf::Vector2f current = m_shape.getPosition();
    sf::Vector2f dir = target - current;

    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    // 如果非常接近当前小目标 (比如小于 2 像素)
    if (dist < 2.0f) {
        // 到达该点，弹出，准备去下一个点
        m_pathQueue.pop_front();
        return; 
    }

    // 正常移动
    sf::Vector2f normDir = dir / dist;
    m_shape.move(normDir * m_speed * dt);
}

void Unit::render(sf::RenderWindow& window) {
    window.draw(m_shape);
}

void Unit::moveTowardsTarget(float dt) {
    sf::Vector2f currentPos = m_shape.getPosition();
    sf::Vector2f direction = m_targetPos - currentPos;

    // 计算距离
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    // 如果非常接近目标，就停止 (避免抖动)
    if (distance < 5.0f) {
        m_hasTarget = false;
        return;
    }

    // 归一化方向向量 (变成长度为1的向量)
    sf::Vector2f normalizedDir = direction / distance;

    // 移动公式：新位置 = 旧位置 + 方向 * 速度 * 时间增量
    sf::Vector2f newPos = currentPos + normalizedDir * m_speed * dt;
    m_shape.setPosition(newPos);
}

// --- 子类实现 (多态的具体体现) ---

// 1. Tank: 血厚、慢速、体型大
Tank::Tank(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 300.f;
    m_maxHp = 300.f;
    m_atk = 20.f;
    m_speed = 30.f; // 移动很慢
    m_range = 60.f;
    m_attackInterval = 1.5f; // 攻速慢
    

    m_shape.setSize(sf::Vector2f(50.f, 50.f)); // 体型很大
    m_shape.setOrigin(25.f, 25.f);

    if (team == TEAM_A) {
        m_shape.setFillColor(sf::Color(180, 0, 0)); // 深红
        m_shape.setOutlineColor(sf::Color::Black);
    } else {
        m_shape.setFillColor(sf::Color(0, 0, 180)); // 深蓝
        m_shape.setOutlineColor(sf::Color::White);
    }
    m_shape.setOutlineThickness(3.f); // 轮廓更粗，看起来更硬
}

// 2. Melee: 平衡
Melee::Melee(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 120.f;
    m_maxHp = 120.f;
    m_atk = 15.f;
    m_speed = 60.f; // 速度中等
    m_range = 50.f;
    m_attackInterval = 1.0f;

    m_shape.setSize(sf::Vector2f(30.f, 30.f)); // 标准体型
    m_shape.setOrigin(15.f, 15.f);

    if (team == TEAM_A) {
        m_shape.setFillColor(sf::Color::Red);
        m_shape.setOutlineColor(sf::Color::Black);
    } else {
        m_shape.setFillColor(sf::Color::Blue);
        m_shape.setOutlineColor(sf::Color::White);
    }
    m_shape.setOutlineThickness(2.f);
}

// 3. Ranged: 血少、快速、体型小
Ranged::Ranged(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 60.f;
    m_maxHp = 60.f;
    m_atk = 10.f;
    m_speed = 90.f; // 跑得快
    m_range = 150.f; // 射程远
    m_attackInterval = 0.8f; // 攻速快


    m_shape.setSize(sf::Vector2f(20.f, 20.f)); // 体型小
    m_shape.setOrigin(10.f, 10.f);

    if (team == TEAM_A) {
        m_shape.setFillColor(sf::Color(255, 100, 100)); // 浅红/粉红
        m_shape.setOutlineColor(sf::Color::Black);
    } else {
        m_shape.setFillColor(sf::Color(100, 100, 255)); // 浅蓝
        m_shape.setOutlineColor(sf::Color::White);
    }
    m_shape.setOutlineThickness(1.f);
}