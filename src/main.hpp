#pragma once

#include <string>
#include <SFML/System.hpp>
#include "world.hpp"
#include "pathfinding.hpp"
#include "spaces.hpp"

// Вспомогательный поток расчета пути
class Coworker
{
    volatile unsigned flags;
    sf::Mutex mutex;
    sf::Thread thread;

    const Field *field;
    DeskPosition start_p, finish_p;
    Path path;

public:

    enum Flags {
        cwREADY = 1,
        cwSTART = 2,
        cwDONE = 4,
    };

    Coworker() : flags(cwREADY), thread(&Coworker::body, this), field(NULL) {}
    void start();
    void stop();
    void flags_set(unsigned);
    void flags_clear(unsigned);
    bool flags_get(unsigned);
    void path_find_request(const Field&, DeskPosition, DeskPosition); // Запрос на расчёт пути
    const Path& path_read(); // Получение результата

private:

    // Тело потока
    // Реализованная механика не учитывает возможные изменения на поле во время расчета, 
    // поэтому они должны явно блокироваться на соответствующих участках
    void body();
    void start_wait();
};

extern Coworker the_coworker;
