﻿#include <math.h>
#include "settings.h"
#include "engine.h"
#include "spaces.h"

static const float SK45 = 0.7071067812f;
static const float HYP2 = 2.828427125f;

SpacePosition& SpacePosition::operator=(const DeskPosition& val)
{
    *this = SpacePosition(static_cast<float>(val.x + 1), static_cast<float>(val.y + 1)) 
            / (WORLD_DIM / 2.0f) - CELL_HW - 1.0f;
    return *this;
}

SpacePosition& SpacePosition::operator=(const ScreenPosition& val)
{
    SpacePosition room = SpacePosition(engine.sizes.room_w, engine.sizes.room_h);
    *this = (
        (SpacePosition(val.x, val.y) * HYP2 * 2.0f - room * HYP2 
         - SpacePosition(engine.sizes.lc_ofst, engine.sizes.tc_ofst) * HYP2 * 2.0f) / (room * 2.0f)
        ).rotate(SpacePosition(SK45, -SK45));
    return *this;
}

DeskPosition& DeskPosition::operator=(const SpacePosition& val)
{
    SpacePosition t = (val + 1.0f) * (WORLD_DIM / 2.0f);
    x = static_cast<int>(floor(t.x));
    y = static_cast<int>(floor(t.y));
    return *this;
}

ScreenPosition& ScreenPosition::operator=(const SpacePosition& val)
{
    SpacePosition t = 
        SpacePosition(engine.sizes.lc_ofst, engine.sizes.tc_ofst) + (val.rotate(SK45) + HYP2 / 2.0f) 
        * SpacePosition(engine.sizes.room_w, engine.sizes.room_h) / HYP2;
    x = t.x; y = t.y;
    return *this;
}
