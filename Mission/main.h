#pragma once

#include <string>
#include "world.h"
#include "pathfinding.h"

void pathFindRequest(const Field &field, int xStart, int yStart, int xFinish, int yFinish);
bool pathReadyCheck();
Path pathRead();
