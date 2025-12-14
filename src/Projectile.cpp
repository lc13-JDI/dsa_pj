#include "Projectile.h"
#include "ResourceManager.h"
#include <cmath>
#include <iostream>

// 常量 PI，用于计算旋转角度
const float PI = 3.14159265f;

Projectile::Projectile(float startX, float startY, Unit* target, float damage)
    : m_target(target), m_damage(damage), m_speed(300.f), m_active(true)
{
    // 1. 加载子弹纹理 (确保 ResourceManager 已加载 "bullet")
    // 如果 bullet 纹理没加载成功，这里会显示紫色方块
    m_sprite.setTexture(ResourceManager::getInstance().getTexture("bullet"));
    
    // 2. 设置原点为图片中心，方便旋转和定位
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    
    // 3. 设置初始位置
    m_sprite.setPosition(startX, startY);

    // 设置缩放（如果子弹图片过大）
    m_sprite.setScale(1.0f, 1.0f);
}

Projectile::~Projectile() {
    // 可以在这里播放爆炸特效或音效
}

void Projectile::update(float dt) {
    if (!m_active) return;

    // 1. 检查目标是否失效
    // 如果目标指针为空，或者目标已经死亡，子弹失效（简单的处理方式）
    if (!m_target || m_target->isDead()) {
        m_active = false;
        return;
    }

    // 2. 计算从当前位置指向目标的方向向量
    sf::Vector2f myPos = m_sprite.getPosition();
    sf::Vector2f targetPos = m_target->getPosition(); // 获取目标中心位置
    sf::Vector2f dir = targetPos - myPos;
    
    // 计算距离
    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    // 3. 碰撞检测 (如果距离小于 10 像素，视为击中)
    if (dist < 10.f) {
        // 对目标造成伤害
        m_target->takeDamage(m_damage);
        // 标记子弹为非活跃，将在 Game::update 中被移除
        m_active = false; 
        return;
    }

    // 4. 移动逻辑
    // 归一化方向向量 (Unit Vector)
    sf::Vector2f normDir = dir / dist;
    // 位移 = 方向 * 速度 * 时间
    m_sprite.move(normDir * m_speed * dt);
    
    // // 5. 让子弹头部朝向目标 (简单的旋转计算)
    // float angle = std::atan2(dir.y, dir.x) * 180.f / PI;
    // m_sprite.setRotation(angle);
}

void Projectile::render(sf::RenderWindow& window) {
    if (m_active) {
        window.draw(m_sprite);
    }
}