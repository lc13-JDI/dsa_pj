#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <array>

// 动画状态枚举
enum class AnimState {
    WALK,
    ATTACK
};

// 动画配置结构体
struct AnimInfo {
    int frameWidth;       // 单帧宽
    int frameHeight;      // 单帧高
    
    int walkFrames;       // 行走状态的总帧数
    int attackFrames;     // 攻击状态的总帧数
    
    float walkDuration;   // 行走单帧时间
    float attackDuration; // 攻击单帧时间
};

class Movable {
public:
    Movable();
    virtual ~Movable();

    // 初始化精灵和基础参数
    void initSprite(const sf::Texture& texture, const AnimInfo& info);

    // 设置行与方向的映射关系
    // 参数对应 Spritesheet 中的行号 (0-9)
    // 这里的 Up, UpRight 等指代的是逻辑方向，填入对应的行号即可
    void setWalkRows(int up, int upRight, int right, int downRight, int down);
    void setAttackRows(int up, int upRight, int right, int downRight, int down);

    // 设置缩放
    void setScale(float scaleX, float scaleY);

    // 核心更新：需要传入当前状态和朝向向量
    void updateAnimation(float dt, sf::Vector2f dir, AnimState state);

    virtual void render(sf::RenderWindow& window);
    
    void setPosition(float x, float y);
    void setPosition(const sf::Vector2f& pos);
    sf::Vector2f getPosition() const;
    // 获取精灵对象的引用，用于返回类中的精灵对象
    sf::Sprite& getSprite() { return m_sprite; }

protected:
    sf::Sprite m_sprite;
    AnimInfo m_animInfo;
   
    float m_animationTimer;// 动画计时器
    int m_currentFrame;// 当前动画帧索引
      
    int m_currentRow;// 当前渲染的行号
    bool m_isFlipped;// 是否水平翻转

    // 映射表: 索引 0=Up, 1=UpRight, 2=Right, 3=DownRight, 4=Down
    std::array<int, 5> m_walkRowMap;
    std::array<int, 5> m_attackRowMap;
};