#include "settings.hpp"
#include <limits>
#include <stdio.h>
#include <math.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "engine.hpp"
#include "spaces.hpp"
#include "world.hpp"
#include "main.hpp"


#if _MSC_VER && (_MSC_VER < 1800) && !__INTEL_COMPILER
#define snprintf(buff, sz, str, ...) sprintf(buff, str, __VA_ARGS__)
#endif

static const float BKG_SIZE = 1024.0f;
static const float SPR_SIZE = 128.0f;
static const float TILE_HOT = SPR_SIZE * 0.75f;
static const unsigned TEXT_COLOR = 0xFF0010FF;
static const int TEXT_CHR_SIZE = 24;
static const char *RES_DIR = "resources/";

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
    bool operator<(const UnitOnScreen &u) { return (pos.y < u.pos.y); }
};

typedef std::vector<UnitOnScreen> ScreenPositions; // Список расположения юнитов

Engine the_engine; // Экземпляр движка

////////////////////////////////////////////////////////////////////////////////
Engine::Engine()
{
    window = new sf::RenderWindow();
    banner_timeout = 0.0f;
    textures.push_back(TextureInfo("skybkg.png"));
    textures.push_back(TextureInfo("sprites.png"));
    sprites.push_back(SpriteInfo(textures[txtBKG], 0, 0, 1024, 1024, 0, 0, BKG_SIZE / 2, BKG_SIZE / 2));
    sprites.push_back(SpriteInfo(textures[txtSPR], 130, 0, 128, 64, 0, 64, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 380, 77, 63, 96, 0, 0, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 445, 77, 65, 97, 63, 0, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 232, 66, 24, 47, 20, 25, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 468, 0, 23, 50, 82, 22, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 0, 0, 128, 128, 0, 0, SPR_SIZE / 2, SPR_SIZE));
    sprites.push_back(SpriteInfo(textures[txtSPR], 0, 130, 66, 35, 29, 77, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 130, 66, 100, 72, 11, 47, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 364, 0, 102, 75, 18, 44, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 290, 109, 88, 91, 19, 16, SPR_SIZE / 2, TILE_HOT));
    sprites.push_back(SpriteInfo(textures[txtSPR], 260, 0, 102, 107, 5, 0, SPR_SIZE / 2, TILE_HOT));
    sounds.push_back("shot.wav");
    sounds.push_back("hit.wav");
    sounds.push_back("gong.wav");
    windowed = true;
}

Engine::~Engine()
{
    if (window)
        delete window;
}

bool Engine::init()
{
    videomode_set(windowed);
    if (!(window && window->isOpen()))
        return false;

    for (int i = txtBKG; i < _txtEND; ++i)
    {
        if (!textures[i].init())
            return false;
    }
    for (int i = sprBKG; i < _sprEND; ++i)
    {
        if (!sprites[i].init())
            return false;
    }
    for (int i = sndSHOT; i < _sndEND; ++i)
    {
        if (!sounds[i].init())
            return false;
    }
    if (!font.loadFromFile(std::string(RES_DIR) + "Orbitron Medium.ttf"))
        return false;

    return true;
}

void Engine::videomode_set(bool _windowed)
{
    sf::VideoMode mode;
    unsigned style;
    if (_windowed)
    {
        mode = sf::VideoMode(SCREEN_W, SCREEN_H);
        style = sf::Style::Titlebar | sf::Style::Close;
        window->create(mode, TITLE, style);
        window->setFramerateLimit(60);
    }
    else
    {
        mode = sf::VideoMode::getDesktopMode();
        style = sf::Style::Fullscreen;
        window->create(mode, TITLE, style);
        window->setVerticalSyncEnabled(true);
        // Обход SFML's bug #921
        window->setView(sf::View(
            sf::FloatRect(0.0f, 0.0f, static_cast<float>(mode.width), static_cast<float>(mode.height))
        ));
    }
    windowed = _windowed;
    sizes.screen_w = mode.width;
    sizes.screen_h = mode.height;

    int tile_w = (sizes.screen_w - (_LC_OFST * 2)) / WORLD_DIM - ((sizes.screen_w - (_LC_OFST * 2)) / WORLD_DIM) % 2;
    int tile_h = tile_w / 2;
    int tile_hw = tile_w / 2;
    int tile_hh = tile_h / 2 + (tile_h / 2) % 2;
    sizes.room_w = tile_w * WORLD_DIM;
    sizes.room_h = tile_h * WORLD_DIM;
    sizes.lc_ofst = (sizes.screen_w - sizes.room_w) / 2;
    sizes.tc_ofst = (sizes.screen_h - sizes.room_h) / 2;
    sizes.spr_scale = tile_w / SPR_SIZE;
    sizes.bkg_scale = ((sizes.screen_w > sizes.screen_h) ? sizes.screen_w : sizes.screen_h) / BKG_SIZE;
}

void Engine::frame_render()
{
    char buffer[32];

    if (!window->isOpen())
        return;
    sprite_draw(&sprites[sprBKG].sprite, ScreenPosition(sizes.screen_w, sizes.screen_h) / 2.0f, sizes.bkg_scale);

    field_draw();
    // Отображаем игровую информацию
    snprintf(buffer, sizeof(buffer), "Level %d", the_world.level + 1);
    text_print(
        ScreenPosition(static_cast<float>(sizes.lc_ofst)),
        TEXT_COLOR,
        reinterpret_cast<char*>(&buffer)
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
    float dt = tdelta.asSeconds();
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
        }
        else
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
    }
    else
    {
        the_world.state_check(); // Оцениваем состояние игры
    }

    if (controls.test(csLMBUTTON) && the_world.state == gsINPROGRESS)
    {
        if (!the_world.character->path_requested && the_coworker.flags_get(Coworker::cwREADY) && !lb_down)
        {
            // Будем идти в указанную позицию
            path_change(mouse_p);
            lb_down = true;
        }
    }
    else
        lb_down = false;
    if (controls.test(csRMBUTTON) && the_world.state == gsINPROGRESS)
    {

        if (!the_world.character->path_requested && the_coworker.flags_get(Coworker::cwREADY) && !rb_down)
        {
            // Пытаемся изменить состояние ячейки "свободна"/"препятствие"
            if (cell_flip(mouse_p))
            {
                // При необходимости обсчитываем изменения пути
                if (the_world.character->way.path.size() > 0)
                    the_world.character->way_new_request(the_world.character->way.target);
            }
            rb_down = true;
        }
    }
    else
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

void Engine::sprite_draw(sf::Sprite *spr, const ScreenPosition &pos, float scale)
{
    spr->setPosition(pos);
    spr->setScale(scale, scale);
    window->draw(*spr);
}

// Отрисовка игрового поля
void Engine::field_draw()
{
    // Рисуем пол
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
        {
            if (!the_world.field(x, y).attribs.test(Cell::atrOBSTACLE))
                sprite_draw(&sprites[sprTILE].sprite, DeskPosition(x, y), sizes.spr_scale);
            if (the_world.field(x, y).attribs.test(Cell::atrEXIT))
                sprite_draw(&sprites[sprEXIT].sprite, DeskPosition(x, y), sizes.spr_scale);
        };
    // Рисуем стены
    for (int i = 0; i < WORLD_DIM; i++)
    {
        sprite_draw(&sprites[sprRWALL].sprite, DeskPosition(i, 0), sizes.spr_scale);
        sprite_draw(&sprites[sprLWALL].sprite, DeskPosition(0, i), sizes.spr_scale);
    }
    // Рисуем пушки
    for (Artillery::Settings::iterator it = the_world.artillery.setting.begin(); it != the_world.artillery.setting.end(); ++it)
    {
        if (fabs(it->speed.x) < std::numeric_limits<float>::epsilon())
            sprite_draw(&sprites[sprRBATT].sprite, it->position, sizes.spr_scale);
        else
            sprite_draw(&sprites[sprLBATT].sprite, it->position, sizes.spr_scale);
    }
    // Заполняем список юнитов с их экранными координатами
    ScreenPositions positions;
    UnitOnScreen uos;
    for (UnitsList::iterator it = the_world.alives.begin(); it != the_world.alives.end(); ++it)
    {
        uos.pos = it->position;
        uos.unit = &*it;
        positions.push_back(uos);
    }
    std::sort(positions.begin(), positions.end()); // Сортируем по экранному y
    // Рисуем юниты от дальних к ближним
    for (ScreenPositions::iterator it = positions.begin(); it != positions.end(); ++it)
    {
        switch (it->unit->id()) {
        case Unit::utCharacter:
            if (the_world.character->path_requested)
                sprite_draw(&sprites[sprCHART].sprite, it->pos, sizes.spr_scale);
            else
                sprite_draw(&sprites[sprCHAR].sprite, it->pos, sizes.spr_scale);
            break;
        case Unit::utFireball:
            sprite_draw(&sprites[sprFBALL].sprite, it->pos, sizes.spr_scale * 0.5f);
            break;
        case Unit::utGuard:
            sprite_draw(&sprites[it->unit->speed.x >= 0.0f ? sprRGUARD : sprLGUARD].sprite, it->pos, sizes.spr_scale);
        }
    }
}

// Изменение состояния указанной мышкой ячейки
bool Engine::cell_flip(DeskPosition md)
{
    DeskPosition dp = the_world.character->position;
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
            played_sounds.play(sounds[sndSHOT].buffer);
            break;
        case seHIT:
            played_sounds.play(sounds[sndHIT].buffer);
            break;
        case seLVLUP:
            played_sounds.play(sounds[sndLUP].buffer);
            break;
        }
        the_world.sounds.pop_front();
    }
}

void Engine::text_print(const ScreenPosition &pos, unsigned color, const char* str, bool centered)
{
    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(TEXT_CHR_SIZE);
    text.setColor(sf::Color(color));
    text.setString(str);
    if (centered)
    {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    }
    else
        text.setOrigin(0.0f, 0.0f);
    text.setPosition(pos);
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
bool TextureInfo::init()
{
    if (!texture.loadFromFile(std::string(RES_DIR) + source))
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

AnimationSequence::AnimationSequence(float _length) : length(_length)
{
    node = sequence.front().time == 0.0 ? sequence.begin() : --sequence.end();
}

void AnimationSequence::advance(float tdelta)
{
    Sequence::iterator noden;
    float t = time + tdelta;
    time = t > length ? t - length : t;
    for (
        noden = node;
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
    if (!buffer.loadFromFile(std::string(RES_DIR) + source))
        return false;
    return true;
}
