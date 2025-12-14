#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <string>

// 单例模式资源管理器
class ResourceManager {
public:
    // 获取全局唯一的实例
    static ResourceManager& getInstance();

    // 禁止拷贝
    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    // --- 纹理 (图片) 管理 ---
    // name: 给资源起的别名 (例如 "knight_sheet")
    // fileName: 文件路径 (例如 "assets/textures/knight_spritesheet.png")
    void loadTexture(const std::string& name, const std::string& fileName);
    // 获取纹理引用，用于 Sprite 设置
    sf::Texture& getTexture(const std::string& name);

    // --- 音效 管理 ---
    void loadSoundBuffer(const std::string& name, const std::string& fileName);
    sf::SoundBuffer& getSoundBuffer(const std::string& name);

    // 字体管理
    void loadFont(const std::string& name, const std::string& fileName);
    sf::Font& getFont(const std::string& name);


    // --- 统一加载 ---
    // 一次性加载项目中所有已知的资源
    void loadAllAssets();

private:
    ResourceManager() = default; // 私有构造函数

    // 资源管理器数据
    std::map<std::string, sf::Texture> m_textures;
    std::map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::map<std::string, sf::Font> m_fonts;
};