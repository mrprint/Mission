#pragma once

#include <hge.h>

// Отрисовка полностью адаптируется под настройки разрешения, размера поля и отступов от границы экрана

const char TITLE[] = "The Mission";

const int SCREEN_W = 1024;
const int SCREEN_H = 768;
const int WORLD_DIM = 30; // Размерность поля
const int _LC_OFST = 8; // Желаемый отступ левого угла комнаты от края экрана

const int TILE_W = (SCREEN_W - (_LC_OFST * 2)) / WORLD_DIM - ((SCREEN_W - (_LC_OFST * 2)) / WORLD_DIM) % 2;
const int TILE_H = TILE_W / 2;
const int TILE_HW = TILE_W / 2;
const int TILE_HH = TILE_H / 2 + (TILE_H / 2) % 2;
const int ROOM_W = TILE_W * WORLD_DIM;
const int ROOM_H = TILE_H * WORLD_DIM;
const int LC_OFST = (SCREEN_W - ROOM_W) / 2; // Реальный отступ левого угла комнаты от края экрана
const int TC_OFST = (SCREEN_H - ROOM_H) / 2; // Отступ верхнего угла комнаты от края экрана
const float CELL_W = 2.0f / WORLD_DIM;
const float CELL_HW = CELL_W / 2.0f;
const float U_SIZE = CELL_HW * 0.66f;

const float MAX_TIME_FRACT = 1.0f / 15;
const float CHAR_B_SPEED = 2.0f / WORLD_DIM * 2.0f;
const int ART_COUNT = 4;
const float ART_B_POSSIB = 0.13f;
const float ART_B_SPEED = 2.0f / WORLD_DIM * 4.0f;
const float ART_B_DELAY = 5.0f;
const float ART_DEV = 0.5f;
const float GUARD_B_SPEED = 2.0f / WORLD_DIM * 2.0f;
const float LEVEL_COMPL = 0.2f;

const float BANNER_TOUT = 3.0f;

static inline float to_space_dim(int v)
{
	return float(v + 1) / WORLD_DIM * 2.0f - CELL_HW - 1.0f;
}
