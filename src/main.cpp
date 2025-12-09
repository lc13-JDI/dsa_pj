#include <iostream>
#include <thread> // 测试多线程库是否链接成功
#include <windows.h> // 用于解决中文乱码
#include "Game.h"

void threadTask() {
    std::cout << "[子线程] 正在加载资源..." << std::endl;
}

int main() {
    // 解决 Windows 控制台中文乱码 (设为 UTF-8)
    SetConsoleOutputCP(65001);

    std::cout << "[主线程] 程序开始" << std::endl;

    // 测试多线程
    std::thread t(threadTask);
    t.join(); // 等待子线程结束

    // 测试多文件调用
    Game myGame;
    myGame.start();

    system("pause");
    return 0;
}