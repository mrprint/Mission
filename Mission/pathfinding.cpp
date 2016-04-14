// Реализация A-star
// Взято с http://code.activestate.com/recipes/577457-a-star-shortest-path-algorithm/ и адаптировано

#include <queue>

#include "settings.h"
#include "world.h"
#include "pathfinding.h"

using namespace std;

const int map_width = WORLD_DIM; // horizontal size of the map
const int map_height = WORLD_DIM; // vertical size size of the map
//static int map[n][m];
static int closed_nodes_map[map_width][map_height]; // map of closed (tried-out) nodes
static int open_nodes_map[map_width][map_height]; // map of open (not-yet-tried) nodes
static int dir_map[map_width][map_height]; // map of directions
const int dir = 8; // number of possible directions to go at any position
                   // if dir==4
                   //static int dx[dir]={1, 0, -1, 0};
                   //static int dy[dir]={0, 1, 0, -1};
                   // if dir==8
static int dx[dir] = { 1, 1, 0, -1, -1, -1, 0, 1 };
static int dy[dir] = { 0, 1, 1, 1, 0, -1, -1, -1 };

class node
{
    // current position
    int xPos;
    int yPos;
    // total distance already travelled to reach the node
    int level;
    // priority=level+remaining distance estimate
    int priority;  // smaller: higher priority

public:
    node(int xp, int yp, int d, int p)
    {
        xPos = xp; yPos = yp; level = d; priority = p;
    }

    int getxPos() const { return xPos; }
    int getyPos() const { return yPos; }
    int getLevel() const { return level; }
    int getPriority() const { return priority; }

    void updatePriority(const int & xDest, const int & yDest)
    {
        priority = level + estimate(xDest, yDest) * 10; //A*
    }

    // give better priority to going strait instead of diagonally
    void nextLevel(const int & i) // i: direction
    {
        level += (dir == 8 ? (i % 2 == 0 ? 10 : 14) : 10);
    }

    // Estimation function for the remaining distance to the goal.
    const int & estimate(const int & xDest, const int & yDest) const
    {
        static int xd, yd, d;
        xd = xDest - xPos;
        yd = yDest - yPos;

        // Euclidian Distance
        d = static_cast<int>(sqrt(float(xd*xd) + float(yd*yd)));

        // Manhattan distance
        //d=abs(xd)+abs(yd);

        // Chebyshev distance
        //d=max(abs(xd), abs(yd));

        return(d);
    }
};

// Determine priority (in the priority queue)
bool operator<(const node & a, const node & b)
{
    return a.getPriority() > b.getPriority();
}

// A-star algorithm.
// The route returned is a string of direction digits.
string pathFind(const Field &field, int xStart, int yStart, int xFinish, int yFinish)
{
    static priority_queue<node> pq[2]; // list of open (not-yet-tried) nodes
    static int pqi; // pq index
    static node* n0;
    static node* m0;
    static int i, j, x, y, xdx, ydy;
    static char c;
    pqi = 0;

    // reset the node maps
    for (y = 0;y < map_height;y++)
    {
        for (x = 0;x < map_width;x++)
        {
            closed_nodes_map[x][y] = 0;
            open_nodes_map[x][y] = 0;
        }
    }

    // create the start node and push into list of open nodes
    n0 = new node(xStart, yStart, 0, 0);
    n0->updatePriority(xFinish, yFinish);
    pq[pqi].push(*n0);
    open_nodes_map[xStart][yStart] = n0->getPriority(); // mark it on the open nodes map
    delete n0;

    // A* search
    while (!pq[pqi].empty())
    {
        // get the current node w/ the highest priority
        // from the list of open nodes
        n0 = new node(pq[pqi].top().getxPos(), pq[pqi].top().getyPos(),
            pq[pqi].top().getLevel(), pq[pqi].top().getPriority());

        x = n0->getxPos(); y = n0->getyPos();

        pq[pqi].pop(); // remove the node from the open list
        open_nodes_map[x][y] = 0;
        // mark it on the closed nodes map
        closed_nodes_map[x][y] = 1;

        // quit searching when the goal state is reached
        //if((*n0).estimate(xFinish, yFinish) == 0)
        if (x == xFinish && y == yFinish)
        {
            // generate the path from finish to start
            // by following the directions
            string path = "";
            while (!(x == xStart && y == yStart))
            {
                j = dir_map[x][y];
                c = '0' + (j + dir / 2) % dir;
                path = c + path;
                x += dx[j];
                y += dy[j];
            }

            // garbage collection
            delete n0;
            // empty the leftover nodes
            while (!pq[pqi].empty()) pq[pqi].pop();
            return path;
        }

        // generate moves (child nodes) in all possible directions
        for (i = 0;i < dir;i++)
        {
            xdx = x + dx[i]; ydy = y + dy[i];

            if (!(xdx<0 || xdx>map_width - 1 || ydy<0 || ydy>map_height - 1 || static_cast<Field>(field).cells[ydy][xdx].attribs.count(Cell::atrOBSTACLE) > 0
                || closed_nodes_map[xdx][ydy] == 1))
            {
                // generate a child node
                m0 = new node(xdx, ydy, n0->getLevel(),
                    n0->getPriority());
                m0->nextLevel(i);
                m0->updatePriority(xFinish, yFinish);

                // if it is not in the open list then add into that
                if (open_nodes_map[xdx][ydy] == 0)
                {
                    open_nodes_map[xdx][ydy] = m0->getPriority();
                    pq[pqi].push(*m0);
                    // mark its parent node direction
                    dir_map[xdx][ydy] = (i + dir / 2) % dir;
                }
                else if (open_nodes_map[xdx][ydy] > m0->getPriority())
                {
                    // update the priority info
                    open_nodes_map[xdx][ydy] = m0->getPriority();
                    // update the parent direction info
                    dir_map[xdx][ydy] = (i + dir / 2) % dir;

                    // replace the node
                    // by emptying one pq to the other one
                    // except the node to be replaced will be ignored
                    // and the new node will be pushed in instead
                    while (!(pq[pqi].top().getxPos() == xdx &&
                        pq[pqi].top().getyPos() == ydy))
                    {
                        pq[1 - pqi].push(pq[pqi].top());
                        pq[pqi].pop();
                    }
                    pq[pqi].pop(); // remove the wanted node

                                   // empty the larger size pq to the smaller one
                    if (pq[pqi].size() > pq[1 - pqi].size()) pqi = 1 - pqi;
                    while (!pq[pqi].empty())
                    {
                        pq[1 - pqi].push(pq[pqi].top());
                        pq[pqi].pop();
                    }
                    pqi = 1 - pqi;
                    pq[pqi].push(*m0); // add the better node instead
                }
                delete m0; // garbage collection
            }
        }
        delete n0; // garbage collection
    }
    return ""; // no route found
}

void pathDirection(int *x, int *y, char code)
{
    int j = code - '0';
    *x = dx[j];
    *y = dy[j];
}