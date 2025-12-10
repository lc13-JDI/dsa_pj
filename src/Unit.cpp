#include "Unit.h"
#include <cmath> // 用于 sqrt, atan2 等

Unit::Unit(float x, float y, Team team) 
    : m_team(team), m_hasTarget(false)
{
    // 默认属性 (作为一个兜底，子类会覆盖它)
    m_hp = 100.f;
    m_maxHp = 100.f;
    m_atk = 10.f;
    m_speed = 60.f; // 每秒移动 60 像素
    m_range = 50.f;

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

void Unit::setTarget(float x, float y) {
    m_targetPos = sf::Vector2f(x, y);
    m_hasTarget = true;
}

void Unit::update(float dt) {
    if (m_hasTarget) {
        moveTowardsTarget(dt);
    }
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