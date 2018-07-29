#include "settings.hpp"
#include <limits>
#include <cmath>
#include <string>
#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "engine.hpp"
#include "spaces.hpp"
#include "world.hpp"
#include "coworker.hpp"
#include "assets.hpp"
#include "mathapp.hpp"

using namespace std;

using SpacePosition = tool::SpacePosition;
using DeskPosition = tool::DeskPosition;
using ScreenPosition = tool::ScreenPosition;

constexpr static auto TEXT_COLOR = 0xFF0010FF;
constexpr static auto TEXT_CHR_SIZE = 24;
constexpr static auto RES_DIR = "resources/";

enum TextureIndexes {
    txtBKG = 0, txtSPR, _txtEND
};
enum SpriteIndexes {
    sprBKG = 0, sprTILE, sprLWALL, sprRWALL, sprLBATT, sprRBATT, sprFBALL,
    sprEXIT, sprLGUARD, sprRGUARD, sprCHAR, sprCHART, _sprEND
};
enum SoundIndexes {
    sndSHOT = 0, sndHIT, sndLUP, _sndEND
};

// Юнит с рассчитанным расположением на экране
struct UnitOnScreen {
    ScreenPosition pos;
    Unit *unit;
    bool operator<(const UnitOnScreen &u) const { return (pos.y < u.pos.y); }
};

using ScreenPositions = vector<UnitOnScreen>; // Список расположения юнитов

Engine the_engine; // Экземпляр движка

////////////////////////////////////////////////////////////////////////////////
Engine::Engine() :
    window(make_unique<sf::RenderWindow>()),
    banner_timeout(0.0f),
    windowed(true),
    sizes()
{
    
}

bool Engine::init()
{
    videomode_set(windowed);
    if (!(window && window->isOpen()))
        return false;
    if (!assets_init())
        return false;
    if (!font.loadFromFile(string(RES_DIR) + "Orbitron Medium.ttf"))
        return false;

    return true;
}

void Engine::videomode_set(bool _windowed)
{
    sf::VideoMode mode;
    if (_windowed)
    {
        mode = sf::VideoMode(SCREEN_W, SCREEN_H);
        window->create(mode, TITLE, sf::Style::Titlebar | sf::Style::Close);
        window->setFramerateLimit(60);
    } else
    {
        mode = sf::VideoMode::getDesktopMode();
        window->create(mode, TITLE, sf::Style::Fullscreen);
        window->setVerticalSyncEnabled(true);
#if defined(SFML_SYSTEM_ANDROID) || defined(SFML_SYSTEM_LINUX) || defined(SFML_SYSTEM_FREEBSD)
#if !(SFML_VERSION_MAJOR > 2 || SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR > 4 || SFML_VERSION_MAJOR == 2 && SFML_VERSION_MINOR == 4 && SFML_VERSION_PATCH >= 1)
        // Обход SFML's bug #921
        window->setView(sf::View(
            sf::FloatRect(0.0f, 0.0f, static_cast<tool::fpoint_fast>(mode.width), static_cast<tool::fpoint_fast>(mode.height))
        ));
#endif
#endif
    }
    windowed = _windowed;
    sizes.screen_w = mode.width;
    sizes.screen_h = mode.height;

    auto tile_w = (sizes.screen_w - (_LC_OFST * 2)) / WORLD_DIM - ((sizes.screen_w - (_LC_OFST * 2)) / WORLD_DIM) % 2;
    auto tile_h = tile_w / 2;
    sizes.room_w = tile_w * WORLD_DIM;
    sizes.room_h = tile_h * WORLD_DIM;
    sizes.lc_ofst = (sizes.screen_w - sizes.room_w) / 2;
    sizes.tc_ofst = (sizes.screen_h - sizes.room_h) / 2;
    sizes.spr_scale = tile_w / SPR_SIZE;
    sizes.bkg_scale = ((sizes.screen_w > sizes.screen_h) ? sizes.screen_w : sizes.screen_h) / BKG_SIZE;
}

void Engine::frame_render()
{
    if (!window->isOpen())
        return;
    sprite_draw(the_sprites[sprBKG].sprite, ScreenPosition(sizes.screen_w, sizes.screen_h) / 2.0f, sizes.bkg_scale);

    field_draw();
    // Отображаем игровую информацию
    text_print(
        ScreenPosition(static_cast<tool::fpoint_fast>(sizes.lc_ofst)),
        TEXT_COLOR,
        string("Level ") + to_string(the_world.level + 1)
    );
    switch (the_world.state)
    {
    case gsLOSS:
        text_print(
            ScreenPosition(sizes.screen_w, sizes.screen_h) / 2.0f,
            TEXT_COLOR,
            "YOU LOSS!",
            true
        ); // Запомнилось из какой-то древней игрушки
        break;
    case gsWIN:
        text_print(
            ScreenPosition(sizes.screen_w, sizes.screen_h) / 2.0f,
            TEXT_COLOR,
            "LEVEL UP!",
            true
        );
        break;
    }
    window->display();
}

// Обрабатываем только свой ввод
void Engine::input_process()
{
    sf::Event evt;
    bool do_close = false;
    bool chmode = false;

    if (!window->isOpen())
        return;
    while (window->pollEvent(evt))
    {
        switch (evt.type)
        {
        case sf::Event::Closed:
            do_close = true;
            break;
        case sf::Event::KeyPressed:
            switch (evt.key.code)
            {
            case sf::Keyboard::Escape:
                do_close = true;
                break;
            case sf::Keyboard::F11:
                chmode = true;
                break;
            }
            break;
        case sf::Event::MouseButtonPressed:
            mouse_p = ScreenPosition(evt.mouseButton.x, evt.mouseButton.y);
            switch (evt.mouseButton.button)
            {
            case sf::Mouse::Left:
                controls.set(csLMBUTTON);
                break;
            case sf::Mouse::Right:
                controls.set(csRMBUTTON);
                break;
            }
            break;
        case sf::Event::MouseButtonReleased:
            mouse_p = ScreenPosition(evt.mouseButton.x, evt.mouseButton.y);
            switch (evt.mouseButton.button)
            {
            case sf::Mouse::Left:
                controls.reset(csLMBUTTON);
                break;
            case sf::Mouse::Right:
                controls.reset(csRMBUTTON);
                break;
            }
            break;
        }
    }
    if (chmode)
        videomode_set(!windowed);
    if (do_close)
        window->close();
}

void Engine::update(sf::Time tdelta)
{
    // Для антифликинга
    static bool lb_down = false;
    static bool rb_down = false;

    if (!window->isOpen())
        return;
    auto dt = tdelta.asSeconds();
    if (dt > MAX_TIME_FRACT)
        dt = MAX_TIME_FRACT; // Замедляем время, если машина не успевает считать

    if (the_world.state != gsINPROGRESS)
    {
        // Обработка таймаута вывода баннеров выигрыша/поражения
        if (banner_timeout > 0.0f)
        {
            banner_timeout -= dt;
            if (banner_timeout <= 0.0f)
            {
                // Снимаем баннер и настраиваем уровень
                the_world.state = gsINPROGRESS;
                the_world.setup();
            }
        } else
        {
            // Показываем баннер и выполняем базовые настройки при смене состояния
            banner_timeout = BANNER_TOUT;
            switch (the_world.state)
            {
            case gsLOSS:
                the_world.sounds.push_back(seHIT);
                the_world.level = 0;
                break;
            case gsWIN:
                the_world.sounds.push_back(seLVLUP);
                ++the_world.level;
                break;
            }
        }
    } else
    {
        the_world.state_check(); // Оцениваем состояние игры
    }

    if (controls.test(csLMBUTTON) && the_world.state == gsINPROGRESS)
    {
        if (!the_world.character->path_requested && the_coworker.flags_get(Coworker::cwREADY) && !lb_down)
        {
            // Будем идти в указанную позицию
            path_change(DeskPosition(mouse_p));
            lb_down = true;
        }
    } else
        lb_down = false;
    if (controls.test(csRMBUTTON) && the_world.state == gsINPROGRESS)
    {

        if (!the_world.character->path_requested && the_coworker.flags_get(Coworker::cwREADY) && !rb_down)
        {
            // Пытаемся изменить состояние ячейки "свободна"/"препятствие"
            if (cell_flip(DeskPosition(mouse_p)))
            {
                // При необходимости обсчитываем изменения пути
                if (the_world.character->way.path.size() > 0)
                    the_world.character->way_new_request(the_world.character->way.target);
            }
            rb_down = true;
        }
    } else
        rb_down = false;
    if (the_world.character->path_requested && the_coworker.flags_get(Coworker::cwREADY))
    {
        the_world.character->path_requested = false;
        the_world.character->way_new_process();
    }
    the_world.move_do(dt); // Рассчитываем изменения
    sounds_play(); // Воспроизводим звуки
}

void Engine::main_loop()
{
    sf::Clock clock;
    while (window->isOpen())
    {
        input_process();
        update(clock.restart());
        frame_render();
    }
}

void Engine::work_do()
{
    if (!init())
        return;

    main_loop();
}

void Engine::sprite_draw(sf::Sprite &spr, const ScreenPosition &pos, tool::fpoint_fast scale)
{
    spr.setPosition(sf::Vector2f(pos));
    spr.setScale(scale, scale);
    window->draw(spr);
}

// Отрисовка игрового поля
void Engine::field_draw()
{
    // Рисуем пол
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
        {
            if (!the_world.field(x, y).attribs.test(Cell::atrOBSTACLE))
                sprite_draw(the_sprites[sprTILE].sprite, ScreenPosition(DeskPosition(x, y)), sizes.spr_scale);
            if (the_world.field(x, y).attribs.test(Cell::atrEXIT))
                sprite_draw(the_sprites[sprEXIT].sprite, ScreenPosition(DeskPosition(x, y)), sizes.spr_scale);
        };
    // Рисуем стены
    for (int i = 0; i < WORLD_DIM; i++)
    {
        sprite_draw(the_sprites[sprRWALL].sprite, ScreenPosition(DeskPosition(i, 0)), sizes.spr_scale);
        sprite_draw(the_sprites[sprLWALL].sprite, ScreenPosition(DeskPosition(0, i)), sizes.spr_scale);
    }
    // Рисуем пушки
    for (auto &setting : the_world.artillery.setting)
    {
        if (abs(setting.speed.x) < numeric_limits<tool::fpoint_fast>::epsilon())
            sprite_draw(the_sprites[sprRBATT].sprite, ScreenPosition(setting.position), sizes.spr_scale);
        else
            sprite_draw(the_sprites[sprLBATT].sprite, ScreenPosition(setting.position), sizes.spr_scale);
    }
    // Заполняем список юнитов с их экранными координатами
    ScreenPositions positions;
    for (auto &alive : the_world.alives)
    {
        positions.emplace_back(UnitOnScreen{ ScreenPosition(alive.position) , &alive });
    }
    sort(positions.begin(), positions.end()); // Сортируем по экранному y
    // Рисуем юниты от дальних к ближним
    for (auto &spos : positions)
    {
        switch (spos.unit->id()) {
        case Unit::utCharacter:
            if (the_world.character->path_requested)
                sprite_draw(the_sprites[sprCHART].sprite, spos.pos, sizes.spr_scale);
            else
                sprite_draw(the_sprites[sprCHAR].sprite, spos.pos, sizes.spr_scale);
            break;
        case Unit::utFireball:
            sprite_draw(the_sprites[sprFBALL].sprite, spos.pos, sizes.spr_scale * 0.5f);
            break;
        case Unit::utGuard:
            sprite_draw(the_sprites[spos.unit->speed.x >= 0.0f ? sprRGUARD : sprLGUARD].sprite, spos.pos, sizes.spr_scale);
        }
    }
}

// Изменение состояния указанной мышкой ячейки
bool Engine::cell_flip(DeskPosition md)
{
    auto dp = DeskPosition(the_world.character->position);
    if (md.x < 0 || md.x >= WORLD_DIM || md.y < 0 || md.y >= WORLD_DIM || (md.x == dp.x && md.y == dp.y))
        return false;
    if (the_world.field[md].attribs.test(Cell::atrOBSTACLE))
        the_world.field[md].attribs.reset(Cell::atrOBSTACLE);
    else
        the_world.field[md].attribs.set(Cell::atrOBSTACLE);
    return true;
}

// Ищем новый путь
void Engine::path_change(DeskPosition md)
{
    if (md.x < 0 || md.x >= WORLD_DIM || md.y < 0 || md.y >= WORLD_DIM)
        return;
    the_world.character->way_new_request(md);
}

void Engine::sounds_play()
{
    played_sounds.update();
    while (!the_world.sounds.empty())
    {
        switch (the_world.sounds.front())
        {
        case seSHOT:
            played_sounds.play(the_sounds[sndSHOT].buffer);
            break;
        case seHIT:
            played_sounds.play(the_sounds[sndHIT].buffer);
            break;
        case seLVLUP:
            played_sounds.play(the_sounds[sndLUP].buffer);
            break;
        }
        the_world.sounds.pop_front();
    }
}

void Engine::text_print(const ScreenPosition &pos, unsigned color, const string& str, bool centered)
{
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(TEXT_CHR_SIZE);
    text.setColor(sf::Color(color));
    text.setString(str);
    if (centered)
    {
        auto bounds = text.getLocalBounds();
        text.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    } else
        text.setOrigin(0.0f, 0.0f);
    text.setPosition(sf::Vector2f(pos));
    window->draw(text);
}

////////////////////////////////////////////////////////////////////////////////
void Orchestre::play(const sf::SoundBuffer& buffer)
{
    if (sounds.size() >= 256) // Максимально допустимое количество звуков в SFML
        return;
    sounds.emplace_back(sf::Sound{ buffer });
    sounds.back().play();
}

void Orchestre::update()
{
    while (!sounds.empty() && sounds.front().getStatus() == sf::SoundSource::Stopped)
        sounds.pop_front();
}

////////////////////////////////////////////////////////////////////////////////
bool TextureInfo::init()
{
    if (!texture.loadFromFile(string(RES_DIR) + source))
        return false;
    texture.setSmooth(true);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool SpriteInfo::init()
{
    sprite.setTexture(texture->texture);
    sprite.setTextureRect(txrect);
    sprite.setOrigin(spot.x - offset.x, spot.y - offset.y);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

AnimationSequence::AnimationSequence(tool::fpoint_fast _length) : length(_length)
{
    node = sequence.front().time == 0.0 ? sequence.begin() : --sequence.end();
}

void AnimationSequence::advance(tool::fpoint_fast tdelta)
{
    auto t = time + tdelta;
    time = t > length ? t - length : t;
    for (
        auto noden = node;
        node->time * length > time && noden->time * length > time || noden->time * length < time;
        (node = noden), noden = (++noden != sequence.end()) ? noden : sequence.begin()
        );
}

SpriteInfo* AnimationSequence::sprite_get() const
{
    return node->pic;
}

////////////////////////////////////////////////////////////////////////////////
bool SoundInfo::init()
{
    if (!buffer.loadFromFile(string(RES_DIR) + source))
        return false;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Copyright(c) 2017 https://github.com/mrprint
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files(the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell 
// copies of the Software, and to permit persons to whom the Software is 
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
// SOFTWARE.
