#include "Tower.h"
#include "Projectile.h"
#include "ResourceManager.h"
#include <iostream>
#include <cmath>

// 辅助函数：获取一个静态的 1x1 白色纹理
// 用于制作纯色块或透明碰撞盒，无需加载外部图片
static sf::Texture& getBlankTexture() {
    static sf::Texture tex;
    static bool loaded = false;
    if (!loaded) {
        sf::Image img;
        img.create(1, 1, sf::Color::White); // 创建1个白色像素
        tex.loadFromImage(img);
        loaded = true;
    }
    return tex;
}

Tower::Tower(float x, float y, Team team, TowerType type)
    : Unit(x, y, team), m_type(type)
{
    m_speed = 0.f; // 塔不能移动

    // 定义塔的碰撞体积大小 (像素)
    float width = 0;
    float height = 0;

    if (m_type == TowerType::PRINCESS) {
        // 公主塔数值
        m_maxHp = 1400.f;
        m_atk = 50.f;
        m_range = 250.f; 
        m_attackInterval = 0.8f;
        
        // 碰撞盒大小
        width = 80.f;
        height = 80.f;
    } else { // KING
        // 国王塔数值
        m_maxHp = 2400.f;
        m_atk = 70.f;
        m_range = 280.f;
        m_attackInterval = 1.0f;

        // 国王塔稍大
        width = 100.f;
        height = 100.f;
    }
    
    m_hp = m_maxHp;

    // --- 设置隐形外观 ---
    // 1. 使用 1x1 白点纹理
    m_sprite.setTexture(getBlankTexture());
    m_sprite.setTextureRect(sf::IntRect(0, 0, 1, 1));
    
    // 2. 设置原点居中 (0.5, 0.5)
    m_sprite.setOrigin(0.5f, 0.5f);
    
    // 3. 拉伸到目标大小 (例如 80倍)
    setScale(width, height);

    // 4. 【关键】设置为完全透明
    // 平时不可见，只作为逻辑实体存在，点击或攻击检测都基于这个矩形范围
    m_sprite.setColor(sf::Color::Transparent); 
}

void Tower::update(float dt, const std::vector<Unit*>& allUnits, std::vector<Projectile*>& projectiles) {
    // 0. 受击闪烁恢复逻辑
    // 当 Unit::takeDamage 被调用时，Sprite 会变成纯红色 (255, 0, 0, 255)
    // 我们需要检测这种情况，把它改成半透明红色，然后慢慢淡出变回透明
    
    sf::Color c = getSprite().getColor();
    
    // 检测是否刚刚受击（变成了不透明红色）
    if (c.r == 255 && c.g == 0 && c.a == 255) {
        c.a = 120; // 设置为半透明，制造“受击高亮”效果，但不遮挡背景太严重
        getSprite().setColor(c);
    } 
    // 如果当前不是完全透明，说明正在闪烁，慢慢淡出
    else if (c.a > 0) {
        // 淡出速度
        int fadeSpeed = static_cast<int>(300 * dt); 
        if (c.a > fadeSpeed) c.a -= fadeSpeed;
        else c.a = 0; // 归零，变回完全透明
        getSprite().setColor(c);
    }

    // 1. 攻击冷却
    if (m_attackTimer > 0) m_attackTimer -= dt;

    // 2. 寻找敌人
    Unit* target = findClosestEnemy(allUnits);
    
    // 3. 攻击逻辑
    if (target) {
        sf::Vector2f diff = target->getPosition() - getPosition();
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        if (dist <= m_range) {
            // 在射程内，且冷却完毕
            if (m_attackTimer <= 0) {
                shoot(target, projectiles);
                m_attackTimer = m_attackInterval;
            }
        }
    }
}

void Tower::shoot(Unit* target, std::vector<Projectile*>& projectiles) {
    // 发射子弹
    // 为了视觉效果，让子弹从塔的“顶部”飞出 (y - 30 像素)
    // 这样看起来更有立体感
    Projectile* p = new Projectile(getPosition().x, getPosition().y - 30.f, target, m_atk);
    projectiles.push_back(p);
}