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
