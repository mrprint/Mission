#pragma once

#include <math.h>
#include <SFML/Graphics.hpp>
#include "settings.hpp"

////////////////////////////////////////////////////////////////////////////////
// Координаты в различтных пространствах и автоматические межпространственные 
// преобразования

// Идентификаторы обособленных классов
enum {
    icSPACE,
    icDESK,
    icSCREEN
};

// Точка в двумерном пространстве
template <typename T, int _class=icSPACE>
struct Vector2D
{
    typedef T basetype;
    basetype x, y;
    Vector2D() { x = static_cast<basetype>(0); y = static_cast<basetype>(0); }
    Vector2D(const Vector2D& val) : x(val.x), y(val.y) {}
    Vector2D(basetype _x, basetype _y) : x(_x), y(_y) {}
    Vector2D(basetype val) : x(val), y(val) {}
    Vector2D& operator=(basetype val) { x = val; y = val; return *this; }
    Vector2D operator-() const { return Vector2D(-x, -y); }
    Vector2D operator+(const Vector2D& val) const { return Vector2D(x + val.x, y + val.y); }
    Vector2D operator-(const Vector2D& val) const { return Vector2D(x - val.x, y - val.y); }
    Vector2D& operator+=(const Vector2D& val) { x += val.x; y += val.y; return *this; }
    Vector2D& operator-=(const Vector2D& val) { x -= val.x; y -= val.y; return *this; }
    Vector2D& operator+=(basetype val) { x += val; y += val; return *this; }
    Vector2D& operator-=(basetype val) { x -= val; y -= val; return *this; }
    Vector2D operator*(const Vector2D& val) const { return Vector2D(x * val.x, y * val.y); }
    Vector2D operator/(const Vector2D& val) const { return Vector2D(x / val.x, y / val.y); }
    Vector2D& operator*=(basetype val) { x *= val; y *= val; return *this; }
    Vector2D& operator/=(basetype val) { x /= val; y /= val; return *this; }
    // dot product
    basetype operator%(const Vector2D& val) const { return x * val.x + y * val.y; }
    // distance
    basetype operator >> (const Vector2D& val) const { Vector2D t = *this - val; return static_cast<basetype>(hypot(t.x, t.y)); }
    Vector2D rotate(const Vector2D& val) const { return Vector2D(x*val.x - y*val.y, x*val.y + y*val.x); }
};

struct DeskPosition;
struct ScreenPosition;

typedef Vector2D<float> _Vector2D_Space;
// Положение в пространстве, где -1.0 соответствует одной границе поля, а 1.0 противоположной
struct SpacePosition : _Vector2D_Space
{
    SpacePosition() {}
    SpacePosition(const SpacePosition& val) : _Vector2D_Space(val) {}
    SpacePosition(basetype val) : _Vector2D_Space(val) {}
    SpacePosition(basetype _x, basetype _y) : _Vector2D_Space(_x, _y) {}
    SpacePosition(int _x, int _y) : _Vector2D_Space(static_cast<basetype>(_x), static_cast<basetype>(_y)) {}
    SpacePosition(const DeskPosition& val) { this->operator=(val); }
    SpacePosition(const ScreenPosition& val) { this->operator=(val); }
    SpacePosition(const _Vector2D_Space& val) : _Vector2D_Space(val) {}
    SpacePosition& operator=(basetype val) { this->_Vector2D_Space::operator=(val); return *this; }
    SpacePosition& operator=(const DeskPosition& val);
    SpacePosition& operator=(const ScreenPosition& val);
};

typedef Vector2D<int, icDESK> _Vector2D_Desk;
// Позиция на игровом поле
struct DeskPosition : _Vector2D_Desk
{
    DeskPosition() {}
    DeskPosition(const DeskPosition& val) : _Vector2D_Desk(val) {}
    DeskPosition(basetype val) : _Vector2D_Desk(val) {}
    DeskPosition(basetype _x, basetype _y) : _Vector2D_Desk(_x, _y) {}
    DeskPosition(const SpacePosition& val) { this->operator=(val); }
    DeskPosition(const ScreenPosition& val) { this->operator=(val); }
    DeskPosition(const _Vector2D_Desk& val) : _Vector2D_Desk(val) {}
    DeskPosition& operator=(basetype val) { this->_Vector2D_Desk::operator=(val); return *this; }
    DeskPosition& operator=(const SpacePosition& val);
    DeskPosition& operator=(const ScreenPosition& val) { *this = SpacePosition(val); return *this; };
};

typedef Vector2D<float, icSCREEN> _Vector2D_Screen;
// Позиция на дисплее (читает настройки отображения непосредственно из экземпляра Engine)
struct ScreenPosition : _Vector2D_Screen
{
    ScreenPosition() {}
    ScreenPosition(const ScreenPosition& val) : _Vector2D_Screen(val) {}
    ScreenPosition(basetype val) : _Vector2D_Screen(val) {}
    ScreenPosition(basetype _x, basetype _y) : _Vector2D_Screen(_x, _y) {}
    ScreenPosition(int _x, int _y) : _Vector2D_Screen(static_cast<basetype>(_x), static_cast<basetype>(_y)) {}
    ScreenPosition(const SpacePosition& val) { this->operator=(val); }
    ScreenPosition(const DeskPosition& val) { this->operator=(val); }
    ScreenPosition(const _Vector2D_Screen& val) : _Vector2D_Screen(val) {}
    ScreenPosition& operator=(basetype val) { this->_Vector2D_Screen::operator=(val); return *this; }
    ScreenPosition& operator=(const SpacePosition& val);
    ScreenPosition& operator=(const DeskPosition& val) { *this = SpacePosition(val); return *this; }
    operator sf::Vector2f() const { return sf::Vector2f(x, y); }
};
