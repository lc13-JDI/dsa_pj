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

    // BFS 队列
    std::queue<sf::Vector2i> q;
    q.push(start);

    // 记录路径来源：cameFrom[current] = parent
    // 用于最后回溯路径
    std::map<sf::Vector2i, sf::Vector2i, Vector2iComparator> cameFrom;
    std::map<sf::Vector2i, bool, Vector2iComparator> visited;

    visited[start] = true;
    // 标记起点的来源是它自己（或者无效值），用于结束回溯
    cameFrom[start] = start; 

    bool found = false;

    // 上下左右四个方向 (row, col)
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    while (!q.empty()) {
        sf::Vector2i current = q.front();
        q.pop();

        // 找到终点？
        if (current == end) {
            found = true;
            break;
        }

        // 遍历邻居
        for (int i = 0; i < 4; i++) {
            // 注意：sf::Vector2i 的 x 是列(col), y 是行(row)
            int nextR = current.y + dr[i];
            int nextC = current.x + dc[i];
            sf::Vector2i nextNode(nextC, nextR);

            if (isValid(mapData, nextR, nextC) && !visited[nextNode]) {
                visited[nextNode] = true;
                cameFrom[nextNode] = current; // 记录父节点
                q.push(nextNode);
            }
        }
    }

    if (found) {
        // 回溯重构路径
        sf::Vector2i curr = end;
        while (curr != start) {
            path.push_back(curr);
            curr = cameFrom[curr];
        }
        // path 目前是 终点 -> 起点，不需要把起点加进去（因为我们已经在起点了）
        
        // 反转，变成 起点(不含) -> ... -> 终点
        std::reverse(path.begin(), path.end());
    } else {
        std::cout << "[Pathfinder] No path found!" << std::endl;
    }

    return path;
}