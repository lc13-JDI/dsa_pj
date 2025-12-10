#pragma once
#include <vector>
#include <SFML/System.hpp>
#include "Game.h" // 为了获取 TileType 枚举

class Pathfinder {
public:
    // 核心函数：输入地图、起点、终点，返回路径点列表
    static std::vector<sf::Vector2i> findPath(
        const std::vector<std::vector<int>>& mapData, 
        sf::Vector2i start, 
        sf::Vector2i end
    );

private:
    // 辅助结构：检查坐标是否越界或不可通行
    static bool isValid(const std::vector<std::vector<int>>& mapData, int r, int c);
};