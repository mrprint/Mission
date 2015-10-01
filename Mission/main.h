#pragma once

#include <string>
#include "world.h"

void pathFindRequest(const Field &field, int xStart, int yStart, int xFinish, int yFinish);
bool pathReadyCheck();
std::string pathRead();
