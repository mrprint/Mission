#include "settings.hpp"
#include <string>
#include "coworker_sync.hpp"
#include "engine.hpp"
#include "world.hpp"
#include "pathfinding.hpp"

using namespace std;

Coworker the_coworker;

static FieldsAStar a_star;

////////////////////////////////////////////////////////////////////////////////

void Coworker::path_find_request(const Field &_field, DeskPosition st, DeskPosition fn)
{
    path.clear();
    a_star.search_ofs(&path, _field, st, fn);
    flags_set(cwREADY);
}
