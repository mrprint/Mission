#include "settings.hpp"
#include "coworker.hpp"
#include "engine.hpp"
#include "world.hpp"

int main()
{
    the_coworker.start();
    the_world.setup();
    the_engine.work_do();
    the_coworker.stop();
    the_world.lists_clear();
    return 0;
}
