#include "Unit.h"
#include "Pathfinder.h" 
#include "ResourceManager.h"
#include <cmath> // 用于 sqrt, atan2 等
#include <iostream>

// ======================= 基类 Unit =======================
Unit::Unit(float x, float y, Team team) 
    : m_team(team), m_attackTimer(0.f), m_facingDir(0.f, 1.f) // 默认朝下
{
    // 默认属性 (作为一个兜底，子类会覆盖它)
    m_hp = 100.f;
    m_maxHp = 100.f;
    m_atk = 10.f;
    m_speed = 60.f; // 每秒移动 60 像素
    m_range = 60.f; 
    m_attackInterval = 1.0f; // 默认1秒打一次

    // [Movable 初始化]
    // 初始位置
    setPosition(x, y);
}

// 初始化音效并播放部署声音
void Unit::initSounds(const std::string& deployKey, const std::string& hitKey) {
    try {
        // 从 ResourceManager 获取 SoundBuffer
        m_deploySound.setBuffer(ResourceManager::getInstance().getSoundBuffer(deployKey));
        m_hitSound.setBuffer(ResourceManager::getInstance().getSoundBuffer(hitKey));
        
        // 立即播放部署音效
        m_deploySound.play();
    } catch (const std::exception& e) {
        std::cerr << "[Unit] Error loading sounds: " << e.what() << std::endl;
    }
}

// 扣血逻辑
void Unit::takeDamage(float damage) {
    m_hp -= damage;
    // 简单的受击反馈：变红一下
    getSprite().setColor(sf::Color::Red);
}

// 默认寻敌逻辑：找最近的活着的敌人
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

// 默认攻击逻辑：单体伤害
void Unit::performAttack(Unit* target, const std::vector<Unit*>& allUnits) {
    if (target) {
        target->takeDamage(m_atk);
        // 播放攻击音效
        // 为了防止音效过于频繁重叠（比如攻速极快时），可以检查是否正在播放
        // 但对于打击感来说，通常直接播放或者用 stop() 再 play() 会更干脆
        // m_hitSound.stop(); // 可选：打断上一次
        m_hitSound.play();
    }
}

// 【核心 AI 逻辑】
void Unit::update(float dt, const std::vector<Unit*>& allUnits, std::vector<Projectile*>& projectiles) {
    if (getSprite().getColor() != sf::Color::White) {
        // 简单的颜色恢复渐变效果
        sf::Color c = getSprite().getColor();
        if(c.g < 255) c.g += 5;
        if(c.b < 255) c.b += 5;
        getSprite().setColor(c);
    }

    // 0. 更新攻击计时器
    if (m_attackTimer > 0) m_attackTimer -= dt;

    // 1. 尝试寻找敌人(虚函数，Giant会重写)
    Unit* target = findClosestEnemy(allUnits);
    float distToEnemy = 99999.f;

    if (target) {
        sf::Vector2f diff = target->getPosition() - this->getPosition();
        distToEnemy = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    }

    AnimState currentState = AnimState::WALK; // 默认为行走状态 (即使原地不动也播放行走或者Idle)

    // 2. 决策：打还是走？
    if (target && distToEnemy <= m_range) {
        // --- 状态：攻击 ---
        // --- 攻击状态 ---
        currentState = AnimState::ATTACK;
        // 更新朝向指向敌人
        m_facingDir = target->getPosition() - getPosition();
        // 既然在射程内，就不移动了，直接开火
        if (m_attackTimer <= 0) {
            //执行攻击(虚函数，Valkyrie会重写)
            performAttack(target, allUnits);
            m_attackTimer = m_attackInterval; // 重置冷却
        }
    } else {
        // --- 状态：移动 ---
        // 没敌人或者敌人在射程外，继续赶路
        if (!m_pathQueue.empty()) {
            // 计算移动方向
            sf::Vector2f targetPos = m_pathQueue.front();
            sf::Vector2f diff = targetPos - getPosition();
            float len = std::sqrt(diff.x*diff.x + diff.y*diff.y);
            if(len > 0.1f) {
                m_facingDir = diff; // 更新朝向
            }           
            // 实际移动
            followPath(dt);
        }
        // 如果没有路径，就保持 WALK 状态但是原地不动（Idle）
    }
    // [关键] 更新动画和朝向
    // 更新动画：传入当前状态和朝向
    updateAnimation(dt, m_facingDir, currentState);
}

// 计算路径
void Unit::setTarget(float tx, float ty, const std::vector<std::vector<int>>& mapData) {
    sf::Vector2f startPos = getPosition(); // 使用 Movable 的 getPosition
    
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
    if (m_pathQueue.empty()) return;
    // 获取当前要去的下一个小目标点
    sf::Vector2f target = m_pathQueue.front();
    sf::Vector2f dir = target - getPosition();
    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (dist < 5.0f) { m_pathQueue.pop_front(); return; }
    sf::Vector2f normDir = dir / dist;
    m_sprite.move(normDir * m_speed * dt);
}


// void Unit::moveTowardsTarget(float dt) {
//     sf::Vector2f currentPos = m_shape.getPosition();
//     sf::Vector2f direction = m_targetPos - currentPos;

//     // 计算距离
//     float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

//     // 如果非常接近目标，就停止 (避免抖动)
//     if (distance < 5.0f) {
//         m_hasTarget = false;
//         return;
//     }

//     // 归一化方向向量 (变成长度为1的向量)
//     sf::Vector2f normalizedDir = direction / distance;

//     // 移动公式：新位置 = 旧位置 + 方向 * 速度 * 时间增量
//     sf::Vector2f newPos = currentPos + normalizedDir * m_speed * dt;
//     m_shape.setPosition(newPos);
// }

// --- 子类构造函数调整 ---
// 这里我们需要硬编码一些 Spritesheet 的参数，通常需要查看图片来确定
// 假设：皇室战争素材大概是 5行 或 8行，每行 5-10 帧。
// 让我们暂时假设：每帧 72x72 像素，一行 11 帧 (这在 CR 素材中很常见)


// ======================= 中间层 =======================

// ======================= 中间层 =======================

Tank::Tank(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 300.f; m_maxHp = 300.f; m_atk = 20.f; m_speed = 30.f; 
}
Melee::Melee(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 150.f; m_maxHp = 150.f; m_atk = 15.f; m_speed = 60.f;
}
Ranged::Ranged(float x, float y, Team team) : Unit(x, y, team) {
    m_hp = 60.f; m_maxHp = 60.f; m_atk = 10.f; m_speed = 70.f; m_range = 150.f;
}

// ======================= 具体兵种实现 =======================

// --- 1. Giant (巨人) ---
Giant::Giant(float x, float y, Team team) : Tank(x, y, team) {
    m_hp = 600.f; m_maxHp = 600.f; m_atk = 30.f; m_speed = 25.f; // 极肉，慢
    
    // 配置动画参数 (请根据实际图片大小修改)
    // 假设 Giant Spritesheet 很大
    AnimInfo info;
    info.frameWidth = 201.5; info.frameHeight = 206; 
    info.walkFrames = 8;  info.attackFrames = 8;
    info.walkDuration = 0.15f; info.attackDuration = 0.15f;

    initSprite(ResourceManager::getInstance().getTexture("unit_giant"), info);
    setScale(0.3f, 0.3f); // 巨人要大

    // 映射: 上5行攻击(0-4), 下5行行走(5-9)
    // 请根据游戏内的实际朝向调整这些数字的顺序
    // 顺序: Up, UpRight, Right, DownRight, Down
    setAttackRows(1, 4, 3, 0, 2);
    setWalkRows(9, 7, 6, 8, 5);

    initSounds("sfx_deploy_giant", "sfx_hit_giant");
}

// 巨人只攻击建筑。目前游戏中没有建筑Unit，所以他会忽略所有兵，
// 这样他就会一直执行 moveToTarget 走向敌方基地。
Unit* Giant::findClosestEnemy(const std::vector<Unit*>& allUnits) {
    return nullptr; 
}

// --- 2. PEKKA (皮卡) ---
Pekka::Pekka(float x, float y, Team team) : Tank(x, y, team) {
    m_hp = 500.f; m_maxHp = 500.f; m_atk = 80.f; m_speed = 35.f; // 攻极高
    m_attackInterval = 1.8f; // 攻速很慢

    AnimInfo info;
    info.frameWidth = 231; info.frameHeight = 231; 
    info.walkFrames = 10; info.attackFrames = 6;
    info.walkDuration = 0.12f; info.attackDuration = 0.2f;

    initSprite(ResourceManager::getInstance().getTexture("unit_pekka"), info);
    setScale(0.3f, 0.3f);

    setAttackRows(4, 2, 1, 3, 0);
    setWalkRows(6, 9, 8, 5, 7);

    initSounds("sfx_deploy_pekka", "sfx_hit_pekka");
}

// --- 3. Knight (骑士) ---
Knight::Knight(float x, float y, Team team) : Melee(x, y, team) {
    m_hp = 200.f; m_maxHp = 200.f; m_atk = 20.f; m_speed = 50.f;

    AnimInfo info;
    info.frameWidth = 187; info.frameHeight = 181; 
    info.walkFrames = 12; info.attackFrames = 12;
    info.walkDuration = 0.1f; info.attackDuration = 0.1f;

    initSprite(ResourceManager::getInstance().getTexture("unit_knight"), info);
    setScale(0.3f, 0.3f);

    setAttackRows(4, 2, 1, 3, 0);
    setWalkRows(6, 9, 8, 5, 7);

    initSounds("sfx_deploy_knight", "sfx_hit_knight");
}

// --- 4. Valkyrie (瓦基丽) ---
Valkyrie::Valkyrie(float x, float y, Team team) : Melee(x, y, team) {
    m_hp = 250.f; m_maxHp = 250.f; m_atk = 18.f; m_speed = 55.f;
    m_attackInterval = 1.2f;

    AnimInfo info;
    info.frameWidth = 173; info.frameHeight = 153; // 旋风斩图可能比较宽
    info.walkFrames = 8; info.attackFrames = 12;
    info.walkDuration = 0.15f; info.attackDuration = 0.1f;

    initSprite(ResourceManager::getInstance().getTexture("unit_valkyrie"), info);
    setScale(0.3f, 0.3f);

    setAttackRows(4, 2, 1, 3, 0);
    setWalkRows(8, 6, 9, 5, 7);

    initSounds("sfx_deploy_valkyrie", "sfx_hit_valkyrie");
}

// 瓦基丽的特色：AOE 攻击
void Valkyrie::performAttack(Unit* target, const std::vector<Unit*>& allUnits) {
    // 这里的 target 是主目标，但我们会遍历周围所有敌人
    float aoeRadius = 50.0f; // AOE 半径

    for (auto other : allUnits) {
        if (!other || other->isDead() || other->getTeam() == m_team) continue;

        sf::Vector2f diff = other->getPosition() - getPosition();
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        if (dist <= aoeRadius) {
            other->takeDamage(m_atk);
        }
    }
    // 可以添加一个旋转特效，这里暂时省略
}

// --- 5. Archers (弓箭手) ---
Archers::Archers(float x, float y, Team team) : Ranged(x, y, team) {
    m_hp = 80.f; m_maxHp = 80.f; m_atk = 12.f; m_speed = 65.f; 
    m_range = 150.f;

    AnimInfo info;
    info.frameWidth = 130; info.frameHeight = 135; 
    info.walkFrames = 8; info.attackFrames = 5;
    info.walkDuration = 0.15f; info.attackDuration = 0.24f;

    initSprite(ResourceManager::getInstance().getTexture("unit_archers"), info);
    setScale(0.32f, 0.32f);

    setAttackRows(4, 2, 1, 3, 0);
    setWalkRows(9, 6, 8, 5, 7);

    initSounds("sfx_deploy_archers", "sfx_hit_archers");
}

// --- 6. Dart Goblin (吹箭哥布林) ---
DartGoblin::DartGoblin(float x, float y, Team team) : Ranged(x, y, team) {
    m_hp = 50.f; m_maxHp = 50.f; m_atk = 15.f; m_speed = 90.f; // 极快
    m_range = 200.f; // 射程极远
    m_attackInterval = 0.5f; // 攻速极快

    AnimInfo info;
    info.frameWidth = 129; info.frameHeight = 141; 
    info.walkFrames = 8; info.attackFrames = 5;
    info.walkDuration = 0.15f; info.attackDuration = 0.24f;

    initSprite(ResourceManager::getInstance().getTexture("unit_dartgoblin"), info);
    setScale(0.31f, 0.21f);

    setAttackRows(4, 2, 1, 3, 0);
    setWalkRows(6, 9, 8, 5, 7);

    initSounds("sfx_deploy_dartgoblin", "sfx_hit_dartgoblin");
}