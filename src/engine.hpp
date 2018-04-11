#pragma once

#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "world.hpp"
#include "spaces.hpp"
#include "mathapp.hpp"

// Воспроизводящиеся звуки
class Orchestre
{
    std::deque<sf::Sound> sounds;
public:
    Orchestre() {}
    void play(const sf::SoundBuffer&);
    void update();
};

struct DrawingSizes
{
    tool::fpoint_fast spr_scale;
    tool::fpoint_fast bkg_scale;
    int screen_w;
    int screen_h;
    int room_w;
    int room_h;
    int lc_ofst; // Реальный отступ левого угла комнаты от края экрана
    int tc_ofst; // Отступ верхнего угла комнаты от края экрана
};

////////////////////////////////////////////////////////////////////////////////

class Engine
{
    enum ControlState {
        csLMBUTTON,
        csRMBUTTON,
        _csEND
    };

    std::unique_ptr<sf::RenderWindow> window;
    sf::Font font;
    std::bitset<_csEND> controls;
    tool::ScreenPosition mouse_p;
    Orchestre played_sounds;
    tool::fpoint_fast banner_timeout;
    bool windowed;
public:
    DrawingSizes sizes;

    Engine();
    void work_do();
private:
    bool init();
    void videomode_set(bool);
    void main_loop();
    void frame_render();
    void input_process();
    void update(sf::Time);
    void sprite_draw(sf::Sprite&, const tool::ScreenPosition&, tool::fpoint_fast);
    void field_draw();
    void path_change(tool::DeskPosition);
    bool cell_flip(tool::DeskPosition);
    void sounds_play();
    void text_print(const tool::ScreenPosition&, unsigned, const std::string&, bool = false);
};

////////////////////////////////////////////////////////////////////////////////

extern Engine the_engine;

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
