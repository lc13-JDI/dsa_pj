cmake_minimum_required(VERSION 3.10)

# 项目名称
project(DSA_PJ)

# 1. 设置 C++ 标准
# 建议使用 C++17 或 C++20，方便使用高级特性
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 2. 处理头文件路径
# 告诉 CMake 去哪里找 .h 文件
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/extern) # 如果你有第三方库头文件

# 3. 自动扫描所有源文件
# 将 src 目录下所有的 .cpp 文件收集到 SOURCES 变量中
# 注意：如果你添加了新文件，有时需要重新运行 CMake Configure
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 4. 查找并链接系统线程库 (多线程必须)
find_package(Threads REQUIRED)

# 5. 生成可执行文件
add_executable(BattleSim ${SOURCES})

# 6. 链接库文件 (关键步骤)
# 链接多线程库
target_link_libraries(BattleSim PRIVATE Threads::Threads)

# --- 下面是图形库配置区域 (以 EasyX 为例，如果不用可以注释掉) ---
# 假设你把 easyx.lib 放到了 lib 文件夹下
# link_directories(${CMAKE_SOURCE_DIR}/lib)
# target_link_libraries(BattleSim PRIVATE easyx) 
# 注意：EasyX 需要链接一些 Windows 系统库
# target_link_libraries(BattleSim PRIVATE -luser32 -lgdi32 -lwinmm)
# -----------------------------------------------------------