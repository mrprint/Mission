#include <math.h>
#include <random>
#include <vector>
#include <algorithm>
#include "settings.h"
#include "world.h"
#include "pathfinding.h"
#include "main.h"

unsigned level;
GameState the_state;
Field the_field;
UnitsList the_alives;
Artillery the_artillery;
Character *the_character;
SoundsQueue the_sounds;

std::default_random_engine rand_gen;
std::uniform_real_distribution<float> rand_distrib(0.0f, 1.0f);

// Мелкие вспомогательные функции
static inline bool rnd_choice(float possib)
{
    return rand_distrib(rand_gen) < possib;
}

static inline float deviation_apply(float val, float dev)
{
    return (2 * dev * rand_distrib(rand_gen) - dev + 1) * val;
}

template<class T>
static inline T round(T number)
{
    return number < T(0.0) ? ceil(number - T(0.5)) : floor(number + T(0.5));
}

static inline int complexity_apply(int val, float kc)
{
    return val + int(round(val * level * kc));
}

static inline float complexity_apply(float val, float kc)
{
    return val + val * level * kc;
}

////////////////////////////////////////////////////////////////////////////////
Cell::Cell(const Cell& cell)
{
    attribs = cell.attribs;
}

Cell::Cell(const Attributes& _attribs)
{
    attribs = _attribs;
}

////////////////////////////////////////////////////////////////////////////////
Artillery::Artillery(const Settings& _settings)
{
    setting = _settings;
}

////////////////////////////////////////////////////////////////////////////////
Field::Field(const Field& field)
{
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
            cells[y][x] = field.cells[y][x];
}

void Field::cell_to_pos(SpacePosition *position, int x, int y)
{
    position->x = to_space_dim(x);
    position->y = to_space_dim(y);
}

void Field::pos_to_cell(int *x, int *y, const SpacePosition &position)
{
    float k = WORLD_DIM  / 2.0f;
    *x = int(floor((position.x + 1.0f) * k));
    *y = int(floor((position.y + 1.0f) * k));
}

////////////////////////////////////////////////////////////////////////////////
Unit::Unit()
{
    size = U_SIZE;
    position.x = 0.0f;
    position.y = 0.0f;
    speed.x = 0.0f;
    speed.y = 0.0f;
}

Unit::Unit(const Unit& unit)
{
    size = unit.size;
    position = unit.position;
    speed = unit.speed;
}

bool Unit::is_collided(const Unit& unit) const
{
    float a = position.x - unit.position.x;
    float b = position.y - unit.position.y;
    return sqrt(a * a + b * b) < size + unit.size;
}

void Unit::move(float tdelta)
{
    position.x += speed.x * tdelta;
    position.y += speed.y * tdelta;
}

////////////////////////////////////////////////////////////////////////////////
Character::Character() : Unit()
{
    path_requested = false;
    way.path.clear(); // Стоит на месте
    way.target.x = 0;
    way.target.y = 0;
}

Character::Character(const Character& character) : Unit(character)
{
    path_requested = false;
    way = character.way;
}

void Character::move(float tdelta)
{
    Cell::Coordinates delta;
    int x, y;
    if (the_state == gsINPROGRESS)
        // Перемещаемся только во время игры
        Unit::move(tdelta);
    if (way.path.size() == 0)
        return;
    if (speed.x > 0.0f && position.x >= way.neigpos.x
        || speed.x < 0.0f && position.x <= way.neigpos.x
        || speed.y > 0.0f && position.y >= way.neigpos.y
        || speed.y < 0.0f && position.y <= way.neigpos.y)
    {
        // Этап завершен
        position = way.neigpos;
        if (way.stage >= way.path.size() - 1)
        {
            // Цель достигнута
            way.path.clear();
            way.stage = 0;
            Field::pos_to_cell(&x, &y, position);
            way.target.x = x;
            way.target.y = y;
        }
        else
        {
            // Следующий этап
            Field::pos_to_cell(&x, &y, position);
            ++way.stage;
            delta = way.path.at(way.path.size() - way.stage - 1);
            way.neighbour.x += delta.x;
            way.neighbour.y += delta.y;
            Field::cell_to_pos(&way.neigpos, way.neighbour.x, way.neighbour.y);
        }
        set_speed();
    }
}

void Character::set_speed()
{
    float dx, dy, adx, ady, a;
    if (way.path.size() == 0)
    {
        speed.x = 0.0f;
        speed.y = 0.0f;
        return;
    }
    dx = way.neigpos.x - position.x;
    dy = way.neigpos.y - position.y;
    adx = abs(dx);
    ady = abs(dy);
    if (adx < FLT_EPSILON && ady < FLT_EPSILON || way.path.size() == 0)
        return;
    if (adx > ady)
    {
        a = atan(ady / adx);
        speed.x = float(_copysign(CHAR_B_SPEED * cos(a), dx));
        speed.y = float(_copysign(CHAR_B_SPEED * sin(a), dy));
    }
    else
    {
        a = atan(adx / ady);
        speed.x = float(_copysign(CHAR_B_SPEED * sin(a), dx));
        speed.y = float(_copysign(CHAR_B_SPEED * cos(a), dy));
    }
}

// Запрос обсчета пути
void Character::way_new_request(int tx, int ty)
{
    int x, y;
    speed.x = 0.0f;
    speed.y = 0.0f;
    Field::pos_to_cell(&x, &y, position);
    way.target.x = tx;
    way.target.y = ty;
    path_find_request(the_field, x, y, tx, ty);
    path_requested = true;
}

// Обработка рассчитанного пути
void Character::way_new_process()
{
    Cell::Coordinates delta;
    int x, y;
    way.path = path_read();
    Field::pos_to_cell(&x, &y, position);
    if (way.path.size() > 0)
    {
        way.stage = 0;
        delta = way.path.at(way.path.size() - 1);
        way.neighbour.x = x + delta.x;
        way.neighbour.y = y + delta.y;
        Field::cell_to_pos(&way.neigpos, way.neighbour.x, way.neighbour.y);
    }
    set_speed();
}

////////////////////////////////////////////////////////////////////////////////
void Guard::move(float tdelta)
{
    int x, y;
    Unit::move(tdelta);
    Field::pos_to_cell(&x, &y, position);
    if (the_field.cells[y][x].attribs.count(Cell::atrGUARDBACKW) > 0)
        speed.x = -abs(speed.x);
    if (the_field.cells[y][x].attribs.count(Cell::atrGUARDFORW) > 0)
        speed.x = abs(speed.x);
}

////////////////////////////////////////////////////////////////////////////////
static void artillerySet(int x, int y, float spx, float spy, float delay)
{
    Artillery::Setting artst;
    if (rnd_choice(ART_B_POSSIB))
    {
        artst.position.x = x;
        artst.position.y = y;
        artst.speed.x = spx;
        artst.speed.y = spy;
        artst.delay = delay;
        artst.timeout = 0.0f;
        the_artillery.setting.push_back(artst);
    }
}

// Инициализация вселенной
void world_setup()
{
    int i;
    Unit *unit;

    lists_clear();

    // Размечаем поле
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
            the_field.cells[y][x].attribs.clear();
    the_field.cells[0][WORLD_DIM - 1].attribs.insert(Cell::atrEXIT); // Позиция выхода
    the_field.cells[2][0].attribs.insert(Cell::atrGUARDFORW); // Вешка направления движения охраны
    the_field.cells[2][WORLD_DIM - 1].attribs.insert(Cell::atrGUARDBACKW); // Вешка направления движения охраны
    // Главный герой
    the_alives.push_back(new Character());
    the_character = static_cast<Character*>(the_alives.back());
    Field::cell_to_pos(&the_character->position, 0, WORLD_DIM - 1);
    the_character->way.target.x = 0;
    the_character->way.target.y = WORLD_DIM - 1;
    the_character->set_speed();
    // Стража
    the_alives.push_back(new Guard());
    unit = the_alives.back();
    Field::cell_to_pos(&unit->position, 0, 2);
    unit->size = U_SIZE * 1.5f;
    unit->speed.x = GUARD_B_SPEED;
    unit->speed.y = 0.0f;
    // Артиллерия
    Artillery::Settings apositions(WORLD_DIM * 2 - 2);
    for (i = 0; i < WORLD_DIM - 1; i++)
    {
        apositions[i].position.x = i;
        apositions[i].position.y = 0;
        apositions[i].speed.x = 0.0f;
        apositions[i].speed.y = deviation_apply(ART_B_SPEED, ART_DEV);
        apositions[i].delay = deviation_apply(ART_B_DELAY, ART_DEV);
        apositions[i].timeout = 0.0f;
        apositions[WORLD_DIM - 1 + i].position.x = 0;
        apositions[WORLD_DIM - 1 + i].position.y = i;
        apositions[WORLD_DIM - 1 + i].speed.x = deviation_apply(complexity_apply(ART_B_SPEED, LEVEL_COMPL), ART_DEV);
        apositions[WORLD_DIM - 1 + i].speed.y = 0.0f;
        apositions[WORLD_DIM - 1 + i].delay = deviation_apply(ART_B_DELAY, ART_DEV);
        apositions[WORLD_DIM - 1 + i].timeout = 0.0f;
    }
    std::shuffle(apositions.begin(), apositions.end(), rand_gen);
    int acount = std::min(complexity_apply(ART_COUNT, LEVEL_COMPL), int(apositions.capacity()));
    for (i = 0; i < acount; i++)
        the_artillery.setting.push_back(apositions[i]);
}

// Изменения в состоянии мира за отведённый квант времени
void move_do(float tdelta)
{
    Unit *unit;
    // Перемещаем существующие юниты и удаляем отжившие
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end();)
    {
        (*it)->move(tdelta);
        if ((*it)->position.x > 1.0f || (*it)->position.x < -1.0f || (*it)->position.y > 1.0f || (*it)->position.y < -1.0f)
        {
            delete *it;
            it = the_alives.erase(it);
        }
        else
            ++it;
    }
    // Генерируем новые выстрелы
    for (Artillery::Settings::iterator it = the_artillery.setting.begin(); it != the_artillery.setting.end(); ++it)
    {
        it->timeout -= tdelta;
        if (it->timeout <= 0.0f)
        {
            it->timeout = it->delay;
            the_alives.push_back(new Fireball());
            unit = the_alives.back();
            Field::cell_to_pos(&unit->position, it->position.x, it->position.y);
            if (it->speed.x > 0.0f)
                unit->position.x -= CELL_HW;
            else
                unit->position.y -= CELL_HW;
            unit->size = U_SIZE;
            unit->speed = it->speed;
            the_sounds.push_back(seSHOT);
        }
    }
}

// Проверка состояния игры
void state_check()
{
    int x, y;
    Field::pos_to_cell(&x, &y, the_character->position);
    if (the_field.cells[y][x].attribs.count(Cell::atrEXIT) > 0)
    {
        the_state = gsWIN;
        return;
    }
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end(); ++it)
    {
        if (*it == the_character)
            continue;
        if (the_character->is_collided(*(*it)))
        {
            the_state = gsLOSS;
            return;
        }
    }
}

// Очистка всех списков
void lists_clear()
{
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end(); ++it)
        delete *it;
    the_alives.clear();
    the_artillery.setting.clear();
    the_sounds.clear();
}
