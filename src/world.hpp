#pragma once

#include <deque>
#include <set>
#include <vector>
#include <algorithm>
#include <cstring>
#include "settings.hpp"
#include "hfstorage.hpp"
#include "pathfinding.hpp"
#include "spaces.hpp"

typedef Vector2D<float> Speed; // Скорость

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

typedef std::deque<SoundEvent> SoundsQueue; // Очередь звуков

void world_setup();
void move_do(float);
void state_check();
void lists_clear();

////////////////////////////////////////////////////////////////////////////////
// Клетка на игровом поле
class Cell
{
public:

    // Какие-либо атрибуты клетки
    enum Attribute {
        atrOBSTACLE, // "Препятствие"
        atrEXIT, // Зона выхода
        atrGUARDFORW, // Охране вперёд
        atrGUARDBACKW // Охране назад
    };

    typedef std::set<Attribute> Attributes;

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

    Cell& operator[](DeskPosition i) { return cells[i.y][i.x]; }
    Cell& operator()(unsigned x, unsigned y) { return cells[y][x]; }
    // Интерфейсный метод для AStar
    bool isobstacle(int x, int y) const { return cells[y][x].attribs.count(Cell::atrOBSTACLE) > 0; }
};

typedef std::vector<DeskPosition> Path; // Оптимальный путь между ячейками
typedef AStar<WORLD_DIM, WORLD_DIM, DeskPosition, Field> FieldsAStar; // AStar, подогнанный к Field

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

    float size;  // Радиус юнита
    SpacePosition position;  // Положение в двухмерном пространстве
    Speed speed; // Скорость перемещения

    Unit();
    virtual ~Unit() {} // Обеспечиваем полноценную деструкцию наследников
    // Копирует всё, включая VMT (протокол произвольного хранения в памяти)
    // Переопределяется для классов с расширенным набором полей данных
    Unit& operator=(const Unit &unit) { memcpy(this, &unit, sizeof(Unit)); return *this; }
    virtual Type id() const { return utUnit; } // Получить тип юнита (RTTI не используем)
    bool is_collided(const Unit&) const; // Столкнулись ли с другим юнитом
    virtual void move(float); // Осуществляем ход
};

// Главный герой
class Character : public Unit
{
public:

    // Характеристики пути к цели
    struct Target {
        DeskPosition neighbour, target; // Ближайшая ячейка на пути и целевая
        SpacePosition neigpos; // Пространственные координаты центра ближайшей ячейки
        Path path; // Список директив смены направления
        unsigned stage; // Этап на пути
        // Протокол произвольного хранения в памяти
        Target& operator=(const Target &target)
        {
            memcpy(this, &target, sizeof(Target));
            memset(&path, 0, sizeof(Path));
            path = target.path;
            return *this;
        }
    };

    Target way; // Набор характеристик пути к цели
    bool path_requested; // Обсчитывается путь

    Character();
    Character& operator=(const Character&); // Копирует всё, включая VMT (протокол произвольного хранения в памяти)
    virtual Type id() const { return utCharacter; }
    virtual void move(float);
    void set_speed(); // Устанавливает скорость
    void way_new_request(DeskPosition); // Запрос обсчета пути
    void way_new_process(); // Обработка рассчитанного пути
};

// Стражник
class Guard : public Unit
{
public:
    virtual Type id() const { return utGuard; }
    virtual void move(float);
};

// Выстрел
class Fireball : public Unit
{
    virtual Type id() const { return utFireball; }
};

typedef HFStorage<Unit> UnitsList; // Группа юнитов

////////////////////////////////////////////////////////////////////////////////
// Пушки
class Artillery
{
public:

    // Настройки ведения огня
    struct Setting {
        DeskPosition position;
        Speed speed;
        float delay;
        float timeout;
    };

    typedef std::vector<Setting> Settings;

    Settings setting; // Все пушки артбатареи

    Artillery() {};
    Artillery(const Settings&);
};

////////////////////////////////////////////////////////////////////////////////
// Основные объекты

extern unsigned level; // Текущий уровень, начиная с 0
extern GameState the_state; // Этап игры
extern Field the_field; // Игровое поле
extern UnitsList the_alives; // Активные объекты
extern Artillery the_artillery; // Все пушки
extern Character *the_character; // Указатель на юнит главного героя, содержащийся в общем списке
extern SoundsQueue the_sounds; // Очередь звуков
