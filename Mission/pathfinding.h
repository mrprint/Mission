#pragma once

#include <string>
#include "world.h"

std::string pathFind(const Field &field, int xStart, int yStart, int xFinish, int yFinish);
void pathDirection(int*, int*, char);
