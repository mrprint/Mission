#include "settings.hpp"
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include "main.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

using namespace std;

Coworker the_coworker;

static FieldsAStar a_star;

////////////////////////////////////////////////////////////////////////////////

void Coworker::start()
{
    w_thread = thread{ &Coworker::body, this };
}

void Coworker::stop()
{
    {
        unique_lock<mutex> lck(mp_mutex);
        flags_set(cwSTART | cwDONE);
        start_cond.notify_one();
    }
    w_thread.join();
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
    {
        unique_lock<mutex> lck(mp_mutex);
        flags_set(cwSTART);
        start_cond.notify_one();
    }
}

const Path& Coworker::path_read() const
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
    unique_lock<mutex> lck(mp_mutex);
    while (!flags_get(cwSTART)) start_cond.wait(lck);
    flags_clear(cwSTART);
}

////////////////////////////////////////////////////////////////////////////////

int main()
{
    the_coworker.start();
    the_world.setup();
    the_engine.work_do();
    the_coworker.stop();
    the_world.lists_clear();
    return 0;
}
