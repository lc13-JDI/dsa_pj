#include "ResourceManager.h"
#include <iostream>
#include <filesystem>

ResourceManager& ResourceManager::getInstance() {
    static ResourceManager instance;
    return instance;
}

void ResourceManager::loadTexture(const std::string& name, const std::string& fileName) {
    // 检查是否已经加载过，防止重复加载
    if (m_textures.find(name) != m_textures.end()) {
        return; 
    }

    sf::Texture tex;
    if (tex.loadFromFile(fileName)) {
        m_textures[name] = tex;
        std::cout << "[ResourceManager] Loaded Texture: " << fileName << " as '" << name << "'" << std::endl;
    } else {
        std::cerr << "[ResourceManager] ERROR: Failed to load Texture: " << fileName << std::endl;

        // 检查文件到底存不存在
        if (!std::filesystem::exists(fileName)) {
            std::cerr << "    -> Reason: File does NOT exist at path!" << std::endl;
            std::cerr << "    -> Absolute Path: " << std::filesystem::absolute(fileName).string() << std::endl;
        } else {
            std::cerr << "    -> Reason: File exists but load failed (format error?)" << std::endl;
        }

        // [重要] 创建一个 32x32 的品红色方块作为“错误占位符”
        // 这样即使图没加载出来，游戏也能运行，你能看到紫色的方块
        sf::Image errorImage;
        errorImage.create(32, 32, sf::Color::Magenta);
        tex.loadFromImage(errorImage);
        m_textures[name] = tex; // 存入占位图 
    }
}

sf::Texture& ResourceManager::getTexture(const std::string& name) {
    if (m_textures.find(name) == m_textures.end()) {
        std::cerr << "[ResourceManager] CRITICAL: Texture not found: " << name << std::endl;
        // 如果找不到，为了不让程序直接崩溃，这里可能会抛出异常或者返回一个默认的空纹理
        // 这里简单处理：直接抛出异常，强迫开发者修复路径错误
        throw std::runtime_error("Texture not found: " + name);
    }
    return m_textures.at(name);
}

void ResourceManager::loadSoundBuffer(const std::string& name, const std::string& fileName) {
    if (m_soundBuffers.find(name) != m_soundBuffers.end()) {
        return;
    }

    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(fileName)) {
        m_soundBuffers[name] = buffer;
        std::cout << "[ResourceManager] Loaded Sound: " << fileName << " as '" << name << "'" << std::endl;
    } else {
        std::cerr << "[ResourceManager] ERROR: Failed to load Sound: " << fileName << std::endl;
    }
}

sf::SoundBuffer& ResourceManager::getSoundBuffer(const std::string& name) {
    if (m_soundBuffers.find(name) == m_soundBuffers.end()) {
        std::cerr << "[ResourceManager] CRITICAL: SoundBuffer not found: " << name << std::endl;
        throw std::runtime_error("SoundBuffer not found: " + name);
    }
    return m_soundBuffers.at(name);
}

// 字体加载实现
void ResourceManager::loadFont(const std::string& name, const std::string& fileName) {
    if (m_fonts.find(name) != m_fonts.end()) return;

    sf::Font font;
    if (font.loadFromFile(fileName)) {
        m_fonts[name] = font;
        std::cout << "[ResourceManager] Loaded Font: " << fileName << " as '" << name << "'" << std::endl;
    } else {
        std::cerr << "[ResourceManager] ERROR: Failed to load Font: " << fileName << std::endl;
    }
}

sf::Font& ResourceManager::getFont(const std::string& name) {
    if (m_fonts.find(name) == m_fonts.end()) {
        std::cerr << "[ResourceManager] CRITICAL: Font not found: " << name << std::endl;
        throw std::runtime_error("Font not found: " + name);
    }
    return m_fonts.at(name);
}

void ResourceManager::loadAllAssets() {
    std::cout << "--- Loading Assets ---" << std::endl;

    // 1. 地图与UI
    loadTexture("background", "assets/textures/Background.png");
    loadTexture("main_bg", "assets/textures/mainBackground.png"); // 可能用于菜单
    loadTexture("ui_heart", "assets/textures/heart.png");
    loadTexture("ui_elixir", "assets/textures/elixirCost.png");
    loadTexture("ui_add", "assets/textures/addCard.png");
    loadTexture("ui_remove", "assets/textures/removeCard.png");
    loadTexture("vfx_damaged", "assets/textures/damaged_area.png");
    loadTexture("ui_crown", "assets/textures/life_bar_crown.png");
    
    // 2. 投射物
    loadTexture("bullet", "assets/textures/bullet.png");
    // 箭矢动画
    loadTexture("arrow_sheet", "assets/textures/arrows_spritesheet.png");

    // 3. 士兵 (Spritesheets - 动态图)
    loadTexture("unit_archers", "assets/textures/archers_spritesheet.png");
    loadTexture("unit_dartgoblin", "assets/textures/dartGoblin_spritesheet.png");
    loadTexture("unit_giant", "assets/textures/giant_spritesheet.png");
    loadTexture("unit_knight", "assets/textures/knight_spritesheet.png");
    loadTexture("unit_pekka", "assets/textures/pekka_spritesheet.png");
    loadTexture("unit_valkyrie", "assets/textures/valkyrie_spritesheet.png");

    // 4. 士兵 (Static - 卡牌图标/头像)
    loadTexture("icon_archers", "assets/textures/archers.png");
    loadTexture("icon_dartgoblin", "assets/textures/dart_goblin.png");
    loadTexture("icon_giant", "assets/textures/giant.png");
    loadTexture("icon_knight", "assets/textures/knight.png");
    loadTexture("icon_pekka", "assets/textures/pekka.png");
    loadTexture("icon_valkyrie", "assets/textures/valkyrie.png");

    // 5. 音频 - 部署 (Deploy)
    loadSoundBuffer("sfx_deploy_archers", "assets/audio/archers_deploy_sound.ogg");
    loadSoundBuffer("sfx_deploy_dartgoblin", "assets/audio/dartgoblin_deploy_sound.ogg");
    loadSoundBuffer("sfx_deploy_giant", "assets/audio/giant_deploy_sound.ogg");
    loadSoundBuffer("sfx_deploy_knight", "assets/audio/knight_deploy_sound.ogg");
    loadSoundBuffer("sfx_deploy_pekka", "assets/audio/pekka_deploy_sound.ogg");
    loadSoundBuffer("sfx_deploy_valkyrie", "assets/audio/valkyrie_deploy_sound.ogg");

    // 6. 音频 - 攻击/受击 (Hit)
    loadSoundBuffer("sfx_hit_archers", "assets/audio/archers_hit_sound.ogg");
    loadSoundBuffer("sfx_hit_dartgoblin", "assets/audio/dartgoblin_hit_sound.ogg");
    loadSoundBuffer("sfx_hit_giant", "assets/audio/giant_hit_sound.ogg");
    loadSoundBuffer("sfx_hit_knight", "assets/audio/knight_hit_sound.ogg");
    loadSoundBuffer("sfx_hit_pekka", "assets/audio/pekka_hit_sound.ogg");
    loadSoundBuffer("sfx_hit_valkyrie", "assets/audio/valkyrie_hit_sound.ogg");

    // 6. 加载字体
    loadFont("main_font", "assets/fonts/Supercell_Magic_Regular.ttf");

    std::cout << "--- Assets Loading Complete ---" << std::endl;
}