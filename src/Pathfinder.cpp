#include "Pathfinder.h"
#include <queue>
#include <map>
#include <algorithm>
#include <iostream>

// 比较器，用于让 sf::Vector2i 可以作为 std::map 的 key
struct Vector2iComparator {
    bool operator()(const sf::Vector2i& a, const sf::Vector2i& b) const {
        if (a.x != b.x) return a.x < b.x;
        return a.y < b.y;
    }
};

// 优先队列的节点结构
struct Node {
    sf::Vector2i pos; // 坐标
    int priority;     // F 值 (G + H)

    // 优先队列默认是大顶堆，我们需要小顶堆（F值越小越优先）
    // 所以这里的 > 实际上是让 F 值小的排在前面
    bool operator>(const Node& other) const {
        return priority > other.priority;
    }
};

// 启发函数：曼哈顿距离 (Manhattan Distance)
// 适用于只能上下左右移动的网格地图
int heuristic(sf::Vector2i a, sf::Vector2i b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool Pathfinder::isValid(const std::vector<std::vector<int>>& mapData, int r, int c) {
    int rows = mapData.size();
    int cols = mapData[0].size();
    
    // 1. 越界检查
    if (r < 0 || r >= rows || c < 0 || c >= cols) return false;

    // 2. 障碍物检查
    int type = mapData[r][c];
    // 河流(RIVER) 和 山脉(MOUNTAIN) 不可走
    // 地面(GROUND)、桥(BRIDGE)、基地(BASE) 可走
    if (type == RIVER || type == MOUNTAIN) return false;

    return true;
}

std::vector<sf::Vector2i> Pathfinder::findPath(
    const std::vector<std::vector<int>>& mapData, 
    sf::Vector2i start, 
    sf::Vector2i end
) {
    std::vector<sf::Vector2i> path;
    
    // 如果起点或终点本身无效，直接返回空
    if (!isValid(mapData, start.y, start.x) || !isValid(mapData, end.y, end.x)) {
        std::cout << "[Pathfinder] Start or End is invalid!" << std::endl;
        return path;
    }

    // 1. 优先队列 (Open Set)
    // 存储待遍历的节点，F 值最小的在队首
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    openSet.push({start, 0});

    // 2. 记录路径来源 (Came From)
    // key: 当前点, value: 上一个点 (用于回溯路径)
    std::map<sf::Vector2i, sf::Vector2i, Vector2iComparator> cameFrom;

     // 3. 记录当前花费 (Cost So Far / G Cost)
    // key: 坐标, value: 从起点到该点的实际代价
    std::map<sf::Vector2i, int, Vector2iComparator> costSoFar;

    // 初始化
    cameFrom[start] = start;
    costSoFar[start] = 0;

    bool found = false;

    // 上下左右四个方向 (row, col)
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    while (!openSet.empty()) {
        // 取出 F 值最小的节点
        sf::Vector2i current = openSet.top().pos;
        openSet.pop();

        // 找到终点？提前结束
        if (current == end) {
            found = true;
            break;
        }

        // 遍历邻居
        for (int i = 0; i < 4; i++) {
            int nextR = current.y + dr[i];
            int nextC = current.x + dc[i];
            sf::Vector2i next(nextC, nextR);

            // 如果邻居是障碍物，跳过
            if (!isValid(mapData, nextR, nextC)) {
                continue;
            }

            // 计算新的 G 值 (假设每格代价为 1)
            int newCost = costSoFar[current] + 1;

            // 如果该邻居没访问过，或者发现了更短的路径到达该邻居
            if (costSoFar.find(next) == costSoFar.end() || newCost < costSoFar[next]) {
                
                // 更新 G 值
                costSoFar[next] = newCost;
                
                // 计算 F 值 = G + H
                int priority = newCost + heuristic(next, end);
                
                // 加入优先队列
                openSet.push({next, priority});
                
                // 记录父节点
                cameFrom[next] = current;
            }
        }
    }

    // 路径重构 (回溯)
    if (found) {
        sf::Vector2i curr = end;
        while (curr != start) {
            path.push_back(curr);
            curr = cameFrom[curr];
        }
        // path 目前是 终点 -> 起点
        // 反转，变成 起点(不含) -> ... -> 终点
        std::reverse(path.begin(), path.end());
    } else {
        // std::cout << "[Pathfinder] 未找到路径!" << std::endl;
    }

    return path;
}