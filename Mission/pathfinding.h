#pragma once

#include <vector>
#include <queue>

template<
    size_t H, size_t W, // Размерность карты
    typename TCoords, // Тип координат, предоставляющий члены "x" и "y". Со знаком.
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
        unsigned char ofsx;
        unsigned char ofsy;
        unsigned char state;
    };

    Attributes *attrs;

    struct AttrsPtr // Координаты и ссылка на атрибуты
    {
        Attributes *pa;
        TCoords pos;
        AttrsPtr() { ; }
        AttrsPtr(const TCoords& p, Attributes *attrs) { pos = p; pa = &(attrs[index2d(pos.x, pos.y)]); }
        inline bool operator> (const AttrsPtr& r) const { return r.pa->fscore < pa->fscore; }
    };

    std::priority_queue<AttrsPtr, std::vector<AttrsPtr>, std::greater<AttrsPtr>> opened;
    std::vector<AttrsPtr> temp_buff; // Для переупорядочивания

public:

    AStar() { temp_buff.reserve(H + W); attrs = new Attributes[H * W]; }
    ~AStar() { delete[] attrs; }

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
        { { -1, -1, 14 },{ 0, -1, 10 },{ 1, -1, 14 },{ -1, 0, 10 },{ 1, 0, 10 },{ -1, 1, 14 },{ 0, 1, 10 },{ 1, 1, 14 } };
        memset(attrs, 0, sizeof(Attributes) * H * W);
        while (!opened.empty()) opened.pop();

        AttrsPtr current = opened_push(start_p, cost_estimate(start_p, finish_p));
        while (!opened.empty())
        {
            current = opened_pop();
            if (current.pos.x == finish_p.x && current.pos.y == finish_p.y)
                return true;
            for (int i = 0; i < 8; ++i)
            {
                int dx = dirs[i].x;
                int dy = dirs[i].y;
                TCoords npos;
                npos.x = current.pos.x + dx;
                npos.y = current.pos.y + dy;
                if (!inbound(npos.x, npos.y) || map.isobstacle(npos.x, npos.y))
                    continue;
                size_t ni = index2d(npos.x, npos.y);
                if (attrs[ni].state == st_Closed)
                    continue;
                TWeight t_gscore = current.pa->gscore + dirs[i].d;
                if (attrs[ni].state == st_Wild)
                {
                    opened_push(npos, t_gscore + cost_estimate(npos, finish_p));
                }
                else
                {
                    if (t_gscore >= attrs[ni].gscore)
                        continue;
                    rearrange(&(attrs[ni]), t_gscore + cost_estimate(npos, finish_p));
                }
                attrs[ni].ofsx = dx + 1;
                attrs[ni].ofsy = dy + 1;
                attrs[ni].gscore = t_gscore;
            }
        }
        return false;
    }

    inline AttrsPtr opened_push(const TCoords& p)
    {
        AttrsPtr a(p, attrs); opened.push(a); a.pa->state = st_Opened; return a;
    }

    inline AttrsPtr opened_push(const TCoords& s, TWeight score)
    {
        AttrsPtr a(s, attrs); a.pa->fscore = score; opened.push(a); a.pa->state = st_Opened; return a;
    }

    inline AttrsPtr opened_pop()
    {
        AttrsPtr a = opened.top(); opened.pop(); a.pa->state = st_Closed; return a;
    }

    void rearrange(Attributes *attr, TWeight score)
    {
        AttrsPtr a;
        while (a = opened.top(), opened.pop(), a.pa != attr) temp_buff.push_back(a);
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
            p.x = attrs[ci].ofsx - 1;
            p.y = attrs[ci].ofsy - 1;
            path->push_back(p);
            ci -= W * p.y + p.x;
        }
    }

    void get_path(TPath *path, const TCoords& start_p, const TCoords& finish_p)
    {
        get_path_ofs(path, start_p, finish_p);
        TCoords cp = start_p;
        for (std::vector<TCoords>::reverse_iterator it = path->rbegin(); it != path->rend(); ++it)
        {
            cp.x += it->x;
            cp.y += it->y;
            *it = cp;
        }
    }

    static inline size_t index2d(size_t x, size_t y)
    {
        return y * W + x;
    }

    static inline TWeight cost_estimate(const TCoords& a, const TCoords& b)
    {
        return static_cast<TWeight>(10 * (abs(b.x - a.x) + abs(b.y - a.y)));
    }

    static inline bool inbound(int x, int y)
    {
        return x >= 0 && x < W && y >= 0 && y < H;
    }
};
