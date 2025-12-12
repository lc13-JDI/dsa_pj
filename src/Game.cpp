#include "Game.h"
#include <iostream>
#include "Unit.h"
#include <chrono> // 用于线程休眠
#include "ResourceManager.h"

Game::Game() : m_running(false) {
    // 1. 加载资源
    ResourceManager::getInstance().loadAllAssets(); // 加载所有资源

    // 2. 初始化窗口和地图
    initWindow();
    initMap();
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

void Game::initMap() {
    // 1. 全部重置为平地
    m_mapData.assign(ROWS, std::vector<int>(COLS, GROUND));

    // 2. 设置河流 (假设在第 9 行，横贯整个地图)
    int riverRow = 10;
    for (int c = 0; c < COLS; c++) {
        m_mapData[riverRow][c] = RIVER;
    }

    // 3. 设置桥梁 (左桥和右桥)
    m_mapData[riverRow][10] = BRIDGE;// 左桥
    m_mapData[riverRow][14] = BRIDGE;// 右桥

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

void Game::initUnits() {
    // 调整生成位置以适应新地图
    // Team A 坦克
    Unit* tank = new Tank(10 * TILE_SIZE, 6 * TILE_SIZE, TEAM_A); // 国王塔前
    tank->setTarget(10 * TILE_SIZE, 13 * TILE_SIZE, m_mapData); 
    m_units.push_back(tank);

    // Team B 近战
    Unit* m1 = new Melee(9 * TILE_SIZE, 15 * TILE_SIZE, TEAM_B); // 左路
    m1->setTarget(9 * TILE_SIZE, 6 * TILE_SIZE, m_mapData);
    m_units.push_back(m1);
    
    // Team B 另一个近战
    Unit* m2 = new Melee(14 * TILE_SIZE, 15 * TILE_SIZE, TEAM_B); // 右路
    m2->setTarget(14 * TILE_SIZE, 6 * TILE_SIZE, m_mapData);
    m_units.push_back(m2);
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

        update(dt);
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
        unit->update(dt, m_units);
    }

    // 2. 清理尸体 (Remove Dead Units)
    auto it = m_units.begin();
    while (it != m_units.end()) {
        if ((*it)->isDead()) {
            delete *it; // 释放内存
            it = m_units.erase(it); // 从列表移除
        } else {
            ++it;
        }
    }
}

void Game::render() {
    // 【加锁】我们要读 m_units 来画图，防止读的时候被逻辑线程删掉了
    std::lock_guard<std::mutex> lock(m_mutex);

    m_window.clear();

    // 1. [新增] 绘制背景图
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

    // 3. 绘制单位
    for (auto unit : m_units) {
        unit->render(m_window);
    }

    // 4. 显示窗口内容
    m_window.display();
}