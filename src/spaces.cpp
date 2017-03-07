#include "settings.hpp"
#include <cmath>
#include "engine.hpp"
#include "spaces.hpp"

constexpr auto SC45 = 0.7071067812f;
constexpr auto HYP2 = 2.828427125f;

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
