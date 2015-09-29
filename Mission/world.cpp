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
Unit *the_character;
CharacterTarget the_way;
SoundsQueue the_sounds;
bool path_requested = false;

std::default_random_engine rand_gen;
std::uniform_real_distribution<float> rand_distrib(0.0f, 1.0f);

void charSetSpeed();
template<class RandomIt, class UniformRandomNumberGenerator>
void shuffle(RandomIt, RandomIt, UniformRandomNumberGenerator&&);

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
Unit::Unit(const Unit& unit)
{
	u_type = unit.u_type;
	size = unit.size;
	position = unit.position;
	speed = unit.speed;
}

Unit::Unit(Type _u_type)
{
	u_type = _u_type;
}

bool Unit::is_collided(const Unit& unit)
{
	float a = position.x - unit.position.x;
	float b = position.y - unit.position.y;
	return sqrt(a * a + b * b) < size + unit.size;
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
void worldSetup()
{
	int i;
	Unit *unit;

	the_sounds.clear();
	the_alives.clear();
	the_artillery.setting.clear();

	// Размечаем поле
	for (int y = 0; y < WORLD_DIM; y++)
		for (int x = 0; x < WORLD_DIM; x++)
			the_field.cells[y][x].attribs.clear();
	the_field.cells[0][WORLD_DIM - 1].attribs.insert(Cell::atrEXIT); // Позиция выхода
	the_field.cells[2][0].attribs.insert(Cell::atrGUARDFORW); // Вешка направления движения охраны
	the_field.cells[2][WORLD_DIM - 1].attribs.insert(Cell::atrGUARDBACKW); // Вешка направления движения охраны
	// Главный герой
	the_alives.push_front(Unit(Unit::utCharacter));
	the_character = &(*the_alives.begin());
	Field::cell_to_pos(&the_character->position, 0, WORLD_DIM - 1);
	the_character->size = U_SIZE;
	the_way.path = ""; // Стоит на месте
	the_way.target.x = 0;
	the_way.target.y = WORLD_DIM - 1;
	charSetSpeed();
	// Стража
	the_alives.push_front(Unit(Unit::utGuard));
	unit = &(*the_alives.begin());
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
	}
	shuffle(apositions.begin(), apositions.end(), rand_gen);
	int acount = min(complexity_apply(ART_COUNT, LEVEL_COMPL), int(apositions.capacity()));
	for (i = 0; i < acount; i++)
		the_artillery.setting.push_back(apositions[i]);
}

// Изменение состояния стражника
static void guardMove(Unit *unit)
{
	int x, y;
	Field::pos_to_cell(&x, &y, unit->position);
	if (the_field.cells[y][x].attribs.count(Cell::atrGUARDBACKW) > 0)
		unit->speed.x = -abs(unit->speed.x);
	if (the_field.cells[y][x].attribs.count(Cell::atrGUARDFORW) > 0)
		unit->speed.x = abs(unit->speed.x);
}

// Устанавливает скорость главного героя
void charSetSpeed()
{
	float dx, dy, adx, ady, a;
	if (the_way.path.length() == 0)
	{
		the_character->speed.x = 0.0f;
		the_character->speed.y = 0.0f;
		return;
	}
	dx = the_way.neigpos.x - the_character->position.x;
	dy = the_way.neigpos.y - the_character->position.y;
	adx = abs(dx);
	ady = abs(dy);
	if (adx < FLT_EPSILON && ady < FLT_EPSILON || the_way.path.length() == 0)
		return;
	if (adx > ady)
	{
		a = atan(ady / adx);
		the_character->speed.x = float(_copysign(CHAR_B_SPEED * cos(a), dx));
		the_character->speed.y = float(_copysign(CHAR_B_SPEED * sin(a), dy));
	}
	else
	{
		a = atan(adx / ady);
		the_character->speed.x = float(_copysign(CHAR_B_SPEED * sin(a), dx));
		the_character->speed.y = float(_copysign(CHAR_B_SPEED * cos(a), dy));
	}
}

// Обновляет состояние главного героя
static void charMove()
{
	int x, y, dx, dy;
	if (the_way.path.length() == 0)
		return;
	if (the_character->speed.x > 0.0f && the_character->position.x >= the_way.neigpos.x
		|| the_character->speed.x < 0.0f && the_character->position.x <= the_way.neigpos.x
		|| the_character->speed.y > 0.0f && the_character->position.y >= the_way.neigpos.y
		|| the_character->speed.y < 0.0f && the_character->position.y <= the_way.neigpos.y)
	{
		// Этап завершен
		the_character->position = the_way.neigpos;
		if (the_way.stage >= the_way.path.length() - 1)
		{
			// Цель достигнута
			the_way.path = "";
			the_way.stage = 0;
			Field::pos_to_cell(&x, &y, the_character->position);
			the_way.target.x = x;
			the_way.target.y = y;
		}
		else
		{
			// Следующий этап
			Field::pos_to_cell(&x, &y, the_character->position);
			++the_way.stage;
			pathDirection(&dx, &dy, the_way.path.at(the_way.stage));
			the_way.neighbour.x += dx;
			the_way.neighbour.y += dy;
			Field::cell_to_pos(&the_way.neigpos, the_way.neighbour.x, the_way.neighbour.y);
		}
		charSetSpeed();
	}
}

void moveDo(float tdelta)
{
	// Перемещаем существующие юниты и удаляем отжившие
	for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end();)
	{
		if (it->u_type == Unit::utCharacter && the_state != gsINPROGRESS)
		{
			++it;
			continue;
		}
		it->position.x += it->speed.x * tdelta;
		it->position.y += it->speed.y * tdelta;
		if (it->u_type == Unit::utGuard)
			guardMove(&(*it));
		if (it->position.x > 1.0f || it->position.x < -1.0 || it->position.y > 1.0f || it->position.y < -1.0)
			it = the_alives.erase(it);
		else
			++it;
	}
	// Обновляем состояние главного героя
	charMove();
	// Генерируем новые выстрелы
	Unit fb(Unit::utFireball);
	for (Artillery::Settings::iterator it = the_artillery.setting.begin(); it != the_artillery.setting.end(); ++it)
	{
		it->timeout -= tdelta;
		if (it->timeout <= 0.0f)
		{
			it->timeout = it->delay;
			Field::cell_to_pos(&fb.position, it->position.x, it->position.y);
			if (abs(it->speed.x) >= FLT_EPSILON)
				fb.position.x -= CELL_HW;
			else
				fb.position.y -= CELL_HW;
			fb.size = U_SIZE;
			fb.speed = it->speed;
			the_alives.push_back(fb);
			the_sounds.push_back(seSHOT);
		}
	}
}

// Проверка состояния игры
void stateCheck()
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
		if (it->u_type == Unit::utCharacter)
			continue;
		if (the_character->is_collided(*it))
		{
			the_state = gsLOSS;
			return;
		}
	}
}

// Реализация отсутствующего в VS2010 std::shuffle
template<class RandomIt, class UniformRandomNumberGenerator>
static void shuffle(RandomIt first, RandomIt last,
	UniformRandomNumberGenerator&& g)
{
	typedef typename std::iterator_traits<RandomIt>::difference_type diff_t;
	typedef typename std::make_unsigned<diff_t>::type udiff_t;
	typedef typename std::uniform_int_distribution<udiff_t> distr_t;
	typedef typename distr_t::param_type param_t;

	distr_t D;
	diff_t n = last - first;
	for (diff_t i = n - 1; i > 0; --i) {
		using std::swap;
		swap(first[i], first[D(g, param_t(0, i))]);
	}
}