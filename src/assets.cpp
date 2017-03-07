#include "settings.hpp"
#include "assets.hpp"

#include "assets.inc"

bool assets_init()
{
    for (size_t i = 0; i < sizeof(the_textures) / sizeof(TextureInfo); ++i)
    {
        if (!the_textures[i].init())
            return false;
    }
    for (size_t i = 0; i < sizeof(the_sprites) / sizeof(SpriteInfo); ++i)
    {
        if (!the_sprites[i].init())
            return false;
    }
    for (size_t i = 0; i < sizeof(the_sounds) / sizeof(SoundInfo); ++i)
    {
        if (!the_sounds[i].init())
            return false;
    }
    return true;
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
