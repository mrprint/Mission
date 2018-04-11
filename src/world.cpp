#include "settings.hpp"
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>
#include <new>
#include <random>
#include <chrono>
#include "world.hpp"
#include "spaces.hpp"
#include "pathfinding.hpp"
#include "coworker.hpp"
#include "mathapp.hpp"

using namespace std;

using SpacePosition = tool::SpacePosition;
using DeskPosition = tool::DeskPosition;
using ScreenPosition = tool::ScreenPosition;

World the_world;

static default_random_engine rng;
static uniform_real_distribution<> ureal_dist(0.0, 1.0);

// Мелкие вспомогательные функции

static inline bool rnd_choice(tool::fpoint_fast possib)
{
    return ureal_dist(rng) < possib;
}

static inline tool::fpoint_fast deviation_apply(tool::fpoint_fast val, tool::fpoint_fast dev)
{
    return (2 * dev * static_cast<tool::fpoint_fast>(ureal_dist(rng)) - dev + 1) * val;
}

static inline int complexity_apply(int val, tool::fpoint_fast kc)
{
    return val + static_cast<int>(round(val * the_world.level * kc));
}

static inline tool::fpoint_fast complexity_apply(tool::fpoint_fast val, tool::fpoint_fast kc)
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
    return position.distance(unit.position) < size + unit.size;
}

void Unit::move(tool::fpoint_fast tdelta)
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

void Character::move(tool::fpoint_fast tdelta)
{
    if (the_world.state == gsINPROGRESS)
        // Перемещаемся только во время игры
        Unit::move(tdelta);
    if (way.path.size() == 0)
        return;
    constexpr auto eps = numeric_limits<tool::fpoint_fast>::epsilon();
    if ((speed.x > eps && position.x >= way.neigpos.x)
        || (speed.x < -eps && position.x <= way.neigpos.x)
        || (speed.y > eps && position.y >= way.neigpos.y)
        || (speed.y < -eps && position.y <= way.neigpos.y))
    {
        // Этап завершен
        position = way.neigpos;
        if (way.stage >= way.path.size() - 1)
        {
            // Цель достигнута
            way.path.clear();
            way.stage = 0;
            way.target = position;
        } else
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
    auto d = way.neigpos - position;
    if (d.atzero())
    {
        speed = 0.0f;
        return;
    }
    speed = d.normalized() * CHAR_B_SPEED;
}

// Запрос обсчета пути
void Character::way_new_request(DeskPosition pos)
{
    speed = 0.0f;
    way.target = pos;
    the_coworker.path_find_request(the_world.field, DeskPosition(position), pos);
    path_requested = true;
}

// Обработка рассчитанного пути
void Character::way_new_process()
{
    the_coworker.path_read(way.path);
    if (way.path.size() > 0)
    {
        way.stage = 0;
        way.neighbour = static_cast<DeskPosition>(position) + way.path[way.path.size() - 1];
        way.neigpos = way.neighbour;
    }
    set_speed();
}

////////////////////////////////////////////////////////////////////////////////
void Guard::move(tool::fpoint_fast tdelta)
{
    Unit::move(tdelta);
    auto dp = DeskPosition(position);
    if (the_world.field[dp].attribs.test(Cell::atrGUARDBACKW))
        speed.x = -abs(speed.x);
    if (the_world.field[dp].attribs.test(Cell::atrGUARDFORW))
        speed.x = abs(speed.x);
}

////////////////////////////////////////////////////////////////////////////////
// Инициализация вселенной
void World::setup()
{
    rng.seed(
        static_cast<unsigned>(
            chrono::duration_cast<chrono::milliseconds>(
                chrono::steady_clock::now().time_since_epoch()
                ).count()
            )
    );
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
    shuffle(apositions.begin(), apositions.end(), rng);
    auto apend = apositions.begin()
        + min(complexity_apply(ART_COUNT, LEVEL_COMPL), static_cast<int>(apositions.capacity()));
    copy(apositions.begin(), apend, back_inserter(artillery.setting));
}

// Изменения в состоянии мира за отведённый квант времени
void World::move_do(tool::fpoint_fast tdelta)
{
    // Перемещаем существующие юниты и удаляем отжившие
    for (UnitsList::iterator it = alives.begin(); it != alives.end();)
    {
        it->move(tdelta);
        if (it->position.x > 1.0f || it->position.x < -1.0f || it->position.y > 1.0f || it->position.y < -1.0f)
        {
            it->~Unit();
            it = alives.erase(it);
        } else
            ++it;
    }
    // Генерируем новые выстрелы
    for (auto &setting : artillery.setting)
    {
        setting.timeout -= tdelta;
        if (setting.timeout <= 0.0f)
        {
            setting.timeout = setting.delay;
            if (alives.full())
                break;
            auto pnewfb = reinterpret_cast<Fireball*>(alives.allocate());
            new (pnewfb) Fireball();
            pnewfb->position = setting.position;
            if (setting.speed.x > 0.0f)
                pnewfb->position.x -= CELL_HW;
            else
                pnewfb->position.y -= CELL_HW;
            pnewfb->size = U_SIZE;
            pnewfb->speed = setting.speed;
            sounds.push_back(seSHOT);
        }
    }
}

// Проверка состояния игры
void World::state_check()
{
    if (field[DeskPosition(character->position)].attribs.test(Cell::atrEXIT))
    {
        state = gsWIN;
        return;
    }
    for (auto &alive : alives)
    {
        if (&alive == character)
            continue;
        if (character->is_collided(alive))
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
