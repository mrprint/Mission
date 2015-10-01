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
SpacePosition, // ��������� � ������������, ��� -1.0 ������������� ����� ������� ����, � 1.0 ���������������
Speed; // ��������

// ��������� ����
typedef enum {
	gsINPROGRESS,
	gsLOSS,
	gsWIN
} GameState;

// �������� �������
typedef enum {
	seSHOT,
	seHIT,
	seLVLUP
} SoundEvent;

typedef std::deque<SoundEvent> SoundsQueue; // ������� ������

void worldSetup();
void moveDo(float);
void stateCheck();
void listsClear();

////////////////////////////////////////////////////////////////////////////////
// ������ �� ������� ����
class Cell
{
public:

	// �����-���� �������� ������
	typedef enum {
		atrOBSTACLE, // "�����������"
		atrEXIT, // ���� ������
		atrGUARDFORW, // ������ �����
		atrGUARDBACKW // ������ �����
	} Attribute;

	typedef std::set<Attribute> Attributes;

	// ������� �� ������� ����
	typedef struct {
		int x, y;
	} Coordinates;

	std::set<Attribute> attribs;

	Cell() {};
	Cell(const Cell&);
	Cell(const Attributes&);
};

////////////////////////////////////////////////////////////////////////////////
// ������� ����
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
// ������� ����� ������� ������
class Unit
{
public:

    // ������������ �����
    typedef enum {
        utUnit,      // ������� ���
        utCharacter, // ������� �����
        utGuard,     // �������
        utFireball   // �������
    } Type;

	float size;  // ������ �����
	SpacePosition position;  // ��������� � ���������� ������������
	Speed speed; // �������� �����������

	Unit();
	Unit(const Unit&);
    virtual Type id() { return utUnit; } // �������� ��� ����� (RTTI �� ����������)
	bool is_collided(const Unit&); // ����������� �� � ������ ������
    virtual void move(float); // ������������ ���
};

typedef std::list<Unit*> UnitsList; // ������ ������

// ������� �����
class Character : public Unit
{
public:

    // �������������� ���� � ����
    typedef struct
    {
        Cell::Coordinates neighbour, target; // ��������� ������ �� ���� � �������
        SpacePosition neigpos; // ���������������� ���������� ������ ��������� ������
        std::string path; // ������ �������� ����� �����������
        unsigned stage; // ���� �� ����
    } Target;

    Target way; // ����� ������������� ���� � ����
    bool path_requested; // ������������� ����

    Character();
    Character(const Character&);
    virtual Type id() { return utCharacter; }
    virtual void move(float);
    void set_speed(); // ������������� ��������
    void way_new_request(int, int); // ������ ������� ����
    void way_new_process(); // ��������� ������������� ����
};

// �������
class Guard : public Unit
{
public:
    virtual Type id() { return utGuard; }
    virtual void move(float);
};

// �������
class Fireball : public Unit 
{
    virtual Type id() { return utFireball; }
};

////////////////////////////////////////////////////////////////////////////////
// �����
class Artillery
{
public:

	// ��������� ������� ����
	typedef struct {
		Cell::Coordinates position;
		Speed speed;
		float delay;
		float timeout;
	} Setting;

	typedef std::vector<Setting> Settings;

	Settings setting; // ��� ����� ����������

	Artillery() {};
	Artillery(const Settings&);
};

////////////////////////////////////////////////////////////////////////////////
// �������������� ���� � ����
typedef struct
{
	Cell::Coordinates neighbour, target; // ��������� ������ �� ���� � �������
	SpacePosition neigpos; // ���������������� ���������� ������ ��������� ������
	std::string path; // ������ �������� ����� �����������
	unsigned stage; // ���� �� ����
} CharacterTarget;

////////////////////////////////////////////////////////////////////////////////
// �������� �������

extern unsigned level; // ������� �������, ������� � 0
extern GameState the_state; // ���� ����
extern Field the_field; // ������� ����
extern UnitsList the_alives; // �������� �������
extern Artillery the_artillery; // ��� �����
extern Character *the_character; // ��������� �� ���� �������� �����, ������������ � ����� ������
extern SoundsQueue the_sounds; // ������� ������

extern std::default_random_engine rand_gen;