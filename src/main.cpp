#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

/**
 * Interfaz del Mod KugiBot
 * Muestra el panel flotante abajo a la derecha cuando se pausa el juego.
 */
class KugiBotWidget : public CCNode {
public:
    static KugiBotWidget* create() {
        auto ret = new KugiBotWidget();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() {
        if (!CCNode::init()) return false;

        this->setContentSize({130.0f, 75.0f});

        // Fondo semi-transparente elegante
        auto bg = CCScale9Sprite::create("square02_001.png");
        bg->setContentSize(this->getContentSize());
        bg->setOpacity(180);
        bg->setPosition(this->getContentSize() / 2);
        this->addChild(bg);

        // Título del Mod
        auto title = CCLabelBMFont::create("KugiBot", "goldFont.fnt");
        title->setScale(0.45f);
        title->setPosition({this->getContentSize().width / 2, this->getContentSize().height - 12.0f});
        this->addChild(title);

        // Estado del Bot
        bool botActive = Mod::get()->getSettingValue<bool>("bot-enabled");
        std::string statusStr = botActive ? "Bot: <g>ON</c>" : "Bot: <r>OFF</c>";
        auto statusLabel = MDTextArea::create(statusStr, {120.0f, 20.0f});
        statusLabel->setPosition({this->getContentSize().width / 2, 42.0f});
        this->addChild(statusLabel);

        // Menú de Botones
        auto menu = CCMenu::create();
        menu->setPosition({0, 0});
        this->addChild(menu);

        // Botón para abrir las configuraciones completas del Mod en Geode
        auto settingsSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        settingsSpr->setScale(0.5f);
        auto settingsBtn = CCMenuItemSpriteExtra::create(
            settingsSpr,
            this,
            menu_selector(KugiBotWidget::onOpenSettings)
        );
        settingsBtn->setPosition({this->getContentSize().width / 2, 18.0f});
        menu->addChild(settingsBtn);

        return true;
    }

    void onOpenSettings(CCObject*) {
        geode::openSettingsPopup(Mod::get());
    }
};

/**
 * Hook a PauseLayer para inyectar el Widget abajo a la derecha
 */
class $modify(KugiPauseHook, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        // Crear el widget de KugiBot abajo a la derecha
        auto widget = KugiBotWidget::create();
        widget->setPosition({winSize.width - 140.0f, 10.0f});
        widget->setID("kugi-bot-widget"_spr);
        this->addChild(widget, 100);
    }
};

/**
 * Hook a PlayerObject para manejar los Clics, Trazo y Velocidad
 */
class $modify(KugiPlayerHook, PlayerObject) {
    struct Fields {
        std::vector<CCPoint> m_trailPositions;
        size_t m_clickCount = 0;
        CCDrawNode* m_drawNode = nullptr;
    };

    void pushButton(PlayerButton btn) {
        PlayerObject::pushButton(btn);

        if (Mod::get()->getSettingValue<bool>("bot-enabled")) {
            m_fields->m_clickCount++;
            
            if (Mod::get()->getSettingValue<bool>("show-clicks")) {
                // Notificación visual rápida de clic registrado
                Notification::create(
                    fmt::format("Clic #{}: KugiBot", m_fields->m_clickCount),
                    NotificationIcon::Info,
                    0.5f
                )->show();
            }
        }
    }

    void update(float dt) {
        // Modificar velocidad si el bot está activo
        if (Mod::get()->getSettingValue<bool>("bot-enabled")) {
            double speedMult = Mod::get()->getSettingValue<double>("bot-speed");
            dt *= static_cast<float>(speedMult);
        }

        PlayerObject::update(dt);

        // Lógica para dibujar el trazo del bot
        if (Mod::get()->getSettingValue<bool>("bot-enabled") && Mod::get()->getSettingValue<bool>("show-trail")) {
            if (!m_fields->m_drawNode) {
                m_fields->m_drawNode = CCDrawNode::create();
                if (auto playLayer = PlayLayer::get()) {
                    playLayer->m_objectLayer->addChild(m_fields->m_drawNode, 999);
                }
            }

            // Guardar posición actual para la estela
            m_fields->m_trailPositions.push_back(this->getPosition());
            if (m_fields->m_trailPositions.size() > 100) {
                m_fields->m_trailPositions.erase(m_fields->m_trailPositions.begin());
            }

            // Dibujar el trazo en pantalla
            if (m_fields->m_drawNode && m_fields->m_trailPositions.size() > 1) {
                m_fields->m_drawNode->clear();
                for (size_t i = 0; i < m_fields->m_trailPositions.size() - 1; ++i) {
                    m_fields->m_drawNode->drawLine(
                        m_fields->m_trailPositions[i],
                        m_fields->m_trailPositions[i + 1],
                        ccc4f(0.0f, 0.8f, 1.0f, 0.7f), // Color Cyan brillante
                        1.5f
                    );
                }
            }
        }
    }

    void playerDestroyed(bool p0) {
        // Trazado e indicador de posibles muertes
        if (Mod::get()->getSettingValue<bool>("bot-enabled") && Mod::get()->getSettingValue<bool>("trace-deaths")) {
            if (auto playLayer = PlayLayer::get()) {
                auto deathMarker = CCSprite::createWithSpriteFrameName("GJ_deleteBtn_001.png");
                deathMarker->setPosition(this->getPosition());
                deathMarker->setScale(0.6f);
                playLayer->m_objectLayer->addChild(deathMarker, 1000);
            }
        }

        PlayerObject::playerDestroyed(p0);
    }
};

/**
 * Hook a PlayLayer para inicializar KugiBot
 */
class $modify(KugiPlayHook, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        log::info("KugiBot cargado correctamente en PlayLayer.");
        return true;
    }
};
