#include <iostream>
#include <SFML/Graphics.hpp>

int main()
{
    std::cout << "[Debug] 程序开始运行..." << std::endl;

    // 创建一个窗口，分辨率 800x600，标题为 "Battle Simulation"
    sf::RenderWindow window(sf::VideoMode(800, 600), "Battle Simulation");

    window.setFramerateLimit(60);

    // 创建一个绿色的圆
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);

    // 游戏主循环
    while (window.isOpen())
    {
        sf::Event event;
        // 处理事件（比如按关闭按钮）
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // 1. 清除上一帧
        window.clear();
        // 2. 绘制内容
        window.draw(shape);
        // 3. 显示到屏幕
        window.display();
    }

    return 0;
}