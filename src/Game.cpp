#include "Game.h"
#include <iostream>
#include "Unit.h"
#include <chrono> // 用于线程休眠

Game::Game() : m_running(false) {
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
}

void Game::initMap() {
    // 1. 初始化所有格子为平地 (GROUND)
    m_mapData.resize(ROWS, std::vector<int>(COLS, GROUND));

    // 2. 绘制河流 (RIVER) - 假设在地图中间的第 9 和 10 行
    for (int c = 0; c < COLS; c++) {
        m_mapData[9][c] = RIVER;
        m_mapData[10][c] = RIVER;
    }

    // 3. 绘制桥梁 (BRIDGE) - 左侧桥和右侧桥
    // 左桥 (第3列)
    m_mapData[9][3] = BRIDGE;
    m_mapData[10][3] = BRIDGE;
    // 右桥 (第11列)
    m_mapData[9][11] = BRIDGE;
    m_mapData[10][11] = BRIDGE;

    // 4. 设置基地 (BASE)
    // 甲方基地 (上方正中)
    m_mapData[1][COLS / 2] = BASE_A;
    // 乙方基地 (下方正中)
    m_mapData[ROWS - 2][COLS / 2] = BASE_B;

    // --- 添加一些复杂的山脉，测试寻路 ---
    // 在左边造一堵墙，强制左边的兵绕远路去右边的桥
    m_mapData[5][0] = MOUNTAIN;
    m_mapData[5][1] = MOUNTAIN;
    m_mapData[5][2] = MOUNTAIN;
    m_mapData[5][3] = MOUNTAIN; // 封住左边路口的一部分

    std::cout << "[Info] Map initialized (" << ROWS << "x" << COLS << ")" << std::endl;
}

void Game::initUnits() {
    // 生成两个会相遇的兵，测试战斗
    // 1. 左下位置生成一个坦克
    Unit* tank = new Tank(3 * TILE_SIZE, 15 * TILE_SIZE, TEAM_A);
    tank->setTarget(3 * TILE_SIZE, 5 * TILE_SIZE, m_mapData); 
    m_units.push_back(tank);

    // 2. 左上位置生成两个近战兵
    Unit* m1 = new Melee(3 * TILE_SIZE, 5 * TILE_SIZE, TEAM_B);
    m1->setTarget(3 * TILE_SIZE, 15 * TILE_SIZE, m_mapData);
    m_units.push_back(m1);
    
    Unit* m2 = new Melee(4 * TILE_SIZE, 5 * TILE_SIZE, TEAM_B); // 稍微错开一点
    m2->setTarget(3 * TILE_SIZE, 15 * TILE_SIZE, m_mapData);
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

    // 创建一个通用的矩形形状用于绘制格子
    sf::RectangleShape tileShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    
    // 设置格子的边框，这样能看清网格线
    tileShape.setOutlineThickness(1.0f);
    tileShape.setOutlineColor(sf::Color(50, 50, 50)); // 深灰色边框

    // 遍历地图数据并绘制
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            // 设置位置
            tileShape.setPosition(c * TILE_SIZE, r * TILE_SIZE);

            // 根据地形类型设置颜色
            int type = m_mapData[r][c];
            switch (type) {
                case GROUND:
                    tileShape.setFillColor(sf::Color(100, 200, 100)); // 浅绿
                    break;
                case RIVER:
                    tileShape.setFillColor(sf::Color(50, 150, 250)); // 蓝色
                    break;
                case BRIDGE:
                    tileShape.setFillColor(sf::Color(160, 82, 45)); // 棕色/木色
                    break;
                case MOUNTAIN:
                    tileShape.setFillColor(sf::Color(100, 100, 100)); // 灰色
                    break;
                case BASE_A:
                    tileShape.setFillColor(sf::Color(200, 50, 50)); // 红色 (敌方/甲方)
                    break;
                case BASE_B:
                    tileShape.setFillColor(sf::Color(50, 50, 200)); // 蓝色 (我方/乙方)
                    break;
                default:
                    tileShape.setFillColor(sf::Color::White);
                    break;
            }

            // 绘制
            m_window.draw(tileShape);
        }
    }

    // 绘制所有单位
    for (auto unit : m_units) {
        unit->render(m_window);
    }

    m_window.display();
}