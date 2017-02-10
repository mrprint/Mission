#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "world.hpp"
#include "spaces.hpp"

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
        csRMBUTTON,
        _csEND
    };

    std::unique_ptr<sf::RenderWindow> window;
    sf::Font font;
    std::bitset<_csEND> controls;
    ScreenPosition mouse_p;
    Orchestre played_sounds;
    float banner_timeout;
    bool windowed;
public:
    DrawingSizes sizes;

    Engine();
    void work_do();
private:
    bool init();
    void videomode_set(bool);
    void main_loop();
    void frame_render();
    void input_process();
    void update(sf::Time);
    void sprite_draw(sf::Sprite&, const ScreenPosition&, float);
    void field_draw();
    void path_change(DeskPosition);
    bool cell_flip(DeskPosition);
    void sounds_play();
    void text_print(const ScreenPosition&, unsigned, const std::string&, bool = false);
};

////////////////////////////////////////////////////////////////////////////////

extern Engine the_engine;
