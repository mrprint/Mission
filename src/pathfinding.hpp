#pragma once

#include <string.h>
#include <cmath>
#include <functional>
#include <vector>
#include <queue>
#include <memory>

template<
    size_t H, size_t W, // Размерность карты
    typename TCoords, // Тип координат, предоставляющий члены "x" и "y". Со знаком
    typename TMap, // Карта. Предоставляет "isobstacle(x, y)"
    typename TWeight = int, // Тип веса
    typename TPath = std::vector<TCoords> // Возвращаемый путь, предоставляющий push_back
>
class AStar
{
protected:

    enum State {
        st_Wild = 0,
        st_Opened,
        st_Closed
    };

    struct Attributes // Атрибуты позиции
    {
        TWeight fscore, gscore;
        char ofsx;
        char ofsy;
        unsigned char state;
    };

    std::unique_ptr<Attributes[]> attrs;

    struct AttrsPtr // Координаты и ссылка на атрибуты
    {
        Attributes *pa;
        TCoords pos;
        AttrsPtr() noexcept {}
        AttrsPtr(const TCoords& p, Attributes *attrs) noexcept { pos = p; pa = &attrs[index2d(pos.x, pos.y)]; }
        bool operator> (const AttrsPtr& r) const { return r.pa->fscore < pa->fscore; }
    };

    std::priority_queue<AttrsPtr, std::vector<AttrsPtr>, std::greater<AttrsPtr> > opened;
    std::vector<AttrsPtr> temp_buff; // Для переупорядочивания

public:

    AStar() : attrs(new Attributes[H * W]) { temp_buff.reserve(H + W); }

    // Получить смещения (в обратном порядке)
    bool search_ofs(TPath *path, const TMap& map, const TCoords& start_p, const TCoords& finish_p)
    {
        if (!do_search(map, start_p, finish_p))
            return false;
        get_path_ofs(path, start_p, finish_p);
        return true;
    }

    // Получить абсолютные координаты (в обратном порядке)
    bool search(TPath *path, const TMap& map, const TCoords& start_p, const TCoords& finish_p)
    {
        if (!do_search(map, start_p, finish_p))
            return false;
        get_path(path, start_p, finish_p);
        return true;
    }

protected:

    bool do_search(const TMap& map, const TCoords& start_p, const TCoords& finish_p)
    {
        static struct { int x, y, d; } dirs[] =
        { { -1, -1, 19 },{ 0, -1, 10 },{ 1, -1, 19 },{ -1, 0, 10 },{ 1, 0, 10 },{ -1, 1, 19 },{ 0, 1, 10 },{ 1, 1, 19 } };
        memset(attrs.get(), 0, sizeof(Attributes) * H * W);
        while (!opened.empty()) opened.pop();

        AttrsPtr current = opened_push(start_p, cost_estimate(start_p, finish_p));
        while (!opened.empty())
        {
            current = opened_pop();
            if (current.pos.x == finish_p.x && current.pos.y == finish_p.y)
                return true;
            for (int i = 0; i < 8; ++i)
            {
                auto dx = dirs[i].x;
                auto dy = dirs[i].y;
                TCoords npos;
                npos.x = current.pos.x + dx;
                npos.y = current.pos.y + dy;
                if (!inbound(npos.x, npos.y) || map.isobstacle(npos.x, npos.y))
                    continue;
                auto ni = index2d(npos.x, npos.y);
                if (attrs[ni].state == st_Closed)
                    continue;
                TWeight t_gscore = current.pa->gscore + dirs[i].d;
                if (attrs[ni].state == st_Wild)
                {
                    opened_push(npos, t_gscore + cost_estimate(npos, finish_p));
                } else
                {
                    if (t_gscore >= attrs[ni].gscore)
                        continue;
                    rearrange(&attrs[ni], t_gscore + cost_estimate(npos, finish_p));
                }
                attrs[ni].ofsx = dx;
                attrs[ni].ofsy = dy;
                attrs[ni].gscore = t_gscore;
            }
        }
        return false;
    }

    AttrsPtr opened_push(const TCoords& p)
    {
        AttrsPtr a(p, attrs.get()); opened.push(a); a.pa->state = st_Opened; return a;
    }

    AttrsPtr opened_push(const TCoords& s, TWeight score)
    {
        AttrsPtr a(s, attrs.get()); a.pa->fscore = score; opened.push(a); a.pa->state = st_Opened; return a;
    }

    AttrsPtr opened_pop()
    {
        AttrsPtr a = opened.top(); opened.pop(); a.pa->state = st_Closed; return a;
    }

    void rearrange(Attributes *attr, TWeight score)
    {
        AttrsPtr a;
        while ((a = opened.top()), opened.pop(), (a.pa != attr)) temp_buff.push_back(a);
        a.pa->fscore = score;
        opened.push(a);
        while (!temp_buff.empty())
        {
            opened.push(temp_buff.back());
            temp_buff.pop_back();
        }
    }

    void get_path_ofs(TPath *path, const TCoords& start_p, const TCoords& finish_p)
    {
        size_t ci = index2d(finish_p.x, finish_p.y);
        size_t si = index2d(start_p.x, start_p.y);
        while (ci != si)
        {
            TCoords p;
            p.x = attrs[ci].ofsx;
            p.y = attrs[ci].ofsy;
            path->push_back(p);
            ci -= W * p.y + p.x;
        }
    }

    void get_path(TPath *path, const TCoords& start_p, const TCoords& finish_p)
    {
        get_path_ofs(path, start_p, finish_p);
        TCoords cp = start_p;
        for (typename std::vector<TCoords>::reverse_iterator it = path->rbegin(); it != path->rend(); ++it)
        {
            cp.x += it->x;
            cp.y += it->y;
            *it = cp;
        }
    }

    static size_t index2d(size_t x, size_t y)
    {
        return y * W + x;
    }

    static TWeight cost_estimate(const TCoords& a, const TCoords& b)
    {
        TCoords dt; dt.x = b.x - a.x; dt.y = b.y - a.y;
        return static_cast<TWeight>(10 * (dt.x * dt.x + dt.y * dt.y));
    }

    static bool inbound(int x, int y)
    {
        return x >= 0 && x < static_cast<int>(W) && y >= 0 && y < static_cast<int>(H);
    }
};

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
