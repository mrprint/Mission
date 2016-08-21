#include <string>
#include <time.h>
#include <SFML/System.hpp>
#include "main.h"
#include "settings.h"
#include "engine.h"
#include "world.h"
#include "pathfinding.h"

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
    for (bool done = false; !done; sf::sleep(sf::microseconds(POLL_GRANULARITY)))
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
    the_state = gsINPROGRESS;
    level = 0;
    world_setup();
    engine.work_do();
    the_coworker.stop();
    lists_clear();
    return 0;
}
