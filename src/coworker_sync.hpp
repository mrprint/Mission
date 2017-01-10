#pragma once

#include "world.hpp"

// Вспомогательный объект для расчета пути
// Имитирует взаимодействие с рабочим потоком
class Coworker
{
    unsigned flags;
    Path path;

public:

    enum Flags {
        cwREADY = 1,
        cwSTART = 2,
        cwDONE = 4
    };

    Coworker() : flags(cwREADY) { }
    void start() { }
    void stop() { }
    void flags_set(unsigned _flags) { flags |= _flags; }
    void flags_clear(unsigned _flags) { flags &= ~_flags; }
    bool flags_get(unsigned _flags) { return (flags & _flags) == _flags; }
    // Запрос на расчёт пути
    void path_find_request(const Field&, DeskPosition, DeskPosition);
    // Получение результата
    const Path& path_read() const { return path; }
};

extern Coworker the_coworker;
