#include "settings.hpp"
#include <thread>
#include <mutex>
#include "coworker_async.hpp"
#include "world.hpp"

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
