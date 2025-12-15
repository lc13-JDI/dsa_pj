#include "Unit.h"
#include "Tower.h"
#include "Pathfinder.h" 
#include "ResourceManager.h"
#include <cmath> 
#include <iostream>

// ======================= 基类 Unit =======================
Unit::Unit(float x, float y, Team team) 
    : m_team(team), m_attackTimer(0.f), m_facingDir(0.f, 1.f), // 默认朝下
        m_hasCrown(false), m_barMaxWidth(40.f), m_lockedEnemy(nullptr),
        m_repathTimer(0.f) // 初始化计时器
{
    // 默认属性 (作为一个兜底，子类会覆盖它)
    m_hp = 100.f;
    m_maxHp = 100.f;
    m_atk = 10.f;
    m_speed = 60.f; // 每秒移动 60 像素
    m_range = 60.f; 
    m_aggroRange = 150.f; // 默认警戒范围
    m_attackInterval = 1.0f; // 默认1秒打一次

    // 初始位置
    setPosition(x, y);

    // 默认 UI 初始化 (防止忘记调用)
    initUI(false); 
}

void Unit::setStrategicTarget(float x, float y) {
    m_strategicTarget = sf::Vector2f(x, y);
    // 初始设置时，清空路径，以便下次 update 自动计算
    m_pathQueue.clear();
}

void Unit::pathfindToStrategic(const std::vector<std::vector<int>>& mapData) {
    sf::Vector2f startPos = getPosition();
    int startCol = static_cast<int>(startPos.x) / Game::TILE_SIZE;
    int startRow = static_cast<int>(startPos.y) / Game::TILE_SIZE;
    
    int endCol = static_cast<int>(m_strategicTarget.x) / Game::TILE_SIZE;
    int endRow = static_cast<int>(m_strategicTarget.y) / Game::TILE_SIZE;

    std::vector<sf::Vector2i> gridPath = Pathfinder::findPath(mapData, {startCol, startRow}, {endCol, endRow});
    m_pathQueue.clear();
    for (const auto& node : gridPath) {
        m_pathQueue.push_back(sf::Vector2f(node.x * Game::TILE_SIZE + Game::TILE_SIZE / 2.0f, node.y * Game::TILE_SIZE + Game::TILE_SIZE / 2.0f));
    }
}

void Unit::initUI(bool hasCrown, float barWidth, float barHeight, float yOffset) {
    m_hasCrown = hasCrown;
    m_barMaxWidth = barWidth;
    m_uiOffset = sf::Vector2f(0, yOffset);

    // 1. 设置血条背景 (深灰色，带一点边框效果)
    m_hpBarBg.setSize(sf::Vector2f(barWidth, barHeight));
    m_hpBarBg.setFillColor(sf::Color(50, 50, 50));
    m_hpBarBg.setOutlineThickness(1.f);
    m_hpBarBg.setOutlineColor(sf::Color::Black);
    // 中心点设为底部中心，方便定位
    m_hpBarBg.setOrigin(barWidth / 2.f, barHeight / 2.f);

    // 2. 设置血条前景
    m_hpBarFg.setSize(sf::Vector2f(barWidth, barHeight));
    // 根据阵营设置颜色: 红色方(A)用红色，蓝色方(B)用蓝色
    if (m_team == TEAM_A) m_hpBarFg.setFillColor(sf::Color(255, 60, 60)); 
    else                  m_hpBarFg.setFillColor(sf::Color(60, 100, 255));
    m_hpBarFg.setOrigin(barWidth / 2.f, barHeight / 2.f);

    // 3. 设置皇冠图标
    if (m_hasCrown) {
            m_crownSprite.setTexture(ResourceManager::getInstance().getTexture("ui_crown"));
            // 假设皇冠放在血条左侧，稍微偏出一点
            sf::FloatRect bounds = m_crownSprite.getLocalBounds();
            m_crownSprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f); // 底部中心
            m_crownSprite.setScale(0.2f, 0.2f);
    }
}

void Unit::updateUI() {
    // 1. 同步位置
    sf::Vector2f basePos = getPosition() + m_uiOffset;
    m_hpBarBg.setPosition(basePos);
    
    // 前景条的位置需要稍微调整，因为它宽度变化时 origin 还是中心，会导致两边缩
    // SFML 的 setSize 是从左上角开始算的，但我们设置了 Origin
    // 简单做法：前景条取消 Origin 的 X 轴居中，改为左对齐
    m_hpBarFg.setPosition(basePos.x - m_barMaxWidth / 2.f, basePos.y - m_hpBarBg.getSize().y / 2.f);
    
    // 2. 更新血量百分比
    float pct = m_hp / m_maxHp;
    if (pct < 0) pct = 0;
    m_hpBarFg.setSize(sf::Vector2f(m_barMaxWidth * pct, m_hpBarBg.getSize().y));
    // 前景条 Origin 归零 (左上角)，方便宽度变化
    m_hpBarFg.setOrigin(0, 0);

    // 3. 皇冠位置 (在血条左上角或者正上方)
    if (m_hasCrown) {
        // 放在血条左上角稍微偏出的位置，模仿皇室战争
        m_crownSprite.setPosition(basePos.x - m_barMaxWidth/2.f - 10.f, basePos.y);
    }
}

void Unit::render(sf::RenderWindow& window) {
    // 1. 绘制单位本体
    Movable::render(window);

    // 2. 绘制 UI (在单位上方)
    window.draw(m_hpBarBg);
    window.draw(m_hpBarFg);
    if (m_hasCrown) {
        window.draw(m_crownSprite);
    }
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

// 空间划分寻敌算法
// 复杂度：O(K)，K 为周围格子内的单位数，远小于 O(N)
Unit* Unit::findClosestEnemy(const std::vector<std::vector<Unit*>>& spatialGrid) {
    Unit* closest = nullptr;
    float minDist = 99999.f;

    // 1. 计算当前所在的网格坐标
    sf::Vector2f myPos = getPosition();
    int centerCol = static_cast<int>(myPos.x) / Game::TILE_SIZE;
    int centerRow = static_cast<int>(myPos.y) / Game::TILE_SIZE;

    // 2. 计算需要搜索的格子半径
    // 索敌范围 / 格子大小，向上取整
    int searchRadius = static_cast<int>(std::ceil(m_aggroRange / Game::TILE_SIZE));

    // 3. 遍历周围的格子 (Square Loop)
    for (int r = centerRow - searchRadius; r <= centerRow + searchRadius; ++r) {
        for (int c = centerCol - searchRadius; c <= centerCol + searchRadius; ++c) {
            
            // 越界检查
            if (r >= 0 && r < Game::ROWS && c >= 0 && c < Game::COLS) {
                int index = r * Game::COLS + c;
                
                // 遍历该格子内的所有单位
                const auto& cellUnits = spatialGrid[index];
                for (auto other : cellUnits) {
                    if (!other) continue;
                    if (other == this) continue;
                    if (other->isDead()) continue;
                    if (other->getTeam() == this->getTeam()) continue; 

                    sf::Vector2f diff = other->getPosition() - myPos;
                    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                    if (dist > m_aggroRange) continue;

                    if (dist < minDist) {
                        minDist = dist;
                        closest = other;
                    }
                }
            }
        }
    }
    return closest;
}

// 默认攻击逻辑：单体伤害
void Unit::performAttack(Unit* target, const std::vector<std::vector<Unit*>>& spatialGrid) {
    if (target) {
        target->takeDamage(m_atk);
        m_hitSound.play();
    }
}

// 【核心 AI 逻辑】
void Unit::update(float dt,const std::vector<std::vector<Unit*>>& spatialGrid, std::vector<Projectile*>& projectiles, const std::vector<std::vector<int>>& mapData) {
    if (getSprite().getColor() != sf::Color::White) {
        // 简单的颜色恢复渐变效果
        sf::Color c = getSprite().getColor();
        if (m_team == TEAM_A) { // 红色方基础色
             // 红色方不需要变白，这里简单恢复到白色或者保持原色
             if(c.g < 255) c.g += 5;
             if(c.b < 255) c.b += 5;
        } else {
             if(c.g < 255) c.g += 5;
             if(c.b < 255) c.b += 5;
        }
        // 注意：Unit 基类默认是 White，Tower 会覆盖这个逻辑
        // 这里为了简单，只做简单的受击变红 -> 恢复白色
        // 真正颜色逻辑在 Tower 里重写了，普通兵全是白色底图
        getSprite().setColor(c);
    }

    // 0. 更新攻击计时器
    if (m_attackTimer > 0) m_attackTimer -= dt;

    // ================= AI 决策树 =================

    // 1. 验证当前锁定的敌人是否依然有效 (存活且在警戒范围内)
    if (m_lockedEnemy) {
        if (m_lockedEnemy->isDead()) {
            m_lockedEnemy = nullptr;
        } else {
            sf::Vector2f diff = m_lockedEnemy->getPosition() - getPosition();
            float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
            if (dist > m_aggroRange * 1.5f) { // 追太远就放弃 (防抖动，给个1.5倍缓冲)
                m_lockedEnemy = nullptr;
            }
        }
    }

    // 2. 如果没有锁定敌人，尝试索敌 (Giant 会忽略这一步因为 findClosestEnemy 返回 nullptr)
    if (!m_lockedEnemy) {
        Unit* potential = findClosestEnemy(spatialGrid);
        if (potential) {
            sf::Vector2f diff = potential->getPosition() - getPosition();
            float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
            if (dist <= m_aggroRange) {
                m_lockedEnemy = potential;
                // 一旦发现敌人，清空推塔路径，准备战斗/追击
                m_pathQueue.clear(); 
            }
        }
    }

    AnimState currentState = AnimState::WALK;

    // 3. 战斗逻辑 (针对 Locked Enemy)
    if (m_lockedEnemy) {
        sf::Vector2f enemyPos = m_lockedEnemy->getPosition();
        sf::Vector2f diff = enemyPos - getPosition();
        float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
        m_facingDir = diff; // 面向敌人

        if (dist <= m_range) {
            // 射程内 -> 攻击
            currentState = AnimState::ATTACK;
            if (m_attackTimer <= 0) {
                performAttack(m_lockedEnemy, spatialGrid);
                m_attackTimer = m_attackInterval; 
            }
        } else {
            // --- 射程外，追击 
            m_repathTimer -= dt;
            
            // 如果路径走完了(但还没追上)，或者过了0.5秒(敌人位置变了)，就重新寻路
            if (m_pathQueue.empty() || m_repathTimer <= 0.f) {
                setTarget(enemyPos.x, enemyPos.y, mapData);
                m_repathTimer = 0.5f; // 重置计时器
            }
            
            // 沿路径移动
            followPath(dt);

            // 更新朝向
            if (!m_pathQueue.empty()) {
                sf::Vector2f next = m_pathQueue.front();
                sf::Vector2f d = next - getPosition();
                float len = std::sqrt(d.x*d.x + d.y*d.y);
                if (len > 0.1f) m_facingDir = d;
            }
        }
    } 
    // 4. 推塔逻辑 (无敌人干扰)
    else {
        // (A) 检查当前战略目标（塔）是否还活着
        bool isStrategicAlive = false;
        // 简单判断：目标位置附近是否有活着的 Tower
        int tCol = static_cast<int>(m_strategicTarget.x) / Game::TILE_SIZE;
        int tRow = static_cast<int>(m_strategicTarget.y) / Game::TILE_SIZE;
        
        // 检查目标格子里的单位
        if (tCol >= 0 && tCol < Game::COLS && tRow >= 0 && tRow < Game::ROWS) {
            int idx = tRow * Game::COLS + tCol;
            for (auto u : spatialGrid[idx]) {
                if (dynamic_cast<Tower*>(u) && u->getTeam() != m_team && !u->isDead()) {
                    isStrategicAlive = true;
                    break;
                }
            }
        }

        // (B) 如果目标塔挂了，切换到敌方国王塔
        if (!isStrategicAlive) {
            float kingX = 10 * Game::TILE_SIZE + 20;
            float kingY = (m_team == TEAM_A) ? (16 * Game::TILE_SIZE + 20) : (2 * Game::TILE_SIZE + 20);
            
            // 如果已经是国王塔了就不切了，避免死循环
            // 简单距离判断
            float distToKing = std::abs(m_strategicTarget.y - kingY);
            if (distToKing > 50.0f) { // 说明当前不是国王塔
                m_strategicTarget = sf::Vector2f(kingX, kingY);
                m_pathQueue.clear(); // 目标变了，重算路径
            }
        }

        // (C) 移动向战略目标
        // 如果没有路径，计算路径
        if (m_pathQueue.empty()) {
            pathfindToStrategic(mapData);
        }
        
        // 沿路径移动 (最后一段距离如果是攻击范围，可以提前停，但为了简单我们让它走到面前)
        // 优化：如果和目标距离小于 range，其实也可以停下来攻击塔了。
        // 为了复用攻击逻辑，我们这里只管走路。当走进塔的范围，findClosestEnemy 可能会返回塔，
        // 从而在下一帧进入 "战斗逻辑"。
        
        // 这里有一个特殊的点：如果单位是 Giant，findClosestEnemy 会返回塔。
        // 如果单位是 Knight，findClosestEnemy 也会返回塔（因为塔也是 Enemy）。
        // 所以，为了避免还在走路却已经进入射程不攻击的问题，我们可以让 findClosestEnemy 
        // 始终接管“攻击判定”。
        // 但为了保持 "有兵打兵，没兵推塔" 的优先级，我们刚才的逻辑是：
        // "findClosestEnemy" 应该优先返回兵。
        // 让我们修正一下 findClosestEnemy 的逻辑。
        
        followPath(dt);
        
        // 如果正在移动，更新朝向
        if (!m_pathQueue.empty()) {
            sf::Vector2f next = m_pathQueue.front();
            sf::Vector2f diff = next - getPosition();
            float len = std::sqrt(diff.x*diff.x + diff.y*diff.y);
            if (len > 0.1f) m_facingDir = diff;
        }
    }

    updateAnimation(dt, m_facingDir, currentState);
    updateUI();
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
    
    // 配置动画参数
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

    // UI: 无皇冠，宽条，位置较高
    initUI(false, 50.f, 6.f, -45.f);
}

// 巨人只攻击建筑。目前游戏中没有建筑Unit，所以他会忽略所有兵，
// 这样他就会一直执行 moveToTarget 走向敌方基地。
// Giant 只看塔
// 【修改】巨人只打建筑，使用空间网格加速
Unit* Giant::findClosestEnemy(const std::vector<std::vector<Unit*>>& spatialGrid) {
    Unit* closest = nullptr;
    float minDist = 99999.f;

    sf::Vector2f myPos = getPosition();
    int centerCol = static_cast<int>(myPos.x) / Game::TILE_SIZE;
    int centerRow = static_cast<int>(myPos.y) / Game::TILE_SIZE;
    // 巨人索敌范围可以稍微大一点，或者全图
    // 为了利用 spatialGrid，我们还是限定一个比较大的范围，比如 10 格
    int searchRadius = 10; 

    for (int r = centerRow - searchRadius; r <= centerRow + searchRadius; ++r) {
        for (int c = centerCol - searchRadius; c <= centerCol + searchRadius; ++c) {
            if (r >= 0 && r < Game::ROWS && c >= 0 && c < Game::COLS) {
                int index = r * Game::COLS + c;
                for (auto other : spatialGrid[index]) {
                    if (!other || other == this || other->isDead() || other->getTeam() == this->getTeam()) continue;
                    
                    if (!dynamic_cast<Tower*>(other)) continue;

                    sf::Vector2f diff = other->getPosition() - this->getPosition();
                    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                    
                    if (dist < minDist) { minDist = dist; closest = other; }
                }
            }
        }
    }
    return closest;
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

    initUI(false, 50.f, 6.f, -50.f);
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

    initUI(false, 40.f, 5.f, -40.f);
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

    initUI(false, 40.f, 5.f, -35.f);
}

// 瓦基丽的特色：AOE 攻击
// 【修改】瓦基丽的旋风斩 (AOE) 使用空间网格加速
void Valkyrie::performAttack(Unit* target, const std::vector<std::vector<Unit*>>& spatialGrid) {
    float aoeRadius = 60.0f; // AOE 半径 (稍微加大一点)
    
    // 计算周围涉及的格子
    sf::Vector2f myPos = getPosition();
    int centerCol = static_cast<int>(myPos.x) / Game::TILE_SIZE;
    int centerRow = static_cast<int>(myPos.y) / Game::TILE_SIZE;
    int searchRadius = static_cast<int>(std::ceil(aoeRadius / Game::TILE_SIZE));

    for (int r = centerRow - searchRadius; r <= centerRow + searchRadius; ++r) {
        for (int c = centerCol - searchRadius; c <= centerCol + searchRadius; ++c) {
            if (r >= 0 && r < Game::ROWS && c >= 0 && c < Game::COLS) {
                int index = r * Game::COLS + c;
                for (auto other : spatialGrid[index]) {
                    if (!other || other->isDead() || other->getTeam() == m_team) continue;

                    sf::Vector2f diff = other->getPosition() - myPos;
                    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                    if (dist <= aoeRadius) {
                        other->takeDamage(m_atk);
                    }
                }
            }
        }
    }
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

    initUI(false, 30.f, 4.f, -30.f);
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

    initUI(false, 30.f, 4.f, -30.f);
}