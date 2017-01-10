#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "world.hpp"
#include "spaces.hpp"

// Вспомогательный поток расчета пути
// Изначально был задействован для преодоления на скорую руку сверхнизкой производительности
// использовавшейся имплементации поиска пути, а сейчас оставлен "на всякий случай"
class Coworker
{
    std::atomic<unsigned> flags;
    std::mutex mp_mutex;
    std::condition_variable start_cond;
    std::thread w_thread;

    const Field *field;
    DeskPosition start_p, finish_p;
    Path path;

public:

    enum Flags {
        cwREADY = 1,
        cwSTART = 2,
        cwDONE = 4
    };

    Coworker() : field(nullptr) { flags.store(cwREADY); }
    void start();
    void stop();
    void flags_set(unsigned _flags) { flags.fetch_or(_flags); }
    void flags_clear(unsigned _flags) { flags.fetch_and(~_flags); }
    bool flags_get(unsigned _flags) { return (flags.load() & _flags) == _flags; }
    // Запрос на расчёт пути
    void path_find_request(const Field&, DeskPosition, DeskPosition);
    // Получение результата
    const Path& path_read() const { return path; }

private:

    // Тело потока
    // Реализованная механика не учитывает возможные изменения на поле во время расчета, 
    // поэтому они должны явно блокироваться на соответствующих участках
    void body();
    void start_wait();
};

extern Coworker the_coworker;
