#pragma once

#ifdef _DEBUG
//#include <vld.h>
#endif

// Отрисовка полностью адаптируется под настройки разрешения, размера поля и отступов от границы экрана

constexpr auto TITLE = "The Mission";

constexpr auto SCREEN_W = 1024;
constexpr auto SCREEN_H = 768;
constexpr auto WORLD_DIM = 30; // Размерность поля
constexpr auto _LC_OFST = 8; // Желаемый отступ левого угла комнаты от края экрана

constexpr auto CELL_W = 2.0f / WORLD_DIM;
constexpr auto CELL_HW = CELL_W / 2.0f;
constexpr auto U_SIZE = CELL_HW * 0.66f;

constexpr auto MAX_TIME_FRACT = 1.0f / 15;
constexpr auto CHAR_B_SPEED = 2.0f / WORLD_DIM * 2.0f;
constexpr auto ART_COUNT = 4;
constexpr auto ART_B_SPEED = 2.0f / WORLD_DIM * 4.0f;
constexpr auto ART_B_DELAY = 5.0f;
constexpr auto ART_DEV = 0.5f;
constexpr auto GUARD_B_SPEED = 2.0f / WORLD_DIM * 2.0f;
constexpr auto LEVEL_COMPL = 0.2f;

constexpr auto BANNER_TOUT = 3.0f;

constexpr auto F_EPSILON = 1e-7f;

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
