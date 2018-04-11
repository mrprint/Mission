#pragma once

#include <deque>
#include <bitset>
#include <vector>
#include <algorithm>
#include "settings.hpp"
#include "hfstorage.hpp"
#include "pathfinding.hpp"
#include "spaces.hpp"
#include "mathapp.hpp"

using Speed = tool::Vector2D<tool::fpoint_fast>; // Скорость

// Состояние игры
enum GameState {
    gsINPROGRESS,
    gsLOSS,
    gsWIN
};

// Звуковое событие
enum SoundEvent {
    seSHOT,
    seHIT,
    seLVLUP
};

using SoundsQueue = std::deque<SoundEvent>; // Очередь звуков

////////////////////////////////////////////////////////////////////////////////
// Клетка на игровом поле
class Cell
{
public:

    // Какие-либо атрибуты клетки
    enum Attribute {
        atrOBSTACLE = 0, // "Препятствие"
        atrEXIT, // Зона выхода
        atrGUARDFORW, // Охране вперёд
        atrGUARDBACKW, // Охране назад
        _atrEND
    };

    using Attributes = std::bitset<_atrEND>;

    Attributes attribs;

    Cell() {};
    Cell(const Cell&);
    Cell(const Attributes&);
};

////////////////////////////////////////////////////////////////////////////////
// Игровое поле
class Field
{
    Cell cells[WORLD_DIM][WORLD_DIM];

public:

    Field() {};
    Field(const Field&);

    Cell& operator[](tool::DeskPosition i) { return cells[i.y][i.x]; }
    Cell& operator()(unsigned x, unsigned y) { return cells[y][x]; }
    // Интерфейсный метод для AStar
    bool isobstacle(int x, int y) const { return cells[y][x].attribs.test(Cell::atrOBSTACLE); }
};

using Path = std::vector<tool::DeskPosition>; // Оптимальный путь между ячейками
using FieldsAStar = AStar<WORLD_DIM, WORLD_DIM, tool::DeskPosition, Field>; // AStar, подогнанный к Field

////////////////////////////////////////////////////////////////////////////////
// Базовый класс игровых юнитов
class Unit
{
public:

    // Спецификация юнита
    enum Type {
        utUnit,      // Базовый тип
        utCharacter, // Главный герой
        utGuard,     // Стажник
        utFireball   // Выстрел
    };

    tool::fpoint_fast size;  // Радиус юнита
    tool::SpacePosition position;  // Положение в двухмерном пространстве
    Speed speed; // Скорость перемещения

    Unit();
    // Обеспечивает полноценную деструкцию наследников
    virtual ~Unit() {}
    // Получить тип юнита
    virtual Type id() const { return utUnit; }
    // Столкнулись ли с другим юнитом
    bool is_collided(const Unit&) const;
    // Осуществляем ход
    virtual void move(tool::fpoint_fast);
};

// Главный герой
class Character : public Unit
{
public:

    // Характеристики пути к цели
    struct Target {
        tool::DeskPosition neighbour, target; // Ближайшая ячейка на пути и целевая
        tool::SpacePosition neigpos; // Пространственные координаты центра ближайшей ячейки
        Path path; // Список директив смены направления
        unsigned stage; // Этап на пути
    };

    Target way; // Набор характеристик пути к цели
    bool path_requested; // Обсчитывается путь

    Character();
    virtual Type id() const { return utCharacter; }
    virtual void move(tool::fpoint_fast) override;
    // Устанавливает скорость
    void set_speed();
    // Запрос обсчета пути
    void way_new_request(tool::DeskPosition);
    // Обработка рассчитанного пути
    void way_new_process();
};

// Стражник
class Guard : public Unit
{
public:
    virtual Type id() const { return utGuard; }
    virtual void move(tool::fpoint_fast) override;
};

// Выстрел
class Fireball : public Unit
{
    virtual Type id() const { return utFireball; }
};

using UnitsList = tool::HFStorage<Unit>; // Группа юнитов

////////////////////////////////////////////////////////////////////////////////
// Пушки
class Artillery
{
public:

    // Настройки ведения огня
    struct Setting {
        tool::DeskPosition position;
        Speed speed;
        tool::fpoint_fast delay;
        tool::fpoint_fast timeout;
    };

    using Settings = std::vector<Setting>;

    Settings setting; // Все пушки артбатареи

    Artillery() {};
    Artillery(const Settings&);
};

////////////////////////////////////////////////////////////////////////////////
// Вселенная

class World
{
public:
    unsigned level; // Текущий уровень, начиная с 0
    GameState state; // Этап игры
    Field field; // Игровое поле
    UnitsList alives; // Активные объекты
    Artillery artillery; // Все пушки
    Character *character; // Указатель на юнит главного героя, содержащийся в общем списке
    SoundsQueue sounds; // Очередь звуков

    World() :
        level(0),
        state(gsINPROGRESS),
        alives(std::max(std::max(sizeof(Character), sizeof(Guard)), sizeof(Fireball)), WORLD_DIM * WORLD_DIM / 2)
    { }
    void move_do(tool::fpoint_fast);
    void setup();
    void state_check();
    void lists_clear();
};

extern World the_world;

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
