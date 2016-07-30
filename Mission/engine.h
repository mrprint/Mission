#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <deque>
#include <set>

// Вся информация о спрайте и его инициализации
struct SpriteInfo {
    const char *source;
    sf::Vector2f spot;
    sf::Texture texture;
    sf::Sprite sprite;
    SpriteInfo(const char *_source, float spot_x = 0.0f, float spot_y = 0.0f)
        : source(_source), spot(spot_x, spot_y) {}
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

class Engine
{
    enum ControlState {
        csEXIT, 
        csLMBUTTON,
        csRBUTTON
    };
    
    sf::RenderWindow* window;
    sf::Vector2i videoSize;
    sf::Font font;
    std::vector<SpriteInfo> sprites;
    std::vector<SoundInfo> sounds;
    std::set<ControlState> controls;
    Orchestre played_sounds;
    float banner_timeout;
public:
    Engine();
    ~Engine();
    void work_do();
private:
    bool init();
    void main_loop();
    void frame_render();
    void input_process();
    void update(sf::Time);
    void sprite_draw(sf::Sprite*, float, float, float);
    void tile_draw(sf::Sprite*, int, int, float);
    void field_draw();
    void path_change(int, int);
    bool cell_flip(int, int);
    void sounds_play();
    void text_print(float, float, unsigned, const char*, bool=false);
};

