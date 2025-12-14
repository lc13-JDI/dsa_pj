#include "Game.h"
#include <iostream>
#include "Unit.h"
#include "Tower.h"
#include "Projectile.h"
#include <chrono> // 用于线程休眠
#include "ResourceManager.h"

Game::Game() : m_running(false) {
    // 1. 加载资源
    ResourceManager::getInstance().loadAllAssets(); // 加载所有资源

    // 2. 初始化窗口和地图
    initWindow();
    initMap();
    initUI();
    initTowers(); // 先初始化塔
    initUnits(); // 初始化单位
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

void Game::initWindow() {
    // 根据地图大小动态计算窗口分辨率
    int width = COLS * TILE_SIZE;
    int height = ROWS * TILE_SIZE;

    // 创建窗口，标题为 Battle Simulation
    m_window.create(sf::VideoMode(width, height), "Battle Simulation - Phase 1");
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
    // 初始化游戏结束文字
    try {
        // 使用引用，防止拷贝
        sf::Font& font = ResourceManager::getInstance().getFont("main_font");
        m_gameOverText.setFont(font);
        m_gameOverText.setCharacterSize(60);
        m_gameOverText.setFillColor(sf::Color::White);
        m_gameOverText.setOutlineColor(sf::Color::Black);
        m_gameOverText.setOutlineThickness(3.f);
        m_gameOverText.setString(""); // 初始为空
    } catch (...) {
        std::cerr << "[Game] Failed to set font for UI." << std::endl;
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

    std::cout << "[Info] Map initialized with CR layout." << std::endl;
}

// 【新增】根据地图生成塔对象
void Game::initTowers() {
    // 遍历地图寻找 BASE_A 和 BASE_B
    // 注意：地图上的点是格子的，我们需要把它转成 Tower 对象
    // 为了避免重复生成 (比如一个大基地占了多个格子)，我们这里手动指定坐标生成
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

    std::cout << "[Info] Towers initialized." << std::endl;
}

void Game::initUnits() {
    // 【修改】初始化时设置战略目标 (塔的坐标)
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

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            m_window.close();
    }
}

void Game::update(float dt) {
    // 【加锁】因为我们要读取和修改 m_units，渲染线程也在读
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. 更新所有单位状态 (移动、攻击)
    for (auto unit : m_units) {
        // 传入 m_units 供单位寻找敌人
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

    // 4. 清理尸体 (Remove Dead Units)
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
                ruin.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                ruin.setPosition(t->getPosition());
                // 稍微缩放一点以适应格子
                ruin.setScale(0.5f, 0.5f);
                
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