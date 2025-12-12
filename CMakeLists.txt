cmake_minimum_required(VERSION 3.16)

# 项目名称
project(BattleSim)

# 1. 设置 C++ 标准
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
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# 4. 查找并链接系统线程库 (多线程必须)
find_package(Threads REQUIRED)

# 5. 生成可执行文件
add_executable(BattleSim ${SOURCES})

# 6. 链接库文件 (关键步骤)
target_link_libraries(BattleSim PRIVATE Threads::Threads)
target_link_libraries(BattleSim PRIVATE
    sfml-graphics 
    sfml-window 
    sfml-system
    sfml-audio
)

# --- 【关键修复】 自动判断架构并定位 OpenAL DLL ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SFML_ARCH_DIR "x64")
else()
    set(SFML_ARCH_DIR "x86")
endif()

# 这里的路径是基于 FetchContent 下载下来的 SFML 源码结构的
set(OPENAL_DLL_PATH "${sfml_SOURCE_DIR}/extlibs/bin/${SFML_ARCH_DIR}/openal32.dll")


add_custom_command(TARGET BattleSim POST_BUILD
    # [操作1] 拷贝 SFML 动态库
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:sfml-audio> 
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    
    # [操作2] 拷贝 OpenAL (修复 Audio 模块导致的启动失败)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENAL_DLL_PATH}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>

    # [操作3] 递归拷贝 assets 文件夹
    # 语法：cmake -E copy_directory <源路径> <目标路径>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets

    COMMENT "正在拷贝 SFML DLL 和 assets 资源文件夹..."
)

