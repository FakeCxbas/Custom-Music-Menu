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
        if (lang == "Español") {
            return true;
        }
        if (lang == "English") {
            return false;
        }
        // Deteccion automatica por entorno/locale del sistema
        try {
            auto loc = std::locale("").name();
            for (auto& c : loc) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            if (loc.find("es") != std::string::npos || loc.find("spanish") != std::string::npos) {
                return true;
            }
        } catch (...) {}
        return false;
    }

    std::string getNowPlayingText(std::string_view songName) {
        if (isSpanish()) {
            return fmt::format("Reproduciendo: {}", songName);
        }
        return fmt::format("Now Playing: {}", songName);
    }

    std::string getErrorText() {
        if (isSpanish()) {
            return "¡No se pudo cargar el archivo de audio!";
        }
        return "Could not load audio file!";
    }

    void showSongNotification(std::filesystem::path const& path) {
        if (!Mod::get()->getSettingValue<bool>("show-notification")) {
            return;
        }
        auto filename = path.filename().string();
        Notification::create(getNowPlayingText(filename), NotificationIcon::Success, 1.5f)->show();
    }
}

static void playCustomMenuMusic(bool fadeIn = false, bool notify = false) {
    if (!Mod::get()->getSettingValue<bool>("enabled")) {
        return;
    }
    auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
    if (path.empty() || !std::filesystem::exists(path)) {
        return;
    }

    auto pathStr = geode::utils::string::pathToString(path);
    auto engine = FMODAudioEngine::sharedEngine();
    if (!engine) {
        return;
    }

    // Si ya esta sonando exactamente este archivo en el canal 0, no lo reiniciamos
    if (engine->isMusicPlaying(pathStr, 0)) {
        return;
    }

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
        if (!MenuLayer::init()) {
            return false;
        }

        // Cargamos la hoja de sprites de notas musicales por si no estaba en cache
        CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("GJ_GameSheet03.plist");

        // Reproducimos la musica personalizada si corresponde
        if (Mod::get()->getSettingValue<bool>("enabled")) {
            auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-music-file");
            if (!path.empty() && std::filesystem::exists(path)) {
                playCustomMenuMusic(false, false);
            }
        }

        // Boton integrado con textura nativa / texture packs
        if (Mod::get()->getSettingValue<bool>("show-menu-button")) {
            if (auto bottomMenu = this->getChildByID("bottom-menu")) {
                // CircleButtonSprite usa CircleBaseSize::MediumAlt (el mismo tamaño y base que Geode y botones del menu)
                // Esto garantiza compatibilidad 100% con Texture Packs (hexágonos, bordes oscuros, etc.)
                CircleButtonSprite* btnSpr = CircleButtonSprite::createWithSpriteFrameName(
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
    // Al cambiar de archivo de audio en la ventana de ajustes
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

    // Al habilitar o deshabilitar el mod
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

    // Al cambiar la opcion de bucle continuo
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
