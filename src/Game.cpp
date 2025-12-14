#include "Game.h"
#include <iostream>
#include "Unit.h"
#include "Tower.h"
#include "Projectile.h"
#include <chrono> // 用于线程休眠
#include <iomanip> // 用于保留小数
#include <sstream>
#include "ResourceManager.h"

// 辅助函数：计算两个单位/点的距离
static float distSq(sf::Vector2f a, sf::Vector2f b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

Game::Game() 
    : m_running(false) , m_selectedCardIndex(-1),
    m_elixir(5.0f), m_maxElixir(10.0f), m_elixirRate(0.7f), // 初始5费，上限10费，每秒回0.7费
    m_enemyElixir(5.0f), m_enemyMaxElixir(10.0f),m_aiThinkTimer(0.f)
    {
    // 1. 加载资源
    ResourceManager::getInstance().loadAllAssets(); // 加载所有资源

    // 2. 初始化窗口和地图
    initWindow();
    initMap();
    initUI();
    initTowers(); // 先初始化塔
    initUnits(); // 初始化单位

    // 默认设置普通难度
    setDifficulty(Difficulty::NORMAL);
}

Game::~Game() {
    // 1. 停止逻辑线程
    m_running = false;
    if (m_logicThread.joinable()) {
        m_logicThread.join();
    }

    // 2. 清理内存
    for (auto unit : m_units) {
        delete unit;
    }
    m_units.clear();

    // 清理子弹内存
    for (auto proj : m_projectiles) {
        delete proj;
    }
    m_projectiles.clear();
}

// 【新增】设置难度逻辑
void Game::setDifficulty(Difficulty level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_difficulty = level;
    
    switch (level) {
        case Difficulty::EASY:
            m_enemyElixirRate = 0.4f;   // 敌方回费很慢 (玩家 0.7)
            m_aiReactionTime = 2.0f;    // 2秒才思考一次
            m_difficultyText.setString("Difficulty: EASY (Press 1/2/3)");
            m_difficultyText.setFillColor(sf::Color::Green);
            break;
        case Difficulty::NORMAL:
            m_enemyElixirRate = 0.7f;   // 敌方回费同玩家
            m_aiReactionTime = 1.0f;    // 1秒思考一次
            m_difficultyText.setString("Difficulty: NORMAL (Press 1/2/3)");
            m_difficultyText.setFillColor(sf::Color::Yellow);
            break;
        case Difficulty::HARD:
            m_enemyElixirRate = 1.2f;   // 敌方回费极快
            m_aiReactionTime = 0.5f;    // 反应极快
            m_difficultyText.setString("Difficulty: HARD (Press 1/2/3)");
            m_difficultyText.setFillColor(sf::Color::Red);
            break;
    }
    std::cout << "[Game] Difficulty set to " << (int)level << std::endl;
}

void Game::initWindow() {
    // 根据地图大小动态计算窗口分辨率
    int width = COLS * TILE_SIZE;
    int height = ROWS * TILE_SIZE + UI_HEIGHT; // 增加UI高度

    // 创建窗口
    m_window.create(sf::VideoMode(width, height), "Battle Simulation");
    // 设置帧率限制为 60 FPS
    m_window.setFramerateLimit(60);

    // 设置背景图
    sf::Texture& bgTexture = ResourceManager::getInstance().getTexture("background");
    m_bgSprite.setTexture(bgTexture);

    // 计算缩放比例，让背景图完全适应窗口大小
    float scaleX = static_cast<float>(width) / bgTexture.getSize().x;
    float scaleY = static_cast<float>(height) / bgTexture.getSize().y;
    m_bgSprite.setScale(scaleX, scaleY);
}

void Game::initUI() {
    // 1. 初始化游戏结束文字
    // 使用引用，防止拷贝
    sf::Font& font = ResourceManager::getInstance().getFont("main_font");
    m_gameOverText.setFont(font);
    m_gameOverText.setCharacterSize(60);
    m_gameOverText.setFillColor(sf::Color::White);
    m_gameOverText.setOutlineColor(sf::Color::Black);
    m_gameOverText.setOutlineThickness(3.f);
    m_gameOverText.setString(""); // 初始为空

    // 2. 初始化 UI 背景底板
    float mapBottom = ROWS * TILE_SIZE;
    float windowWidth = m_window.getSize().x;
    
    m_uiBg.setSize(sf::Vector2f(windowWidth, UI_HEIGHT));
    m_uiBg.setFillColor(sf::Color(50, 50, 50)); // 深灰色底板
    m_uiBg.setOutlineThickness(-2.0f); // 内描边
    m_uiBg.setOutlineColor(sf::Color(30, 30, 30));
    m_uiBg.setPosition(0, mapBottom);

    // 初始化圣水条组件
    float barWidth = windowWidth * 0.8f;
    float barHeight = 25.0f;
    float barX = (windowWidth - barWidth) / 2.0f;
    float barY = mapBottom + UI_HEIGHT - 40.0f; // 放在底部

    // 背景槽 (深黑)
    m_elixirBarBg.setSize(sf::Vector2f(barWidth, barHeight));
    m_elixirBarBg.setFillColor(sf::Color(20, 20, 20));
    m_elixirBarBg.setOutlineThickness(2.f);
    m_elixirBarBg.setOutlineColor(sf::Color(200, 200, 200));
    m_elixirBarBg.setPosition(barX, barY);

    // 前景条 (紫色)
    m_elixirBarFg.setSize(sf::Vector2f(0, barHeight)); // 初始宽度0，在update中更新
    m_elixirBarFg.setFillColor(sf::Color(255, 0, 255)); // 亮紫色
    m_elixirBarFg.setPosition(barX, barY);

    // 圣水图标 (左侧)
    m_elixirIcon.setTexture(ResourceManager::getInstance().getTexture("ui_elixir"));
    m_elixirIcon.setScale(0.15f, 0.15f); // 原图可能很大，缩小一下
    m_elixirIcon.setPosition(barX - 40, barY - 10);

    // 圣水文字 (右侧或条上)
    m_elixirStatusText.setFont(font);
    m_elixirStatusText.setCharacterSize(18);
    m_elixirStatusText.setFillColor(sf::Color::White);
    m_elixirStatusText.setPosition(barX + barWidth + 10, barY);

    // 难度显示UI
    m_difficultyText.setFont(font);
    m_difficultyText.setCharacterSize(16);
    m_difficultyText.setPosition(10, 10); // 左上角
    m_difficultyText.setOutlineColor(sf::Color::Black);
    m_difficultyText.setOutlineThickness(1.5f);

    // 3. 初始化卡牌列表
    // 定义我们要添加的卡牌数据 (类型, 费用, 图标key)
    struct CardData {
        UnitType type;
        int cost;
        std::string iconKey;
    };

    std::vector<CardData> initialCards = {
        { UnitType::KNIGHT, 3, "icon_knight" },
        { UnitType::ARCHERS, 3, "icon_archers" },
        { UnitType::GIANT, 5, "icon_giant" },
        { UnitType::PEKKA, 7, "icon_pekka" },
        { UnitType::VALKYRIE, 4, "icon_valkyrie" },
        { UnitType::DART_GOBLIN, 3, "icon_dartgoblin" }
    };

    // 计算卡牌布局
    // 假设卡牌槽大小为 90x110 (略大于图标)，间隔分布
    float cardWidth = 70.0f; // 根据 icon 图片比例调整
    float cardHeight = 84.0f; // 保持比例
    float startX = 40.0f;
    float startY = mapBottom + 20.0f; // UI 区域内
    float gap = 20.0f;

    for (size_t i = 0; i < initialCards.size(); i++) {
        Card newCard;
        newCard.type = initialCards[i].type;
        newCard.cost = initialCards[i].cost;

        // 设置卡槽背景 (用于选中高亮)
        newCard.slotShape.setSize(sf::Vector2f(cardWidth + 10, cardHeight + 10));
        newCard.slotShape.setFillColor(sf::Color(100, 100, 100)); // 默认灰色
        newCard.slotShape.setOutlineThickness(2.0f);
        newCard.slotShape.setOutlineColor(sf::Color::Black);
        
        float currentX = startX + i * (cardWidth + gap + 10);
        newCard.slotShape.setPosition(currentX, startY);

        // 设置卡牌图标
        newCard.sprite.setTexture(ResourceManager::getInstance().getTexture(initialCards[i].iconKey));
        // 适配大小
        sf::FloatRect bounds = newCard.sprite.getLocalBounds();
        newCard.sprite.setScale(cardWidth / bounds.width, cardHeight / bounds.height);
        newCard.sprite.setPosition(currentX + 5, startY + 5); // 居中于 slot

        // 【新增】卡牌上的费用数字
        newCard.costText.setFont(font);
        newCard.costText.setString(std::to_string(newCard.cost));
        newCard.costText.setCharacterSize(14);
        newCard.costText.setFillColor(sf::Color::Cyan); // 青色字体比较显眼
        newCard.costText.setOutlineColor(sf::Color::Black);
        newCard.costText.setOutlineThickness(1.5f);
        // 放在卡牌右下角
        sf::FloatRect textRect = newCard.costText.getLocalBounds();
        newCard.costText.setPosition(
            newCard.slotShape.getPosition().x + cardWidth - 5, 
            newCard.slotShape.getPosition().y + cardHeight - 8
        );

        // 记录点击区域
        newCard.touchArea = newCard.slotShape.getGlobalBounds();

        m_deck.push_back(newCard);
    }
}

void Game::initMap() {
    // 1. 全部重置为平地
    m_mapData.assign(ROWS, std::vector<int>(COLS, GROUND));

    // 2. 设置河流 (假设在第 10 行，横贯整个地图)
    int riverRow = 10;
    for (int c = 0; c < COLS; c++) {
        m_mapData[riverRow][c] = RIVER;
    }

    // 3. 设置桥梁 (左桥和右桥)
    m_mapData[riverRow][10] = BRIDGE;// 左桥
    m_mapData[riverRow][14] = BRIDGE;// 右桥

    // 这里只是标记地形，实际生成塔在 initTowers
    // 4. 设置基地 (模仿皇室战争布局)
    // 甲方 (上方, Team A)
    m_mapData[5][12] = BASE_A;  // 国王塔 (中间)
    m_mapData[6][10] = BASE_A;  // 左公主塔
    m_mapData[6][14] = BASE_A; // 右公主塔

    // 乙方 (下方, Team B)
    m_mapData[15][12] = BASE_B; // 国王塔 (中间)
    m_mapData[14][10] = BASE_B; // 左公主塔
    m_mapData[14][14] = BASE_B;// 右公主塔

    // 5. 设置边缘障碍 (可选，防止兵走出画面太远)
    for (int r = 0; r < ROWS; r++) {
        m_mapData[r][8] = MOUNTAIN;          // 左边界
        m_mapData[r][16] = MOUNTAIN;         // 右边界
    }
}

// 根据地图生成塔对象
void Game::initTowers() {
    // 这与 mapData 中的标记对应
    
    // Team A (上方)
    // 国王塔
    m_units.push_back(new Tower(12 * TILE_SIZE + 20, 5 * TILE_SIZE + 20, TEAM_A, TowerType::KING));
    // 公主塔
    m_units.push_back(new Tower(10 * TILE_SIZE + 20, 6 * TILE_SIZE + 20, TEAM_A, TowerType::PRINCESS));
    m_units.push_back(new Tower(14 * TILE_SIZE + 20, 6 * TILE_SIZE + 20, TEAM_A, TowerType::PRINCESS));

    // Team B (下方)
    // 国王塔
    m_units.push_back(new Tower(12 * TILE_SIZE + 20, 15 * TILE_SIZE + 20, TEAM_B, TowerType::KING));
    // 公主塔
    m_units.push_back(new Tower(10 * TILE_SIZE + 20, 14 * TILE_SIZE + 20, TEAM_B, TowerType::PRINCESS));
    m_units.push_back(new Tower(14 * TILE_SIZE + 20, 14 * TILE_SIZE + 20, TEAM_B, TowerType::PRINCESS));
}

void Game::initUnits() {
    // 初始化时设置战略目标 (塔的坐标)
    // 假设我们都知道塔的精确坐标：
    // A左塔 (10, 6), A右塔 (14, 6)
    // B左塔 (10, 14), B右塔 (14, 14)
    
    // Team A P.E.K.K.A 进攻中路 -> 目标 B 左塔
    Unit* pekka = new Pekka(12 * TILE_SIZE, 8 * TILE_SIZE, TEAM_A);
    pekka->setStrategicTarget(10 * TILE_SIZE + 20, 14 * TILE_SIZE + 20); 
    m_units.push_back(pekka);

    // Team A Valkyrie 进攻右路 -> 目标 B 右塔
    Unit* valkyrie = new Valkyrie(14 * TILE_SIZE, 8 * TILE_SIZE, TEAM_A);
    valkyrie->setStrategicTarget(14 * TILE_SIZE + 20, 14 * TILE_SIZE + 20); 
    m_units.push_back(valkyrie);

    // Team B Giant 进攻左路 -> 目标 A 左塔
    Unit* giant = new Giant(10 * TILE_SIZE, 12 * TILE_SIZE, TEAM_B);
    giant->setStrategicTarget(10 * TILE_SIZE + 20, 6 * TILE_SIZE + 20); 
    m_units.push_back(giant);
    
    // Team B Knight 防守中路 (会主动索敌)
    Unit* knight = new Knight(12 * TILE_SIZE, 12 * TILE_SIZE, TEAM_B);
    knight->setStrategicTarget(12 * TILE_SIZE + 20, 5 * TILE_SIZE + 20); // 直接冲国王塔
    m_units.push_back(knight);
}

void Game::run() {
    // 启动逻辑线程
    m_running = true;
    m_logicThread = std::thread(&Game::logicLoop, this);

    // 【主线程循环】只负责事件和渲染
    while (m_window.isOpen()) {
        processEvents();
        render();
    }
    
    // 窗口关闭后，通知逻辑线程退出
    m_running = false;
    if (m_logicThread.joinable()) {
        m_logicThread.join(); // 等待线程安全退出
    }
}

void Game::logicLoop() {
    sf::Clock clock;
    while (m_running) {
        // 计算 Delta Time
        sf::Time elapsed = clock.restart();
        float dt = elapsed.asSeconds();

        // 限制逻辑帧率，避免 CPU 100% (大约 60Hz)
        if (dt < 0.016f) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (!m_gameOver) {
            update(dt);
        }
    }
}

// 【关键修改】处理鼠标点击
void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            m_window.close();
        
        // 监听鼠标按下事件
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                handleMouseClick(event.mouseButton.x, event.mouseButton.y);
            }
        }

        // 键盘事件：调节难度
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Num1) setDifficulty(Difficulty::EASY);
            if (event.key.code == sf::Keyboard::Num2) setDifficulty(Difficulty::NORMAL);
            if (event.key.code == sf::Keyboard::Num3) setDifficulty(Difficulty::HARD);
        }
    }
}

// 【新增】处理点击逻辑
void Game::handleMouseClick(int x, int y) {
    float mapHeight = ROWS * TILE_SIZE;

    // 1. 如果点击的是 UI 区域 (底部)
    if (y > mapHeight) {
        // 遍历手牌，检查是否点中了某张卡
        for (int i = 0; i < m_deck.size(); i++) {
            if (m_deck[i].touchArea.contains(static_cast<float>(x), static_cast<float>(y))) {
                m_selectedCardIndex = i; // 更新选中索引
                
                // 更新视觉反馈：选中的变黄，其他的变黑
                for (int j = 0; j < m_deck.size(); j++) {
                    if (j == i) {
                        m_deck[j].slotShape.setOutlineColor(sf::Color::Yellow);
                        m_deck[j].slotShape.setOutlineThickness(3.0f);
                    } else {
                        m_deck[j].slotShape.setOutlineColor(sf::Color::Black);
                        m_deck[j].slotShape.setOutlineThickness(2.0f);
                    }
                }
                std::cout << "[UI] Selected card index: " << i << std::endl;
                return;
            }
        }
        // 如果点了 UI 空白处，可以取消选中
        // m_selectedCardIndex = -1;
    }
    // 2. 如果点击的是 地图 区域，且当前有选中的卡牌
    else if (m_selectedCardIndex != -1) {
        // 【关键修改】检查圣水是否足够
        int cost = m_deck[m_selectedCardIndex].cost;
        if (m_elixir < cost) {
            std::cout << "[Game] Not enough Elixir! Need " << cost << ", have " << (int)m_elixir << std::endl;
            // 可以在这里播放一个“错误提示音”
            return; // 直接返回，不生成，不扣费
        }
        // 计算网格坐标
        int col = x / TILE_SIZE;
        int row = y / TILE_SIZE;

        // 合法性检查：
        // A. 是否越界
        if (row < 0 || row >= ROWS || col < 0 || col >= COLS) return;
        
        // B. 地形检查：不能放在河里(RIVER)或山上(MOUNTAIN)，除非是飞行单位(暂未实现区分)
        int tileType = m_mapData[row][col];
        if (tileType == RIVER || tileType == MOUNTAIN) {
            std::cout << "[Game] Invalid terrain placement!" << std::endl;
            return;
        }

        // C. 放置位置检查：只能放在己方半场 (下半场, row >= 10) 
        // 简单规则：row > 10 或者是 已经过河的区域
        if (row < 10) {
             std::cout << "[Game] Can only deploy on your side!" << std::endl;
             return;
        }

        // D. 生成单位
        // 将网格中心转换为像素坐标
        float spawnX = col * TILE_SIZE + TILE_SIZE / 2.0f;
        float spawnY = row * TILE_SIZE + TILE_SIZE / 2.0f;
        
        // 【加锁】因为我们在修改 m_units
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            spawnUnit(m_deck[m_selectedCardIndex].type, spawnX, spawnY, TEAM_B); // 玩家是 TEAM_B

            // 扣除圣水
            m_elixir -= cost;
            std::cout << "[Game] Spent " << cost << " Elixir. Remaining: " << m_elixir << std::endl;
        }

        // E. 重置选中状态
        m_selectedCardIndex = -1;
        // 恢复所有卡牌边框颜色
        for (auto& card : m_deck) {
            card.slotShape.setOutlineColor(sf::Color::Black);
            card.slotShape.setOutlineThickness(2.0f);
        }
    }
}

// 【新增】生成单位工厂函数
void Game::spawnUnit(UnitType type, float x, float y, int team) {
    Unit* newUnit = nullptr;
    Team t = static_cast<Team>(team);

    switch (type) {
        case UnitType::KNIGHT:      newUnit = new Knight(x, y, t); break;
        case UnitType::GIANT:       newUnit = new Giant(x, y, t); break;
        case UnitType::ARCHERS:     newUnit = new Archers(x, y, t); break;
        case UnitType::PEKKA:       newUnit = new Pekka(x, y, t); break;
        case UnitType::VALKYRIE:    newUnit = new Valkyrie(x, y, t); break;
        case UnitType::DART_GOBLIN: newUnit = new DartGoblin(x, y, t); break;
    }

    if (newUnit) {
        // AI 和 玩家的目标选择逻辑
        // 简单逻辑：攻击敌方公主塔
        // 玩家(Team B)进攻 上方(Team A) 的塔
        // AI(Team A) 进攻 下方(Team B) 的塔
        float targetY = (team == TEAM_B) ? (6 * TILE_SIZE + 20) : (14 * TILE_SIZE + 20);
        
        // 左右路判断
        float leftX = 10 * TILE_SIZE + 20;
        float rightX = 14 * TILE_SIZE + 20;
        
        // 哪边近去哪边
        if (std::abs(x - leftX) < std::abs(x - rightX)) {
            newUnit->setStrategicTarget(leftX, targetY);
        } else {
            newUnit->setStrategicTarget(rightX, targetY);
        }

        m_units.push_back(newUnit);
    }
}

// 【新增】智能 AI 决策逻辑
void Game::updateAI(float dt) {
    // 1. 恢复圣水
    if (m_enemyElixir < m_enemyMaxElixir) {
        m_enemyElixir += m_enemyElixirRate * dt;
        if (m_enemyElixir > m_enemyMaxElixir) m_enemyElixir = m_enemyMaxElixir;
    }

    // 2. 思考间隔 (模拟反应时间)
    m_aiThinkTimer += dt;
    if (m_aiThinkTimer < m_aiReactionTime) return;
    m_aiThinkTimer = 0.f; // 重置计时器

    // 3. 分析局势
    Unit* nearestThreat = nullptr;
    float minThreatDist = 99999.f;
    int threatCount = 0;

    // 敌方基地参考线 (AI在上方，所以防守线在河道上方一点，比如 Row 8)
    float defenseLineY = 8 * TILE_SIZE;

    for (auto u : m_units) {
        // 寻找活着的、玩家的单位
        if (u && !u->isDead() && u->getTeam() == TEAM_B) {
            // 如果玩家单位越过河道 (Y < 10*40 = 400)
            if (u->getPosition().y < 10 * TILE_SIZE) {
                threatCount++;
                float dist = u->getPosition().y; // 距离上边界的距离
                if (dist < minThreatDist) {
                    minThreatDist = dist;
                    nearestThreat = u;
                }
            }
        }
    }

    // 4. 决策：防守 vs 进攻

    // --- A. 防守策略 (当有单位过河) ---
    if (nearestThreat) {
        // 如果圣水足够，进行防守
        if (m_enemyElixir >= 4.0f) {
            // 简单的克制逻辑
            UnitType spawnType = UnitType::KNIGHT; // 默认用骑士防守
            
            // 如果威胁单位是 Giant (坦克)，用 Pekka (高伤) 防守
            if (dynamic_cast<Giant*>(nearestThreat)) {
                if (m_enemyElixir >= 7) spawnType = UnitType::PEKKA;
                else spawnType = UnitType::KNIGHT;
            } 
            // 如果是 Archer (脆皮)，用 Valkyrie (群伤) 防守
            else if (dynamic_cast<Archers*>(nearestThreat)) {
                if (m_enemyElixir >= 4) spawnType = UnitType::VALKYRIE;
            }

            // 获取卡牌费用 (硬编码简化)
            int cost = 3;
            if (spawnType == UnitType::PEKKA) cost = 7;
            if (spawnType == UnitType::VALKYRIE) cost = 4;

            if (m_enemyElixir >= cost) {
                // 在威胁单位前方一点点放置
                float spawnX = nearestThreat->getPosition().x;
                float spawnY = nearestThreat->getPosition().y - 60.0f; // 放在我方一侧
                
                // 边界检查，别放太上面
                if (spawnY < 2 * TILE_SIZE) spawnY = 2 * TILE_SIZE;

                spawnUnit(spawnType, spawnX, spawnY, TEAM_A);
                m_enemyElixir -= cost;
                std::cout << "[AI] Defending with unit type " << (int)spawnType << std::endl;
                return; // 本次思考结束
            }
        }
    }

    // --- B. 进攻策略 (无威胁且圣水充裕) ---
    // 如果圣水快满了 (>9)，必须进攻，防止圣水溢出浪费
    else if (m_enemyElixir > 9.0f) {
        // 随机选择左路 (10, 9) 或 右路 (14, 9) 的桥头
        float bridgeY = 9 * TILE_SIZE; // 河道上方
        float bridgeX = (rand() % 2 == 0) ? (10 * TILE_SIZE) : (14 * TILE_SIZE);
        
        // 优先放坦克
        UnitType type = (rand() % 2 == 0) ? UnitType::GIANT : UnitType::PEKKA;
        int cost = (type == UnitType::GIANT) ? 5 : 7;

        // 如果钱不够皮卡，就放骑士
        if (m_enemyElixir < cost) {
            type = UnitType::KNIGHT;
            cost = 3;
        }

        spawnUnit(type, bridgeX, bridgeY, TEAM_A);
        m_enemyElixir -= cost;
        std::cout << "[AI] Attacking bridge with unit type " << (int)type << std::endl;
    }
}


void Game::update(float dt) {
    // 【加锁】因为我们要读取和修改 m_units，渲染线程也在读
    std::lock_guard<std::mutex> lock(m_mutex);

    // 圣水恢复逻辑
    if (m_elixir < m_maxElixir) {
        m_elixir += m_elixirRate * dt;
        if (m_elixir > m_maxElixir) m_elixir = m_maxElixir;
    }

    // 调用 AI 逻辑
    if (!m_gameOver) {
        updateAI(dt);
    }

    // 1. 更新所有单位状态 (移动、攻击)
    for (auto unit : m_units) {
        unit->update(dt, m_units, m_projectiles, m_mapData);
    }

    // 2. 更新所有子弹
    for (auto proj : m_projectiles) {
        proj->update(dt);
    }

    // 3. 清理非活跃子弹 (击中目标的)
    auto projIt = m_projectiles.begin();
    while (projIt != m_projectiles.end()) {
        if (!(*projIt)->isActive()) {
            delete *projIt;
            projIt = m_projectiles.erase(projIt);
        } else {
            ++projIt;
        }
    }

    // 4. 清理尸体
    auto it = m_units.begin();
    while (it != m_units.end()) {
        Unit* u = *it;
        if (u->isDead()) {
            // 【核心逻辑】检查是否是塔
            Tower* t = dynamic_cast<Tower*>(u);
            if (t) {
                // 1. 生成废墟 Sprite
                sf::Sprite ruin;
                ruin.setTexture(ResourceManager::getInstance().getTexture("vfx_damaged"));
                
                // 设置废墟位置和原点
                sf::FloatRect bounds = ruin.getLocalBounds();
                //ruin.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                ruin.setOrigin(bounds.width / 2.f, 0.f);
                ruin.setPosition(t->getPosition());
                // 稍微缩放一点以适应格子
                ruin.setScale(0.25f, 0.25f);
                
                m_ruins.push_back(ruin);

                // 2. 检查是否为国王塔 -> 游戏结束
                if (t->isKing()) {
                    m_gameOver = true;
                    if (t->getTeam() == TEAM_A) {
                        m_gameOverText.setString("Blue Wins!");
                        m_gameOverText.setFillColor(sf::Color(100, 100, 255));
                    } else {
                        m_gameOverText.setString("Red Wins!");
                        m_gameOverText.setFillColor(sf::Color(255, 60, 60));
                    }
                    
                    // 居中显示文字
                    sf::FloatRect textRect = m_gameOverText.getLocalBounds();
                    m_gameOverText.setOrigin(textRect.left + textRect.width/2.0f,
                                           textRect.top  + textRect.height/2.0f);
                    m_gameOverText.setPosition(m_window.getSize().x/2.0f, m_window.getSize().y/2.0f);
                    
                    std::cout << "[Game] Game Over triggered!" << std::endl;
                }
            }

            delete u; 
            it = m_units.erase(it); 
        } else {
            ++it;
        }
    }
}

void Game::render() {
    // 【加锁】我们要读 m_units 来画图，防止读的时候被逻辑线程删掉了
    std::lock_guard<std::mutex> lock(m_mutex);

    m_window.clear();

    // 1. 绘制背景图
    m_window.draw(m_bgSprite);

    //2. 绘制半透明网格 (调试用，如果不想看格子可以注释掉这一段)
    //这里我们只绘制 基地、河流和桥梁的调试色块，平地设为透明以便看到背景图
    sf::RectangleShape tileShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    tileShape.setOutlineThickness(1.0f);
    tileShape.setOutlineColor(sf::Color(0, 0, 0, 50)); // 极淡的边框

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            tileShape.setPosition(c * TILE_SIZE, r * TILE_SIZE);
            int type = m_mapData[r][c];
            
            // 默认全透明，显示背景图
            tileShape.setFillColor(sf::Color::Transparent); 

            // 为特殊地形添加半透明遮罩，确认逻辑位置是否对齐
            if (type == RIVER) {
                tileShape.setFillColor(sf::Color(0, 0, 255, 100)); // 半透明蓝
            } else if (type == BRIDGE) {
                tileShape.setFillColor(sf::Color(139, 69, 19, 100)); // 半透明棕
            } else if (type == BASE_A) {
                tileShape.setFillColor(sf::Color(255, 0, 0, 150)); // 半透明红
            } else if (type == BASE_B) {
                tileShape.setFillColor(sf::Color(0, 0, 255, 150)); // 半透明蓝
            }

            // 只有非平地才画出来 (或者你可以画所有格子调试)
            if (type != GROUND) {
                m_window.draw(tileShape);
            }
        }
    }

    // 绘制废墟 (在单位下方，背景上方)
    for (const auto& ruin : m_ruins) {
        m_window.draw(ruin);
    }

    // 3. 绘制单位
    for (auto unit : m_units) {
        unit->render(m_window);
    }

    // 绘制子弹
    for (auto proj : m_projectiles) {
        proj->render(m_window);
    }

    // 绘制 UI
    renderUI();

    // 绘制难度文字
    m_window.draw(m_difficultyText);

    // 绘制游戏结束文字
    if (m_gameOver) {
        // 绘制一个半透明背景遮罩，让文字更清晰
        sf::RectangleShape overlay(sf::Vector2f(m_window.getSize().x, m_window.getSize().y));
        overlay.setFillColor(sf::Color(0, 0, 0, 150));
        m_window.draw(overlay);
        m_window.draw(m_gameOverText);
    }

    // 4. 显示窗口内容
    m_window.display();
}

// 绘制 UI
void Game::renderUI() {
    // 0. 绘制底板
    m_window.draw(m_uiBg);

    // 1. 绘制圣水条
    m_window.draw(m_elixirBarBg);
    
    // 计算前景条长度
    float pct = m_elixir / m_maxElixir;
    float maxW = m_elixirBarBg.getSize().x;
    m_elixirBarFg.setSize(sf::Vector2f(maxW * pct, m_elixirBarBg.getSize().y));
    m_window.draw(m_elixirBarFg);

    // 绘制圣水图标
    m_window.draw(m_elixirIcon);

    // 绘制圣水文字 (例如 "4 / 10")
    std::stringstream ss;
    ss << (int)m_elixir << " / " << (int)m_maxElixir;
    m_elixirStatusText.setString(ss.str());
    m_window.draw(m_elixirStatusText);

    // 2. 绘制卡牌
    for (const auto& card : m_deck) {
        m_window.draw(card.slotShape); // 卡槽背景
        m_window.draw(card.sprite);    // 卡牌图标
        m_window.draw(card.costText); // 绘制费用数字
        
        // 视觉提示：如果圣水不够，把卡牌稍微变暗 (可选)
        if (m_elixir < card.cost) {
            // 这里我们无法直接修改 card.sprite 的颜色，因为它是 const 引用 
            // 实际上 renderUI 不应该修改状态。
            // 简单的做法是画一个半透明黑色矩形遮罩
            sf::RectangleShape mask = card.slotShape;
            mask.setFillColor(sf::Color(0, 0, 0, 150)); // 半透明黑
            mask.setOutlineThickness(0);
            m_window.draw(mask);
        }
    }
}