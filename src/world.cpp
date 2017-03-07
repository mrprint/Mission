#include "settings.hpp"
#include <math.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <new>
#include "world.hpp"
#include "spaces.hpp"
#include "pathfinding.hpp"
#include "main.hpp"


World the_world;

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

static inline float deviation_apply(float val, float dev)
{
    return (2 * dev * randomf() - dev + 1) * val;
}

template<typename T>
static inline T round(T number)
{
    return number < static_cast<T>(0.0) ? ceil(number - static_cast<T>(0.5)) : floor(number + static_cast<T>(0.5));
}

static inline int complexity_apply(int val, float kc)
{
    return val + static_cast<int>(round(val * the_world.level * kc));
}

static inline float complexity_apply(float val, float kc)
{
    return val + val * the_world.level * kc;
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

void Character::move(float tdelta)
{
    if (the_world.state == gsINPROGRESS)
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
    the_coworker.path_find_request(the_world.field, position, pos);
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
    if (the_world.field[dp].attribs.test(Cell::atrGUARDBACKW))
        speed.x = -fabs(speed.x);
    if (the_world.field[dp].attribs.test(Cell::atrGUARDFORW))
        speed.x = fabs(speed.x);
}

////////////////////////////////////////////////////////////////////////////////
// Инициализация вселенной
void World::setup()
{
    lists_clear();

    // Размечаем поле
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
            field(x, y).attribs.reset();
    field(WORLD_DIM - 1, 0).attribs.set(Cell::atrEXIT); // Позиция выхода
    field(0, 2).attribs.set(Cell::atrGUARDFORW); // Вешка направления движения охраны
    field(WORLD_DIM - 1, 2).attribs.set(Cell::atrGUARDBACKW); // Вешка направления движения охраны
    // Главный герой
    {
        character = reinterpret_cast<Character*>(alives.allocate());
        new (character) Character();
        character->position = DeskPosition(0, WORLD_DIM - 1);
        character->way.target = DeskPosition(0, WORLD_DIM - 1);
        character->set_speed();
    }
    // Стража
    {
        Guard *pgrd;
        pgrd = reinterpret_cast<Guard*>(alives.allocate());
        new (pgrd) Guard();
        pgrd->position = DeskPosition(0, 2);
        pgrd->size = U_SIZE * 1.5f;
        pgrd->speed = Speed(GUARD_B_SPEED, 0.0f);
    }
    // Артиллерия
    Artillery::Settings apositions(WORLD_DIM * 2 - 2);
    for (int i = 0; i < WORLD_DIM - 1; i++)
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
    std::random_shuffle(apositions.begin(), apositions.end());
    int acount = std::min(complexity_apply(ART_COUNT, LEVEL_COMPL), static_cast<int>(apositions.capacity()));
    for (int i = 0; i < acount; i++)
        artillery.setting.push_back(apositions[i]);
}

// Изменения в состоянии мира за отведённый квант времени
void World::move_do(float tdelta)
{
    // Перемещаем существующие юниты и удаляем отжившие
    for (UnitsList::iterator it = alives.begin(); it != alives.end();)
    {
        Unit *u = &*it;
        u->move(tdelta);
        if (it->position.x > 1.0f || it->position.x < -1.0f || it->position.y > 1.0f || it->position.y < -1.0f)
        {
            it->~Unit();
            it = alives.erase(it);
        }
        else
            ++it;
    }
    // Генерируем новые выстрелы
    {
        Fireball *pnewfb;
        for (Artillery::Settings::iterator it = artillery.setting.begin(); it != artillery.setting.end(); ++it)
        {
            it->timeout -= tdelta;
            if (it->timeout <= 0.0f)
            {
                it->timeout = it->delay;
                pnewfb = reinterpret_cast<Fireball*>(alives.allocate());
                if (!pnewfb)
                    // Контейнер переполнен
                    continue;
                new (pnewfb) Fireball();
                pnewfb->position = it->position;
                if (it->speed.x > 0.0f)
                    pnewfb->position.x -= CELL_HW;
                else
                    pnewfb->position.y -= CELL_HW;
                pnewfb->size = U_SIZE;
                pnewfb->speed = it->speed;
                sounds.push_back(seSHOT);
            }
        }
    }
}

// Проверка состояния игры
void World::state_check()
{
    if (field[character->position].attribs.test(Cell::atrEXIT))
    {
        state = gsWIN;
        return;
    }
    for (UnitsList::iterator it = alives.begin(); it != alives.end(); ++it)
    {
        if (&*it == character)
            continue;
        if (character->is_collided(*it))
        {
            state = gsLOSS;
            return;
        }
    }
}

// Очистка всех списков
void World::lists_clear()
{
    for (UnitsList::iterator it = alives.begin(); !alives.empty(); )
    {
        it->~Unit();
        it = alives.erase(it);
    }
    artillery.setting.clear();
    sounds.clear();
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
