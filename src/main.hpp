#pragma once

#include <string>
#include <SFML/System.hpp>
#include "world.hpp"
#include "pathfinding.hpp"
#include "spaces.hpp"

// Вспомогательный поток расчета пути
class Coworker
{
    volatile unsigned flags;
    sf::Mutex mutex;
    sf::Thread thread;

    const Field *field;
    DeskPosition start_p, finish_p;
    Path path;

public:

    enum Flags {
        cwREADY = 1,
        cwSTART = 2,
        cwDONE = 4
    };

    Coworker() : flags(cwREADY), thread(&Coworker::body, this), field(NULL) {}
    void start();
    void stop();
    void flags_set(unsigned);
    void flags_clear(unsigned);
    bool flags_get(unsigned);
    // Запрос на расчёт пути
    void path_find_request(const Field&, DeskPosition, DeskPosition);
    // Получение результата
    const Path& path_read();

private:

    // Тело потока
    // Реализованная механика не учитывает возможные изменения на поле во время расчета, 
    // поэтому они должны явно блокироваться на соответствующих участках
    void body();
    void start_wait();
};

extern Coworker the_coworker;

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
