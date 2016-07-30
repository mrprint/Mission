#include <stdio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "engine.h"
#include "settings.h"
#include "world.h"
#include "main.h"

static const float BKG_SIZE = 1024.0f;
static const float SPR_SIZE = 128.0f;
static const float TILE_HOT = SPR_SIZE * 0.75f;
static const float SPR_SCALE = TILE_W / SPR_SIZE;
static const float SK45 = 0.7071067812f;
static const float HYP2 = 2.828427125f;
static const unsigned TEXT_COLOR = 0xFF0010FF;
static const char *RES_DIR = "resources\\";

enum SpriteIndexes {
    bgspr = 0, tlspr, lwspr, rwspr, lbspr, rbspr, fbspr, epspr, lgspr, rgspr, chspr, chtspr, _endspr
};
enum SoundIndexes {
    shothe = 0, hithe, luphe, _endhe
};

// Юнит с рассчитанным расположением на экране
struct ScreenPos {
    float x, y;
    Unit *unit;
    bool operator<(const ScreenPos &sp) { return (y < sp.y); }
};

typedef std::vector<ScreenPos> ScreenPositions; // Список расположения юнитов

////////////////////////////////////////////////////////////////////////////////
// Трансформация пространственных координат в экранные
static inline void space_to_screen(float *x, float *y, const SpacePosition &position)
{
    float xsk = position.x * SK45;
    float ysk = position.y * SK45;
    float tx = xsk - ysk;
    float ty = xsk + ysk;
    *x = LC_OFST + (HYP2 / 2.0f + tx) * ROOM_W / HYP2;
    *y = TC_OFST + (HYP2 / 2.0f + ty) * ROOM_H / HYP2;
}

// Трансформация экранных координат в пространственные
static inline void screen_to_space(SpacePosition *position, float x, float y)
{
    float k = 2.0f * ROOM_H * ROOM_W * SK45;
    position->x = -(-x * HYP2 * ROOM_H + HYP2 * LC_OFST * ROOM_H + HYP2 * ROOM_W * (TC_OFST + ROOM_H - y)) / k;
    position->y = -(x * HYP2 * ROOM_H - HYP2 * LC_OFST * ROOM_H + HYP2 * ROOM_W * (TC_OFST - y)) / k;
}

// Выбор ячейки по экранным координатам
static inline void screen_to_field(int *fx, int *fy, float sx, float sy)
{
    SpacePosition position;
    screen_to_space(&position, sx, sy);
    Field::pos_to_cell(fx, fy, position);
}

////////////////////////////////////////////////////////////////////////////////
Engine::Engine()
{
    window = NULL;
    videoSize = sf::Vector2i(SCREEN_W, SCREEN_H);
    banner_timeout = 0.0f;
    sprites = {
        SpriteInfo("skybkg.png"),
        SpriteInfo("tile.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("lwall.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("rwall.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("lbatt.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("rbatt.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("fireball.png" , SPR_SIZE / 2, SPR_SIZE),
        SpriteInfo("exit.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("lguard.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("rguard.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("character.png" , SPR_SIZE / 2, TILE_HOT),
        SpriteInfo("character_t.png" , SPR_SIZE / 2, TILE_HOT)
    };
    sounds = { "shot.wav", "hit.wav", "gong.wav" };
}

Engine::~Engine()
{
    if (window)
        delete window;
}

bool Engine::init()
{
    window = new sf::RenderWindow(
        sf::VideoMode(videoSize.x, videoSize.y),
        "The Mission",
        sf::Style::Titlebar | sf::Style::Close
    );
    if (!window)
        return false;

    for (int i = bgspr; i < _endspr; ++i)
    {
        if (!sprites[i].init())
            return false;
    }
    sf::Sprite *background = &sprites[bgspr].sprite;
    background->setPosition(0.0f, 0.0f);
    sf::FloatRect brect = background->getLocalBounds();
    background->setScale(SCREEN_W / brect.width, SCREEN_H / brect.height);
    for (int i = shothe; i < _endhe; ++i)
    {
        if (!sounds[i].init())
            return false;
    }
    if (!font.loadFromFile(std::string(RES_DIR) + "Orbitron Medium.ttf"))
        return false;

    window->setVerticalSyncEnabled(true);
    return true;
}

void Engine::frame_render()
{
    char buffer[32];

    if (!window->isOpen())
        return;
    window->draw(sprites[bgspr].sprite);
    field_draw();
    // Отображаем игровую информацию
    snprintf(buffer, sizeof(buffer), "Level %d", level + 1);
    text_print(float(LC_OFST), float(LC_OFST), TEXT_COLOR, reinterpret_cast<char*>(&buffer));
    switch (the_state)
    {
    case gsLOSS:
        text_print(float(SCREEN_W / 2), float(SCREEN_H / 2), TEXT_COLOR, "YOU LOSS!", true); // Запомнилось из какой-то древней игрушки
        break;
    case gsWIN:
        text_print(float(SCREEN_W / 2), float(SCREEN_H / 2), TEXT_COLOR, "LEVEL UP!", true);
        break;
    }
    window->display();
}

void Engine::input_process()
{
    sf::Event evt;
    if (!window->isOpen())
        return;
    while (window->pollEvent(evt))
    {
        switch (evt.type) 
        {
        case sf::Event::Closed:
            window->close();
            break;
        }
    }
}

void Engine::update(sf::Time tdelta)
{
    sf::Vector2i m_pos;
    bool closefl = false;

    // Для антифликинга
    static bool lb_down = false;
    static bool rb_down = false;

    if (!window->isOpen())
        return;
    float dt = tdelta.asSeconds();
    if (dt > MAX_TIME_FRACT)
        dt = MAX_TIME_FRACT; // Замедляем время, если машина не успевает считать

    if (the_state != gsINPROGRESS)
    {
        // Обработка таймаута вывода баннеров выигрыша/поражения
        if (banner_timeout > 0.0f)
        {
            banner_timeout -= dt;
            if (banner_timeout <= 0.0f)
            {
                // Снимаем баннер и настраиваем уровень
                the_state = gsINPROGRESS;
                worldSetup();
            }
        }
        else
        {
            // Показываем баннер и выполняем базовые настройки при по смене состояния
            banner_timeout = BANNER_TOUT;
            switch (the_state)
            {
            case gsLOSS:
                the_sounds.push_back(seHIT);
                level = 0;
                break;
            case gsWIN:
                the_sounds.push_back(seLVLUP);
                ++level;
                break;
            }
        }
    }
    else
    {
        stateCheck(); // Оцениваем состояние игры
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
        closefl = true;
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && the_state == gsINPROGRESS)
    {
        if (!the_character->path_requested && path_ready_check() && !lb_down)
        {
            // Будем идти в указанную позицию
            m_pos = sf::Mouse::getPosition(*window);
            path_change(m_pos.x, m_pos.y);
            lb_down = true;
        }
    }
    else
        lb_down = false;
    if (sf::Mouse::isButtonPressed(sf::Mouse::Right) && the_state == gsINPROGRESS)
    {

        if (!the_character->path_requested && path_ready_check() && !rb_down)
        {
            // Пытаемся изменить состояние ячейки "свободна"/"препятствие"
            m_pos = sf::Mouse::getPosition(*window);
            if (cell_flip(m_pos.x, m_pos.y))
            {
                // При необходимости обсчитываем изменения пути
                if (the_character->way.path.size() > 0)
                    the_character->way_new_request(the_character->way.target.x, the_character->way.target.y);
            }
            rb_down = true;
        }
    }
    else
        rb_down = false;
    if (the_character->path_requested && path_ready_check())
    {
        the_character->path_requested = false;
        the_character->way_new_process();
    }
    moveDo(dt); // Рассчитываем изменения
    sounds_play(); // Воспроизводим звуки
    if (closefl)
        window->close();
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

void Engine::sprite_draw(sf::Sprite *spr, float x, float y, float scale)
{
    spr->setPosition(x, y);
    spr->setScale(scale, scale);
    window->draw(*spr);
}

// Отрисовка тайла в позиции ячейки
void Engine::tile_draw(sf::Sprite *spr, int cx, int cy, float scale)
{
    SpacePosition position;
    float x, y;
    Field::cell_to_pos(&position, cx, cy);
    space_to_screen(&x, &y, position);
    sprite_draw(spr, x, y, scale);
}

// Отрисовка игрового поля
void Engine::field_draw()
{
    // Рисуем пол
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
        {
            if (the_field.cells[y][x].attribs.count(Cell::atrOBSTACLE) == 0)
                tile_draw(&sprites[tlspr].sprite, x, y, SPR_SCALE);
            if (the_field.cells[y][x].attribs.count(Cell::atrEXIT) > 0)
                tile_draw(&sprites[epspr].sprite, x, y, SPR_SCALE);
        };
    // Рисуем стены
    for (int i = 0; i < WORLD_DIM; i++)
        tile_draw(&sprites[rwspr].sprite, i, 0, SPR_SCALE);
    for (int i = 0; i < WORLD_DIM; i++)
        tile_draw(&sprites[lwspr].sprite, 0, i, SPR_SCALE);
    // Рисуем пушки
    for (Artillery::Settings::iterator it = the_artillery.setting.begin(); it != the_artillery.setting.end(); ++it)
    {
        if (abs(it->speed.x) < FLT_EPSILON)
            tile_draw(&sprites[rbspr].sprite, it->position.x, it->position.y, SPR_SCALE);
        else
            tile_draw(&sprites[lbspr].sprite, it->position.x, it->position.y, SPR_SCALE);
    }
    // Заполняем список юнитов с их экранными координатами
    ScreenPositions positions;
    ScreenPos upos;
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end(); ++it)
    {
        space_to_screen(&upos.x, &upos.y, (*it)->position);
        upos.unit = *it;
        positions.push_back(upos);
    }
    std::sort(positions.begin(), positions.end()); // Сортируем по экранному y
                                                   // Рисуем юниты от дальних к ближним
    for (ScreenPositions::iterator it = positions.begin(); it != positions.end(); ++it)
    {
        switch (it->unit->id()) {
        case Unit::utCharacter:
            if (the_character->path_requested)
                sprite_draw(&sprites[chtspr].sprite, it->x, it->y, SPR_SCALE);
            else
                sprite_draw(&sprites[chspr].sprite, it->x, it->y, SPR_SCALE);
            break;
        case Unit::utFireball:
            sprite_draw(&sprites[fbspr].sprite, it->x, it->y, SPR_SCALE * 0.5f);
            break;
        case Unit::utGuard:
            sprite_draw(&sprites[it->unit->speed.x >= 0.0f ? rgspr : lgspr].sprite, it->x, it->y, SPR_SCALE);
        }
    }
}

// Изменение состояния указанной мышкой ячейки
bool Engine::cell_flip(int mx, int my)
{
    int x, y, cx, cy;
    screen_to_field(&x, &y, static_cast<float>(mx), static_cast<float>(my));
    Field::pos_to_cell(&cx, &cy, the_character->position);
    if (x < 0 || x >= WORLD_DIM || y < 0 || y >= WORLD_DIM || (x == cx && y == cy))
        return false;
    if (the_field.cells[y][x].attribs.count(Cell::atrOBSTACLE) > 0)
        the_field.cells[y][x].attribs.erase(Cell::atrOBSTACLE);
    else
        the_field.cells[y][x].attribs.insert(Cell::atrOBSTACLE);
    return true;
}

// Ищем новый путь
void Engine::path_change(int mx, int my)
{
    int x, y;
    screen_to_field(&x, &y, static_cast<float>(mx), static_cast<float>(my));
    if (x < 0 || x >= WORLD_DIM || y < 0 || y >= WORLD_DIM)
        return;
    the_character->way_new_request(x, y);
}

void Engine::sounds_play()
{
    played_sounds.update();
    while (!the_sounds.empty())
    {
        switch (the_sounds.front())
        {
        case seSHOT:
            played_sounds.play(sounds[shothe].buffer);
            break;
        case seHIT:
            played_sounds.play(sounds[hithe].buffer);
            break;

        case seLVLUP:
            played_sounds.play(sounds[luphe].buffer);
            break;
        }
        the_sounds.pop_front();
    }
}

void Engine::text_print(float x, float y, unsigned color, const char* str, bool centered)
{
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(24);
    text.setColor(sf::Color(color));
    text.setString(str);
    if (centered)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    }
    else
        text.setOrigin(0.0f, 0.0f);
    text.setPosition(x, y);
    window->draw(text);
}

////////////////////////////////////////////////////////////////////////////////
void Orchestre::play(const sf::SoundBuffer& buffer)
{
    sf::Sound s_entity;
    if (sounds.size() >= 256) // Максимально допустимое количество звуков в SFML
        return;
    sounds.push_back(s_entity);
    sounds.back().setBuffer(buffer);
    sounds.back().play();
}

void Orchestre::update()
{
    while (!sounds.empty() && sounds.front().getStatus() == sf::SoundSource::Stopped)
        sounds.pop_front();
}

////////////////////////////////////////////////////////////////////////////////
bool SpriteInfo::init()
{
    if (!texture.loadFromFile(std::string(RES_DIR) + source))
        return false;
    texture.setSmooth(true);
    sprite.setTexture(texture);
    sprite.setOrigin(spot);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
bool SoundInfo::init()
{
    if (!buffer.loadFromFile(std::string(RES_DIR) + source))
        return false;
    return true;
}
