#pragma once

#include <string>
#include "world.h"
#include "pathfinding.h"

void path_find_request(const Field &field, int xStart, int yStart, int xFinish, int yFinish);
bool path_ready_check();
const Path& path_read();
