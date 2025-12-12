#include "Movable.h"
#include <cmath>

const float PI = 3.14159265f;

Movable::Movable() 
    : m_animationTimer(0.f), m_currentFrame(0), 
      m_currentRow(0), m_isFlipped(false) 
{
    // 默认映射，防止未初始化崩溃
    m_walkRowMap = {0, 1, 2, 3, 4};
    m_attackRowMap = {5, 6, 7, 8, 9};
}

Movable::~Movable() {}

void Movable::initSprite(const sf::Texture& texture, const AnimInfo& info) {
    m_sprite.setTexture(texture);
    m_animInfo = info;

    // 设置原点为底部中心
    m_sprite.setOrigin(info.frameWidth / 2.0f, info.frameHeight / 2.0f);
    // 初始裁剪
    m_sprite.setTextureRect(sf::IntRect(0, 0, info.frameWidth, info.frameHeight));
}

void Movable::setWalkRows(int up, int upRight, int right, int downRight, int down) {
    m_walkRowMap = {up, upRight, right, downRight, down};
}

void Movable::setAttackRows(int up, int upRight, int right, int downRight, int down) {
    m_attackRowMap = {up, upRight, right, downRight, down};
}

void Movable::setScale(float scaleX, float scaleY) {
    m_sprite.setScale(scaleX, scaleY);
}

void Movable::updateAnimation(float dt, sf::Vector2f dir, AnimState state) {
    // 1. 确定当前应该使用哪组配置 (行走 vs 攻击)
    int totalFrames = (state == AnimState::WALK) ? m_animInfo.walkFrames : m_animInfo.attackFrames;
    float duration = (state == AnimState::WALK) ? m_animInfo.walkDuration : m_animInfo.attackDuration;
    const auto& currentMap = (state == AnimState::WALK) ? m_walkRowMap : m_attackRowMap;

    // 2. 计算朝向 (Row Selection)
    // 只有当有移动方向或者处于攻击状态时，才重新计算朝向
    // (如果静止且不攻击，保持上一次的朝向)
    bool hasDirection = (dir.x != 0 || dir.y != 0);
    
    if (hasDirection) {
        float angle = std::atan2(dir.y, dir.x) * 180.f / PI;
        if (angle < 0) angle += 360.f;

        // 映射逻辑 (5方向镜像)
        // 0:Up, 1:UpRight, 2:Right, 3:DownRight, 4:Down
        int directionIndex = 0; 
        m_isFlipped = false;

        // 角度判定区间 (中心线 +- 22.5度)
        if (angle >= 247.5f && angle < 292.5f) {
            directionIndex = 0; // Up
        } else if (angle >= 292.5f && angle < 337.5f) {
            directionIndex = 1; // Up-Right
        } else if (angle >= 337.5f || angle < 22.5f) {
            directionIndex = 2; // Right
        } else if (angle >= 22.5f && angle < 67.5f) {
            directionIndex = 3; // Down-Right
        } else if (angle >= 67.5f && angle < 112.5f) {
            directionIndex = 4; // Down
        } else if (angle >= 112.5f && angle < 157.5f) {
            directionIndex = 3; // Down-Right (Flip)
            m_isFlipped = true;
        } else if (angle >= 157.5f && angle < 202.5f) {
            directionIndex = 2; // Right (Flip)
            m_isFlipped = true;
        } else if (angle >= 202.5f && angle < 247.5f) {
            directionIndex = 1; // Up-Right (Flip)
            m_isFlipped = true;
        }

        // 从映射表中取出真实的 SpriteSheet 行号
        m_currentRow = currentMap[directionIndex];
    } 
    // 如果没有方向 (静止)，我们通常保持 m_currentRow 不变，
    // 但是如果状态切换了 (比如从 Walk 变成 Attack)，我们需要强制更新行号
    else {
        // 获取当前角度对应的逻辑方向索引... 比较复杂，这里简化：
        // 假设 Unit 类会一直传入有效的 dir (比如 m_lastDir)，
        // 或者我们仅仅根据当前的 m_currentRow 来猜测对应的 Attack Row。
        // 为了简单，我们依赖 update 调用者总是传入一个非零的 facing direction，
        // 或者我们在 Unit 类里保存 m_facingDir。
    }

    // 3. 动画帧更新
    m_animationTimer += dt;
    if (m_animationTimer >= duration) {
        m_animationTimer = 0.f;
        m_currentFrame++;
        if (m_currentFrame >= totalFrames) {
            // 循环播放
            m_currentFrame = 0;
        }
    }

    // 4. 应用裁剪
    int left = m_currentFrame * m_animInfo.frameWidth;
    int top = m_currentRow * m_animInfo.frameHeight;

    m_sprite.setTextureRect(sf::IntRect(left, top, m_animInfo.frameWidth, m_animInfo.frameHeight));

    // 5. 应用翻转
    // 获取当前的缩放绝对值 (用户可能设置了 setScale(2.0, 2.0))
    float scaleX = std::abs(m_sprite.getScale().x);
    float scaleY = std::abs(m_sprite.getScale().y);

    if (m_isFlipped) {
        m_sprite.setScale(-scaleX, scaleY);
    } else {
        m_sprite.setScale(scaleX, scaleY);
    }
}

void Movable::render(sf::RenderWindow& window) {
    window.draw(m_sprite);
}

void Movable::setPosition(float x, float y) { m_sprite.setPosition(x, y); }
void Movable::setPosition(const sf::Vector2f& pos) { m_sprite.setPosition(pos); }
sf::Vector2f Movable::getPosition() const { return m_sprite.getPosition(); }