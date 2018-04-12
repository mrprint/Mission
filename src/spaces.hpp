#pragma once

#include <cmath>
#include <limits>
#include <SFML/Graphics.hpp>
#include "settings.hpp"
#include "mathapp.hpp"

namespace tool
{
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
    template <typename T, int _class = icSPACE>
    struct Vector2D
    {
        using basetype = T;
        basetype x, y;
        Vector2D() noexcept { x = static_cast<basetype>(0); y = static_cast<basetype>(0); }
        Vector2D(const Vector2D& val) noexcept : x(val.x), y(val.y) {}
        Vector2D(basetype _x, basetype _y) noexcept : x(_x), y(_y) {}
        explicit Vector2D(basetype val) noexcept : x(val), y(val) {}
        Vector2D& operator=(basetype val) noexcept { x = val; y = val; return *this; }
        Vector2D operator-() const { return Vector2D(-x, -y); }
        Vector2D operator+(const Vector2D& val) const { return Vector2D(x + val.x, y + val.y); }
        Vector2D operator+(basetype val) const { return *this + Vector2D(val); }
        Vector2D operator-(const Vector2D& val) const { return Vector2D(x - val.x, y - val.y); }
        Vector2D operator-(basetype val) const { return *this - Vector2D(val); }
        Vector2D& operator+=(const Vector2D& val) noexcept { x += val.x; y += val.y; return *this; }
        Vector2D& operator-=(const Vector2D& val) noexcept { x -= val.x; y -= val.y; return *this; }
        Vector2D& operator+=(basetype val) noexcept { x += val; y += val; return *this; }
        Vector2D& operator-=(basetype val) noexcept { x -= val; y -= val; return *this; }
        Vector2D operator*(const Vector2D& val) const { return Vector2D(x * val.x, y * val.y); }
        Vector2D operator*(basetype val) const { return *this * Vector2D(val); }
        Vector2D operator/(const Vector2D& val) const { return Vector2D(x / val.x, y / val.y); }
        Vector2D operator/(basetype val) const { return *this / Vector2D(val); }
        Vector2D& operator*=(basetype val) noexcept { x *= val; y *= val; return *this; }
        Vector2D& operator/=(basetype val) noexcept { x /= val; y /= val; return *this; }
        basetype dot(const Vector2D& val) const { return x * val.x + y * val.y; }
        basetype dot(basetype val) const { return dot(Vector2D(val)); }
        basetype distance(const Vector2D& val) const { Vector2D t = *this - val; return static_cast<basetype>(hypot_fast(t.x, t.y)); }
        basetype distance(basetype val) const { return distance(Vector2D(val)); }
        basetype hypot() const { return static_cast<basetype>(hypot_fast(x, y)); }
        Vector2D normalized() const { return *this / hypot(); }
        Vector2D rotate(const Vector2D& val) const { return Vector2D(x*val.x - y * val.y, x*val.y + y * val.x); }
        Vector2D rotate(basetype val) const { return rotate(Vector2D(val)); }
        bool atzero() const {
            constexpr auto eps = std::numeric_limits<tool::fpoint_fast>::epsilon();
            return x < eps && x > -eps && y < eps && y > -eps;
        }
    };

    struct DeskPosition;
    struct ScreenPosition;

    using _Vector2D_Space = Vector2D<tool::fpoint_fast>;
    // Положение в пространстве, где -1.0 соответствует одной границе поля, а 1.0 противоположной
    struct SpacePosition : _Vector2D_Space
    {
        SpacePosition() noexcept {}
        SpacePosition(const SpacePosition& val) noexcept : _Vector2D_Space(val) {}
        explicit SpacePosition(basetype val) noexcept : _Vector2D_Space(val) {}
        SpacePosition(basetype _x, basetype _y) noexcept : _Vector2D_Space(_x, _y) {}
        SpacePosition(int _x, int _y) noexcept : _Vector2D_Space(static_cast<basetype>(_x), static_cast<basetype>(_y)) {}
        explicit SpacePosition(const DeskPosition& val) noexcept { this->operator=(val); }
        explicit SpacePosition(const ScreenPosition& val) noexcept { this->operator=(val); }
        SpacePosition(const _Vector2D_Space& val) noexcept : _Vector2D_Space(val) {}
        SpacePosition& operator=(basetype val) noexcept { this->_Vector2D_Space::operator=(val); return *this; }
        SpacePosition& operator=(const DeskPosition& val) noexcept;
        SpacePosition& operator=(const ScreenPosition& val) noexcept;
    };

    using _Vector2D_Desk = Vector2D<int, icDESK>;
    // Позиция на игровом поле
    struct DeskPosition : _Vector2D_Desk
    {
        DeskPosition() noexcept {}
        DeskPosition(const DeskPosition& val) noexcept : _Vector2D_Desk(val) {}
        explicit DeskPosition(basetype val) noexcept : _Vector2D_Desk(val) {}
        DeskPosition(basetype _x, basetype _y) noexcept : _Vector2D_Desk(_x, _y) {}
        explicit DeskPosition(const SpacePosition& val) noexcept { this->operator=(val); }
        explicit DeskPosition(const ScreenPosition& val) noexcept { this->operator=(val); }
        DeskPosition(const _Vector2D_Desk& val) noexcept : _Vector2D_Desk(val) {}
        DeskPosition& operator=(basetype val) noexcept { this->_Vector2D_Desk::operator=(val); return *this; }
        DeskPosition& operator=(const SpacePosition& val) noexcept;
        DeskPosition& operator=(const ScreenPosition& val) noexcept { *this = SpacePosition(val); return *this; };
    };

    using _Vector2D_Screen = Vector2D<tool::fpoint_fast, icSCREEN>;
    // Позиция на дисплее (читает настройки отображения непосредственно из экземпляра Engine)
    struct ScreenPosition : _Vector2D_Screen
    {
        ScreenPosition() noexcept {}
        ScreenPosition(const ScreenPosition& val) noexcept : _Vector2D_Screen(val) {}
        explicit ScreenPosition(basetype val) noexcept : _Vector2D_Screen(val) {}
        ScreenPosition(basetype _x, basetype _y) noexcept : _Vector2D_Screen(_x, _y) {}
        ScreenPosition(int _x, int _y) noexcept : _Vector2D_Screen(static_cast<basetype>(_x), static_cast<basetype>(_y)) {}
        explicit ScreenPosition(const SpacePosition& val) noexcept { this->operator=(val); }
        explicit ScreenPosition(const DeskPosition& val) noexcept { this->operator=(val); }
        ScreenPosition(const _Vector2D_Screen& val) noexcept : _Vector2D_Screen(val) {}
        ScreenPosition& operator=(basetype val) noexcept { this->_Vector2D_Screen::operator=(val); return *this; }
        ScreenPosition& operator=(const SpacePosition& val) noexcept;
        ScreenPosition& operator=(const DeskPosition& val) noexcept { *this = SpacePosition(val); return *this; }
        explicit operator sf::Vector2f() const { return sf::Vector2f(x, y); }
    };
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
