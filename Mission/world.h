#pragma once

#include <random>
#include <deque>
#include <set>
#include <list>
#include <vector>
#include "settings.h"

typedef struct {
	float x, y;
}
SpacePosition, // Положение в пространстве, где -1.0 соответствует одной границе поля, а 1.0 противоположной
Speed; // Скорость

// Состояние игры
typedef enum {
	gsINPROGRESS,
	gsLOSS,
	gsWIN
} GameState;

// Звуковое событие
typedef enum {
	seSHOT,
	seHIT,
	seLVLUP
} SoundEvent;

typedef std::deque<SoundEvent> SoundsQueue; // Очередь звуков

void worldSetup();
void moveDo(float);
void stateCheck();
void listsClear();

////////////////////////////////////////////////////////////////////////////////
// Клетка на игровом поле
class Cell
{
public:

	// Какие-либо атрибуты клетки
	typedef enum {
		atrOBSTACLE, // "Препятствие"
		atrEXIT, // Зона выхода
		atrGUARDFORW, // Охране вперёд
		atrGUARDBACKW // Охране назад
	} Attribute;

	typedef std::set<Attribute> Attributes;

	// Позиция на игровом поле
	typedef struct {
		int x, y;
	} Coordinates;

	std::set<Attribute> attribs;

	Cell() {};
	Cell(const Cell&);
	Cell(const Attributes&);
};

////////////////////////////////////////////////////////////////////////////////
// Игровое поле
class Field
{
public:

	Cell cells[WORLD_DIM][WORLD_DIM];

	Field() {};
	Field(const Field&);
	static void cell_to_pos(SpacePosition*, int, int);
	static void pos_to_cell(int *x, int *y, const SpacePosition &position);
};

////////////////////////////////////////////////////////////////////////////////
// Базовый класс игровых юнитов
class Unit
{
public:

    // Спецификация юнита
    typedef enum {
        utUnit,      // Базовый тип
        utCharacter, // Главный герой
        utGuard,     // Стажник
        utFireball   // Выстрел
    } Type;

	float size;  // Радиус юнита
	SpacePosition position;  // Положение в двухмерном пространстве
	Speed speed; // Скорость перемещения

	Unit();
	Unit(const Unit&);
    virtual Type id() { return utUnit; } // Получить тип юнита (RTTI не используем)
	bool is_collided(const Unit&); // Столкнулись ли с другим юнитом
    virtual void move(float); // Осуществляем ход
};

typedef std::list<Unit*> UnitsList; // Группа юнитов

// Главный герой
class Character : public Unit
{
public:

    // Характеристики пути к цели
    typedef struct
    {
        Cell::Coordinates neighbour, target; // Ближайшая ячейка на пути и целевая
        SpacePosition neigpos; // Пространственные координаты центра ближайшей ячейки
        std::string path; // Список директив смены направления
        unsigned stage; // Этап на пути
    } Target;

    Target way; // Набор характеристик пути к цели
    bool path_requested; // Обсчитывается путь

    Character();
    Character(const Character&);
    virtual Type id() { return utCharacter; }
    virtual void move(float);
    void set_speed(); // Устанавливает скорость
    void way_new_request(int, int); // Запрос обсчета пути
    void way_new_process(); // Обработка рассчитанного пути
};

// Стажник
class Guard : public Unit
{
public:
    virtual Type id() { return utGuard; }
    virtual void move(float);
};

// Выстрел
class Fireball : public Unit 
{
    virtual Type id() { return utFireball; }
};

////////////////////////////////////////////////////////////////////////////////
// Пушки
class Artillery
{
public:

	// Настройки ведения огня
	typedef struct {
		Cell::Coordinates position;
		Speed speed;
		float delay;
		float timeout;
	} Setting;

	typedef std::vector<Setting> Settings;

	Settings setting; // Все пушки артбатареи

	Artillery() {};
	Artillery(const Settings&);
};

////////////////////////////////////////////////////////////////////////////////
// Характеристики пути к цели
typedef struct
{
	Cell::Coordinates neighbour, target; // Ближайшая ячейка на пути и целевая
	SpacePosition neigpos; // Пространственные координаты центра ближайшей ячейки
	std::string path; // Список директив смены направления
	unsigned stage; // Этап на пути
} CharacterTarget;

////////////////////////////////////////////////////////////////////////////////
// Основные объекты

extern unsigned level; // Текущий уровень, начиная с 0
extern GameState the_state; // Этап игры
extern Field the_field; // Игровое поле
extern UnitsList the_alives; // Активные объекты
extern Artillery the_artillery; // Все пушки
extern Character *the_character; // Указатель на юнит главного героя, содержащийся в общем списке
extern SoundsQueue the_sounds; // Очередь звуков

extern std::default_random_engine rand_gen;