#pragma once

#include "world.hpp"

// Вспомогательный объект для расчета пути
// Имитирует взаимодействие с рабочим потоком
class Coworker
{
    unsigned flags;
    Path path;

public:

    enum Flags {
        cwREADY = 1,
        cwSTART = 2,
        cwDONE = 4
    };

    Coworker() : flags(cwREADY) { }
    void start() { }
    void stop() { }
    void flags_set(unsigned _flags) { flags |= _flags; }
    void flags_clear(unsigned _flags) { flags &= ~_flags; }
    bool flags_get(unsigned _flags) { return (flags & _flags) == _flags; }
    // Запрос на расчёт пути
    void path_find_request(const Field&, DeskPosition, DeskPosition);
    // Получение результата
    void path_read(Path& _path) { _path.swap(path); }
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
