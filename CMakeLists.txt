cmake_minimum_required(VERSION 3.16)

# 项目名称
project(BattleSim)

# 1. 设置 C++ 标准
# 建议使用 C++17 或 C++20，方便使用高级特性
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 使用 FetchContent 拉取 SFML
include(FetchContent)

message(STATUS "正在拉取 SFML 源码... (首次编译可能需要几分钟)")

FetchContent_Declare(
    sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        2.6.2  
)

# 这一步会下载并立刻配置 SFML
FetchContent_MakeAvailable(sfml)

# 2. 处理头文件路径
# 告诉 CMake 去哪里找 .h 文件
include_directories(${CMAKE_SOURCE_DIR}/include)
#include_directories(${CMAKE_SOURCE_DIR}/extern) # 如果你有第三方库头文件



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
target_link_libraries(BattleSim PRIVATE
    sfml-graphics 
    sfml-window 
    sfml-system
    sfml-audio
)


add_custom_command(TARGET BattleSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:sfml-audio>
        # 如果用到了 network，记得把下面这行取消注释
        # $<TARGET_FILE:sfml-network> 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying SFML DLLs to executable directory..."
)

