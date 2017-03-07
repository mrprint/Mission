#pragma once

#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

constexpr auto BKG_SIZE = 1024.0f;
constexpr auto SPR_SIZE = 128.0f;

// Вся информация о текстуре и её инициализации
struct TextureInfo {
    const char *source;
    sf::Texture texture;
    TextureInfo(const char *_source) : source(_source) {}
    bool init();
};

// Вся информация о спрайте и его инициализации
struct SpriteInfo {
    const TextureInfo *texture;
    sf::IntRect txrect;
    sf::Vector2u offset;
    sf::Vector2f spot;
    sf::Sprite sprite;
    SpriteInfo(
        const TextureInfo &_texture,
        int txx, int txy, int txw, int txh,
        unsigned ofsx, unsigned ofsy,
        float spx, float spy
    ) : texture(&_texture), txrect(txx, txy, txw, txh), offset(ofsx, ofsy), spot(spx, spy) {}
    bool init();
};

// Анимация
struct AnimationSequence {

    struct Node {
        SpriteInfo *pic;
        float time;
    };

    using Sequence = std::vector<Node>;

    float length; // длина анимации
    float time; // текущий момент времени
    Sequence sequence;
    Sequence::iterator node; // текущий узел

    AnimationSequence(float);
    void advance(float);
    void time_set(float _time) { advance(time - floor(_time / length) * length); };
    SpriteInfo* sprite_get() const;
};


// Вся информация о звуке и его инициализации
struct SoundInfo {
    const char *source;
    sf::SoundBuffer buffer;
    SoundInfo(const char *source) : source(source) {}
    bool init();
};

bool assets_init();

extern TextureInfo the_textures[];
extern SpriteInfo the_sprites[];
extern SoundInfo the_sounds[];

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
