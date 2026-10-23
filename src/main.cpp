#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

namespace {
    bool isSpanish() {
        auto lang = Mod::get()->getSettingValue<std::string>("language");
        if (lang == "Español") return true;
        if (lang == "English") return false;

        try {
            auto loc = std::locale("").name();
            for (auto& c : loc) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return loc.find("es") != std::string::npos || loc.find("spanish") != std::string::npos;
        } catch (...) {
            return false;
        }
    }

    void showSongNotification(std::filesystem::path const& path) {
        if (!Mod::get()->getSettingValue<bool>("show-notification")) return;

        auto name = path.filename().string();
        auto text = isSpanish() 
            ? fmt::format("Reproduciendo: {}", name) 
            : fmt::format("Now Playing: {}", name);

        Notification::create(text, NotificationIcon::Success, 1.5f)->show();
    }
}

static void playCustomMenuMusic(bool fadeIn = false, bool notify = false) {
    if (!Mod::get()->getSettingValue<bool>("enabled")) return;

    auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
    if (path.empty() || !std::filesystem::exists(path)) return;

    auto engine = FMODAudioEngine::sharedEngine();
    if (!engine) return;

    auto pathStr = geode::utils::string::pathToString(path);
    if (engine->isMusicPlaying(pathStr, 0)) return;

    bool loop = Mod::get()->getSettingValue<bool>("loop");
    float fadeTime = fadeIn ? 1.0f : 0.0f;
    engine->playMusic(pathStr, loop, fadeTime, 0);

    if (notify) {
        showSongNotification(path);
    }
}

class $modify(CustomGameManager, GameManager) {
    void playMenuMusic() {
        if (Mod::get()->getSettingValue<bool>("enabled")) {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
            if (!path.empty() && std::filesystem::exists(path)) {
                playCustomMenuMusic(false, false);
                return;
            }
        }
        GameManager::playMenuMusic();
    }

    void fadeInMenuMusic() {
        if (Mod::get()->getSettingValue<bool>("enabled")) {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
            if (!path.empty() && std::filesystem::exists(path)) {
                playCustomMenuMusic(true, false);
                return;
            }
        }
        GameManager::fadeInMenuMusic();
    }
};

class $modify(CustomMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("GJ_GameSheet03.plist");

        if (Mod::get()->getSettingValue<bool>("enabled")) {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
            if (!path.empty() && std::filesystem::exists(path)) {
                playCustomMenuMusic(false, false);
            }
        }

        if (Mod::get()->getSettingValue<bool>("show-menu-button")) {
            if (auto bottomMenu = this->getChildByID("bottom-menu")) {
                auto btnSpr = CircleButtonSprite::createWithSpriteFrameName(
                    "GJ_noteIcon_001.png",
                    0.85f,
                    CircleBaseColor::Green,
                    CircleBaseSize::MediumAlt
                );
                if (!btnSpr) {
                    btnSpr = CircleButtonSprite::createWithSpriteFrameName(
                        "GJ_musicIcon_001.png",
                        0.85f,
                        CircleBaseColor::Green,
                        CircleBaseSize::MediumAlt
                    );
                }
                if (!btnSpr) {
                    btnSpr = CircleButtonSprite::createWithSpriteFrameName(
                        "GJ_musicOnBtn_001.png",
                        0.85f,
                        CircleBaseColor::Green,
                        CircleBaseSize::MediumAlt
                    );
                }

                if (btnSpr) {
                    auto btn = CCMenuItemSpriteExtra::create(
                        btnSpr, this, menu_selector(CustomMenuLayer::onCustomMenuMusicBtn)
                    );
                    btn->setID("custom-menu-music-button"_spr);
                    bottomMenu->addChild(btn);
                    bottomMenu->updateLayout();
                }
            }
        }

        return true;
    }

    void onCustomMenuMusicBtn(CCObject*) {
        geode::openSettingsPopup(Mod::get());
    }
};

$execute {
    listenForSettingChanges<std::filesystem::path>("custom-music-file", +[](std::filesystem::path const& path) {
        if (auto menuLayer = MenuLayer::get()) {
            if (Mod::get()->getSettingValue<bool>("enabled") && !path.empty() && std::filesystem::exists(path)) {
                if (auto engine = FMODAudioEngine::sharedEngine()) {
                    bool loop = Mod::get()->getSettingValue<bool>("loop");
                    engine->playMusic(geode::utils::string::pathToString(path), loop, 0.3f, 0);
                    showSongNotification(path);
                }
            } else {
                GameManager::sharedState()->playMenuMusic();
            }
        }
    });

    listenForSettingChanges<bool>("enabled", +[](bool enabled) {
        if (auto menuLayer = MenuLayer::get()) {
            if (enabled) {
                auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
                if (!path.empty() && std::filesystem::exists(path)) {
                    if (auto engine = FMODAudioEngine::sharedEngine()) {
                        bool loop = Mod::get()->getSettingValue<bool>("loop");
                        engine->playMusic(geode::utils::string::pathToString(path), loop, 0.3f, 0);
                        showSongNotification(path);
                        return;
                    }
                }
            }
            GameManager::sharedState()->playMenuMusic();
        }
    });

    listenForSettingChanges<bool>("loop", +[](bool loop) {
        if (auto menuLayer = MenuLayer::get()) {
            if (Mod::get()->getSettingValue<bool>("enabled")) {
                auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
                if (!path.empty() && std::filesystem::exists(path)) {
                    if (auto engine = FMODAudioEngine::sharedEngine()) {
                        engine->playMusic(geode::utils::string::pathToString(path), loop, 0.3f, 0);
                    }
                }
            }
        }
    });
}
