#include "Game.h"
#include <iostream>

Game::Game() {
    initWindow();
    initMap();
    initUnits(); // 初始化单位
}

Game::~Game() {
    // 释放所有动态分配的单位内存
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

    // 5. 添加一些山脉 (MOUNTAIN) - 这里手动添加几个障碍物示例
    m_mapData[5][3] = MOUNTAIN;
    m_mapData[5][11] = MOUNTAIN;
    m_mapData[14][5] = MOUNTAIN;
    m_mapData[14][9] = MOUNTAIN;

    std::cout << "[Info] Map initialized (" << ROWS << "x" << COLS << ")" << std::endl;
}

void Game::initUnits() {
    // --- 场景演示 ---
    // 左侧：红方大军进攻
    
    // 1. 一个肉盾 (Tank) 走在最前面
    Unit* tankA = new Tank(3 * TILE_SIZE + 20, 2 * TILE_SIZE, TEAM_A);
    tankA->setTarget(3 * TILE_SIZE + 20, 18 * TILE_SIZE); // 目标：冲到底部
    m_units.push_back(tankA);

    // 2. 一个远程 (Ranged) 跟在后面
    // 注意：远程跑得快，你可以观察它是否会追上前面的坦克 (目前没写碰撞，会直接穿过去)
    Unit* rangedA = new Ranged(3 * TILE_SIZE + 20, 0 * TILE_SIZE, TEAM_A);
    rangedA->setTarget(3 * TILE_SIZE + 20, 18 * TILE_SIZE);
    m_units.push_back(rangedA);


    // 右侧：蓝方防守
    
    // 3. 两个近战 (Melee) 并排冲锋
    Unit* meleeB1 = new Melee(10 * TILE_SIZE + 20, 15 * TILE_SIZE, TEAM_B);
    meleeB1->setTarget(10 * TILE_SIZE + 20, 2 * TILE_SIZE); // 目标：冲到顶部
    m_units.push_back(meleeB1);

    Unit* meleeB2 = new Melee(12 * TILE_SIZE + 20, 15 * TILE_SIZE, TEAM_B);
    meleeB2->setTarget(12 * TILE_SIZE + 20, 2 * TILE_SIZE); 
    m_units.push_back(meleeB2);

    std::cout << "[Info] Units initialized: Tank, Ranged, and Melee created." << std::endl;
}

void Game::run() {
    while (m_window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            m_window.close();
    }
}

void Game::update() {
    // 1. 计算时间增量 (Delta Time)
    // restart() 会返回自上次调用后经过的时间并重置计时器
    m_deltaTime = m_dtClock.restart().asSeconds();

    // 2. 更新所有单位
    for (auto unit : m_units) {
        unit->update(m_deltaTime);
    }
}

void Game::render() {
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