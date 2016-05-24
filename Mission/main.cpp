#include <windows.h>
#include <process.h>
#include <ctime>
#include <stdlib.h>
#include <string>
#include "main.h"
#include "settings.h"
#include "scene.h"
#include "world.h"
#include "pathfinding.h"

struct PathQueryInfo // Структура для межпотокового обмена
{
    Field *field;
    Cell::Coordinates start, finish;
    Path path;
} path_query_info = { NULL, {0, 0}, {0, 0} };

uintptr_t upThread;
HANDLE hEventPut;
CRITICAL_SECTION cs;
FieldsAStar a_star;

volatile bool done_flag = false; // Неограждаемый, т.к. изменяется только один раз
volatile bool ready_flag = true;

void ready_flag_set(bool val);
std::string expand_environment_variables(std::string);

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
        if (!pathReadyCheck())
        {
            a_star.search_ofs(&path_query_info.path, *path_query_info.field, path_query_info.start, path_query_info.finish);
            //path_query_info.path = path;
            ready_flag_set(true);
        }
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    hEventPut = CreateEvent(NULL, FALSE, FALSE, NULL);
    InitializeCriticalSection(&cs);

    hge = hgeCreate(HGE_VERSION);
    hge->System_SetState(HGE_LOGFILE, expand_environment_variables("${USERPROFILE}\\hge_mission.log").c_str());
    hge->System_SetState(HGE_FRAMEFUNC, FrameFunc);
    hge->System_SetState(HGE_RENDERFUNC, RenderFunc);
    hge->System_SetState(HGE_TITLE, TITLE);
    hge->System_SetState(HGE_WINDOWED, true);
    hge->System_SetState(HGE_HIDEMOUSE, false);
    hge->System_SetState(HGE_SCREENWIDTH, SCREEN_W);
    hge->System_SetState(HGE_SCREENHEIGHT, SCREEN_H);
    hge->System_SetState(HGE_SCREENBPP, 32);
    hge->System_SetState(HGE_USESOUND, true);
    hge->System_SetState(HGE_DONTSUSPEND, true);

    rand_gen.seed((unsigned long)(std::time(0)));

    upThread = _beginthread(CalcThread, 0, NULL);
    if (hge->System_Initiate() && upThread != -1L)
    {
        the_state = gsINPROGRESS;
        level = 0;
        worldSetup();
        sceneSetup();
        hge->System_Start();
        // Останавливаем вспомогательный поток
        done_flag = true;
        SetEvent(hEventPut);
        WaitForSingleObject(reinterpret_cast<HANDLE>(upThread), INFINITE);
        CloseHandle(hEventPut);
        
        sceneFree();
        listsClear();
    }
    else
    {
        MessageBox(NULL, hge->System_GetErrorMessage(), "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
    }

    hge->System_Shutdown();
    hge->Release();

    DeleteCriticalSection(&cs);
    return 0;
}

// Запрос на расчёт пути
void pathFindRequest(const Field &field, int xStart, int yStart, int xFinish, int yFinish)
{
    if (pathReadyCheck())
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
bool pathReadyCheck()
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
Path pathRead()
{
    return path_query_info.path;
}

// Универсальная распаковка переменных системы в пути
// Найдено в интернете и адаптировано
static std::string expand_environment_variables(std::string s)
{
    using namespace std;
    if (s.find("${") == string::npos)
        return s;
    string pre = s.substr(0, s.find("${"));
    string post = s.substr(s.find("${") + 2);
    if (post.find('}') == string::npos)
        return s;
    string variable = post.substr(0, post.find('}'));
    string value = "";
    post = post.substr(post.find('}') + 1);
    char *env = getenv(variable.c_str());
    if (env != NULL)
        value = string(env);
    return expand_environment_variables(pre + value + post);
}