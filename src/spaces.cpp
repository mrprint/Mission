#include "settings.hpp"
#include <math.h>
#include "engine.hpp"
#include "spaces.hpp"

static const float SC45 = 0.7071067812f;
static const float HYP2 = 2.828427125f;

SpacePosition& SpacePosition::operator=(const DeskPosition& val)
{
    *this = SpacePosition(static_cast<basetype>(val.x + 1), static_cast<basetype>(val.y + 1))
            / (WORLD_DIM / 2.0f) - CELL_HW - 1.0f;
    return *this;
}

SpacePosition& SpacePosition::operator=(const ScreenPosition& val)
{
    SpacePosition room = SpacePosition(the_engine.sizes.room_w, the_engine.sizes.room_h);
    *this = (
        (SpacePosition(val.x, val.y) * HYP2 * 2.0f - room * HYP2 
         - SpacePosition(the_engine.sizes.lc_ofst, the_engine.sizes.tc_ofst) * HYP2 * 2.0f) / (room * 2.0f)
        ).rotate(SpacePosition(SC45, -SC45));
    return *this;
}

DeskPosition& DeskPosition::operator=(const SpacePosition& val)
{
    SpacePosition t = (val + 1.0f) * (WORLD_DIM / 2.0f);
    x = static_cast<basetype>(floor(t.x));
    y = static_cast<basetype>(floor(t.y));
    return *this;
}

ScreenPosition& ScreenPosition::operator=(const SpacePosition& val)
{
    SpacePosition t = 
        SpacePosition(the_engine.sizes.lc_ofst, the_engine.sizes.tc_ofst) + (val.rotate(SC45) + HYP2 / 2.0f) 
        * SpacePosition(the_engine.sizes.room_w, the_engine.sizes.room_h) / HYP2;
    x = t.x; y = t.y;
    return *this;
}
