#include <math.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include "settings.hpp"
#include "world.hpp"
#include "spaces.hpp"
#include "pathfinding.hpp"
#include "main.hpp"

unsigned level;
GameState the_state;
Field the_field;
UnitsList the_alives;
Artillery the_artillery;
Character *the_character;
SoundsQueue the_sounds;

// Мелкие вспомогательные функции
static inline int randint(int max = RAND_MAX)
{
    return rand() % max;
}

static inline float randomf()
{
    return static_cast<float>(randint()) / static_cast <float>(RAND_MAX);
}

static inline bool rnd_choice(float possib)
{
    return randomf() < possib;
}

template<class T>
static void shuffle(T, T);

static inline float deviation_apply(float val, float dev)
{
    return (2 * dev * randomf() - dev + 1) * val;
}

template<class T>
static inline T round(T number)
{
    return number < static_cast<T>(0.0) ? ceil(number - static_cast<T>(0.5)) : floor(number + static_cast<T>(0.5));
}

static inline int complexity_apply(int val, float kc)
{
    return val + static_cast<int>(round(val * level * kc));
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

////////////////////////////////////////////////////////////////////////////////
Unit::Unit()
{
    size = U_SIZE;
}

Unit::Unit(const Unit& unit)
{
    size = unit.size;
    position = unit.position;
    speed = unit.speed;
}

bool Unit::is_collided(const Unit& unit) const
{
    return position >> unit.position < size + unit.size;
}

void Unit::move(float tdelta)
{
    position += speed * tdelta;
}

////////////////////////////////////////////////////////////////////////////////
Character::Character() : Unit()
{
    path_requested = false;
    way.path.clear(); // Стоит на месте
    way.target = 0;
}

Character::Character(const Character& character) : Unit(character)
{
    path_requested = false;
    way = character.way;
}

void Character::move(float tdelta)
{
    if (the_state == gsINPROGRESS)
        // Перемещаемся только во время игры
        Unit::move(tdelta);
    if (way.path.size() == 0)
        return;
    if ((speed.x > F_EPSILON && position.x >= way.neigpos.x)
        || (speed.x < -F_EPSILON && position.x <= way.neigpos.x)
        || (speed.y > F_EPSILON && position.y >= way.neigpos.y)
        || (speed.y < -F_EPSILON && position.y <= way.neigpos.y))
    {
        // Этап завершен
        position = way.neigpos;
        if (way.stage >= way.path.size() - 1)
        {
            // Цель достигнута
            way.path.clear();
            way.stage = 0;
            way.target = position;
        }
        else
        {
            // Следующий этап
            ++way.stage;
            way.neighbour += way.path[way.path.size() - way.stage - 1];
            way.neigpos = way.neighbour;
        }
        set_speed();
    }
}

void Character::set_speed()
{
    if (way.path.size() == 0)
    {
        speed = 0.0f;
        return;
    }
    SpacePosition d = way.neigpos - position;
    if (d.x < F_EPSILON && d.x > -F_EPSILON && d.y < F_EPSILON && d.y > -F_EPSILON)
    {
        speed = 0.0f;
        return;
    }
    float a = atan2(d.x, d.y);
    speed = SpacePosition(static_cast<float>(sin(a)), static_cast<float>(cos(a))) * CHAR_B_SPEED;
}

// Запрос обсчета пути
void Character::way_new_request(DeskPosition pos)
{
    speed = 0.0f;
    way.target = pos;
    the_coworker.path_find_request(the_field, position, pos);
    path_requested = true;
}

// Обработка рассчитанного пути
void Character::way_new_process()
{
    way.path = the_coworker.path_read();
    if (way.path.size() > 0)
    {
        way.stage = 0;
        way.neighbour = static_cast<DeskPosition>(position) + way.path[way.path.size() - 1];
        way.neigpos = way.neighbour;
    }
    set_speed();
}

////////////////////////////////////////////////////////////////////////////////
void Guard::move(float tdelta)
{
    Unit::move(tdelta);
    DeskPosition dp = position;
    if (the_field[dp].attribs.count(Cell::atrGUARDBACKW) > 0)
        speed.x = -fabs(speed.x);
    if (the_field[dp].attribs.count(Cell::atrGUARDFORW) > 0)
        speed.x = fabs(speed.x);
}

////////////////////////////////////////////////////////////////////////////////
// Инициализация вселенной
void world_setup()
{
    int i;
    Unit *unit;

    lists_clear();

    // Размечаем поле
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
            the_field(x, y).attribs.clear();
    the_field(WORLD_DIM - 1, 0).attribs.insert(Cell::atrEXIT); // Позиция выхода
    the_field(0, 2).attribs.insert(Cell::atrGUARDFORW); // Вешка направления движения охраны
    the_field(WORLD_DIM - 1, 2).attribs.insert(Cell::atrGUARDBACKW); // Вешка направления движения охраны
    // Главный герой
    the_character = new Character();
    the_alives.push_back(the_character);
    the_character->position = DeskPosition(0, WORLD_DIM - 1);
    the_character->way.target = DeskPosition(0, WORLD_DIM - 1);
    the_character->set_speed();
    // Стража
    unit = new Guard();
    the_alives.push_back(unit);
    unit->position = DeskPosition(0, 2);
    unit->size = U_SIZE * 1.5f;
    unit->speed = Speed(GUARD_B_SPEED, 0.0f);
    // Артиллерия
    Artillery::Settings apositions(WORLD_DIM * 2 - 2);
    for (i = 0; i < WORLD_DIM - 1; i++)
    {
        apositions[i].position = DeskPosition(i, 0);
        apositions[i].speed = Speed(0.0f, deviation_apply(ART_B_SPEED, ART_DEV));
        apositions[i].delay = deviation_apply(ART_B_DELAY, ART_DEV);
        apositions[i].timeout = 0.0f;
        apositions[WORLD_DIM - 1 + i].position = DeskPosition(0, i);
        apositions[WORLD_DIM - 1 + i].speed = Speed(deviation_apply(complexity_apply(ART_B_SPEED, LEVEL_COMPL), ART_DEV), 0.0f);
        apositions[WORLD_DIM - 1 + i].delay = deviation_apply(ART_B_DELAY, ART_DEV);
        apositions[WORLD_DIM - 1 + i].timeout = 0.0f;
    }
    shuffle(apositions.begin(), apositions.end());
    int acount = std::min(complexity_apply(ART_COUNT, LEVEL_COMPL), static_cast<int>(apositions.capacity()));
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
            unit = new Fireball();
            the_alives.push_back(unit);
            unit->position = it->position;
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
    //DeskPosition dp = the_character->position;
    if (the_field[the_character->position].attribs.count(Cell::atrEXIT) > 0)
    {
        the_state = gsWIN;
        return;
    }
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end(); ++it)
    {
        if (*it == the_character)
            continue;
        if (the_character->is_collided(**it))
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

template<class T>
static void shuffle(T first, T last)
{
    typedef typename std::iterator_traits<T>::difference_type diff_t;

    diff_t n = last - first;
    for (diff_t i = n - 1; i > 0; --i) {
        using std::swap;
        swap(first[i], first[static_cast<diff_t>(randint(static_cast<int>(i)))]);
    }
}
