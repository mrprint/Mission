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
