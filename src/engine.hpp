#pragma once

#include <vector>
#include <deque>
#include <set>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "world.hpp"
#include "spaces.hpp"

// Вся информация о текстуре и её инициализации
struct TextureInfo {
    const char *source;
    sf::Texture texture;
    TextureInfo(const char *_source) : source(_source) {}
    bool init();
};

// Вся информация о спрайте и его инициализации
struct SpriteInfo {
    const TextureInfo *texture;
    sf::IntRect txrect;
    sf::Vector2u offset;
    sf::Vector2f spot;
    sf::Sprite sprite;
    SpriteInfo(
        const TextureInfo &_texture,
        int txx, int txy, int txw, int txh,
        unsigned ofsx, unsigned ofsy,
        float spx, float spy
    ) : texture(&_texture), txrect(txx, txy, txw, txh), offset(ofsx, ofsy), spot(spx, spy) {}
    bool init();
};

// Вся информация о звуке и его инициализации
struct SoundInfo {
    const char *source;
    sf::SoundBuffer buffer;
    SoundInfo(const char *source) : source(source) {}
    bool init();
};

// Воспроизводящиеся звуки
class Orchestre
{
    std::deque<sf::Sound> sounds;
public:
    Orchestre() {}
    void play(const sf::SoundBuffer&);
    void update();
};

struct DrawingSizes
{
    float spr_scale;
    float bkg_scale;
    int screen_w;
    int screen_h;
    int room_w;
    int room_h;
    int lc_ofst; // Реальный отступ левого угла комнаты от края экрана
    int tc_ofst; // Отступ верхнего угла комнаты от края экрана
};

////////////////////////////////////////////////////////////////////////////////

class Engine
{
    enum ControlState {
        csLMBUTTON,
        csRMBUTTON
    };

    sf::RenderWindow* window;
    sf::Font font;
    std::vector<TextureInfo> textures;
    std::vector<SpriteInfo> sprites;
    std::vector<SoundInfo> sounds;
    std::set<ControlState> controls;
    ScreenPosition mouse_p;
    Orchestre played_sounds;
    float banner_timeout;
    bool windowed;
public:
    DrawingSizes sizes;

    Engine();
    ~Engine();
    void work_do();
private:
    bool init();
    void videomode_set(bool);
    void main_loop();
    void frame_render();
    void input_process();
    void update(sf::Time);
    void sprite_draw(sf::Sprite*, const ScreenPosition&, float);
    void field_draw();
    void path_change(DeskPosition);
    bool cell_flip(DeskPosition);
    void sounds_play();
    void text_print(const ScreenPosition&, unsigned, const char*, bool = false);
};

////////////////////////////////////////////////////////////////////////////////

extern Engine the_engine;
