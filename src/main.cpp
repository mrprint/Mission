#include "settings.hpp"
#include <string>
#include <time.h>
#include <SFML/System.hpp>
#include "main.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

static const int POLL_GRANULARITY = 1000 / 240; // Четверть 60Гц кадра

Coworker the_coworker;

static FieldsAStar a_star;

////////////////////////////////////////////////////////////////////////////////

void Coworker::start()
{
    thread.launch();
}

void Coworker::stop()
{
    flags_set(cwSTART | cwDONE);
    thread.wait();
}

// Работу с флагами лучше было бы сделать на атомарных операциях, но испоьлзован более универсальный способ

void Coworker::flags_set(unsigned _flags)
{
    mutex.lock();
    flags |= _flags;
    mutex.unlock();
}

void Coworker::flags_clear(unsigned _flags)
{
    mutex.lock();
    flags &= ~_flags;
    mutex.unlock();
}

bool Coworker::flags_get(unsigned _flags)
{
    bool result;
    mutex.lock();
    result = (flags & _flags) == _flags;
    mutex.unlock();
    return result;
}

void Coworker::path_find_request(const Field &_field, DeskPosition st, DeskPosition fn)
{
    if (flags_get(cwREADY))
    {
        field = &_field;
        start_p = st;
        finish_p = fn;
        flags_clear(cwREADY);
    }
    flags_set(cwSTART);
}

const Path& Coworker::path_read()
{
    return path;
}

void Coworker::body()
{
    while (true)
    {
        start_wait();
        if (flags_get(cwDONE))
            break;
        if (!flags_get(cwREADY))
        {
            path.clear();
            a_star.search_ofs(&path, *field, start_p, finish_p);
            flags_set(cwREADY);
        }
    }
}

void Coworker::start_wait()
{
    for (bool done = false; !done; sf::sleep(sf::milliseconds(POLL_GRANULARITY)))
    {
        mutex.lock();
        if (flags & static_cast<unsigned>(cwSTART))
        {
            flags &= ~static_cast<unsigned>(cwSTART);
            done = true;
        }
        mutex.unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////

int main()
{
    srand(static_cast<unsigned>(time(NULL)));
    the_coworker.start();
    the_world.setup();
    the_engine.work_do();
    the_coworker.stop();
    the_world.lists_clear();
    return 0;
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
