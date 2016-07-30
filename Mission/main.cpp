#include <windows.h>
#include <process.h>
#include <string>
#include "main.h"
#include "settings.h"
#include "engine.h"
#include "world.h"
#include "pathfinding.h"

struct PathQueryInfo // Структура для межпотокового обмена
{
    Field *field;
    Cell::Coordinates start, finish;
    Path path;
} path_query_info = { NULL, {0, 0}, {0, 0} };

static uintptr_t upThread;
static HANDLE hEventPut;
static CRITICAL_SECTION cs;
static FieldsAStar a_star;
static Engine engine;

static volatile bool done_flag = false; // Неограждающийся, т.к. изменяется только один раз
static volatile bool ready_flag = true;

void ready_flag_set(bool val);

// Функция вспомогательного потока расчета пути
// Реализованная механика не учитывает возможные изменения на поле во время расчета, 
// поэтому они должны явно блокироваться на соответствующих участках
void CalcThread(void* pParams)
{
    //Path path;
    while (true)
    {
        WaitForSingleObject(hEventPut, INFINITE);
        if (done_flag)
            break;
        if (!path_ready_check())
        {
            path_query_info.path.clear();
            a_star.search_ofs(&path_query_info.path, *path_query_info.field, path_query_info.start, path_query_info.finish);
            ready_flag_set(true);
        }
    }
}

int main()
{
    hEventPut = CreateEvent(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&cs);

    rand_gen.seed((unsigned long)(std::time(0)));

    upThread = _beginthread(CalcThread, 0, NULL);
    if (upThread != -1L)
    {
        the_state = gsINPROGRESS;
        level = 0;
        worldSetup();
        engine.work_do();
        // Останавливаем вспомогательный поток
        done_flag = true;
        SetEvent(hEventPut);
        WaitForSingleObject(reinterpret_cast<HANDLE>(upThread), INFINITE);
        CloseHandle(hEventPut);
        
        listsClear();
    }
    DeleteCriticalSection(&cs);
    return 0;
}

// Запрос на расчёт пути
void path_find_request(const Field &field, int xStart, int yStart, int xFinish, int yFinish)
{
    if (path_ready_check())
    {
        path_query_info.field = const_cast<Field*>(&field);
        path_query_info.start.x = xStart;
        path_query_info.start.y = yStart;
        path_query_info.finish.x = xFinish;
        path_query_info.finish.y = yFinish;
        ready_flag_set(false);
    }
    SetEvent(hEventPut);
}

// Проверка наличия результата
bool path_ready_check()
{
    bool ready;
    EnterCriticalSection(&cs);
    ready = ready_flag;
    LeaveCriticalSection(&cs);
    return ready;
}

// Огороженная установка флага готовности результата
// true - готов
// false - идёт расчёт
static void ready_flag_set(bool val)
{
    EnterCriticalSection(&cs);
    ready_flag = val;
    LeaveCriticalSection(&cs);
}

// Получение результата
const Path& path_read()
{
    return path_query_info.path;
}
