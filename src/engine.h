#pragma once

#include <vector>
#include <deque>
#include <set>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

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

class Engine
{
    enum ControlState {
        csLMBUTTON,
        csRMBUTTON
    };

    sf::RenderWindow* window;
    sf::Vector2i videoSize;
    sf::Font font;
    std::vector<TextureInfo> textures;
    std::vector<SpriteInfo> sprites;
    std::vector<SoundInfo> sounds;
    std::set<ControlState> controls;
    sf::Vector2i mouse_p;
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
    void text_print(float, float, unsigned, const char*, bool = false);
};

