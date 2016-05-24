#include <typeinfo.h>
#include <algorithm>
#include <vector>
#include <hge.h>
#include <hgefont.h>
#include <hgeresource.h>
#include "settings.h"
#include "scene.h"
#include "world.h"
#include "main.h"

const float BKG_SIZE = 1024.0f;
const float SPR_SIZE = 128.0f;
const float TILE_HOT = SPR_SIZE * 0.75f;
const float SPR_SCALE = TILE_W / SPR_SIZE;
const float SK45 = 0.7071067812f;
const float HYP2 = 2.828427125f;

// Юнит с рассчитанным расположением на экране
struct ScreenPos {
    float x, y;
    Unit *unit;
    bool operator<(const ScreenPos &sp) { return (y < sp.y); }
};

typedef std::vector<ScreenPos> ScreenPositions; // Список расположения юнитов

HGE *hge = 0;
float banner_timeout = 0.0f;

hgeResourceManager *game_res;
HEFFECT shothe, hithe, luphe;
HTEXTURE bkg, tile, lwall, rwall, lbatt, rbatt, fball, exitp, lguard, rguard, chart, chartt;
hgeSprite *bgspr, *tlspr, *lwspr, *rwspr, *lbspr, *rbspr, *fbspr, *epspr, *lgspr, *rgspr, *chspr, *chtspr;
hgeFont *fnt;

void tileDraw(hgeSprite*, int, int, float);
void unitDraw(hgeSprite*, const SpacePosition&, float);
void fieldDraw();
void pathChange(float, float);
bool cellFlip(float, float);
void soundsPlay();

// Трансформация пространственных координат в экранные
static inline void space_to_screen(float *x, float *y, const SpacePosition &position)
{
    float xsk = position.x * SK45;
    float ysk = position.y * SK45;
    float tx = xsk - ysk;
    float ty = xsk + ysk;
    *x = LC_OFST + (HYP2 / 2.0f + tx) * ROOM_W / HYP2;
    *y = TC_OFST + (HYP2 / 2.0f + ty) * ROOM_H / HYP2;
}

// Трансформация экранных координат в пространственные
static inline void screen_to_space(SpacePosition *position, float x, float y)
{
    float k = 2.0f * ROOM_H * ROOM_W * SK45;
    position->x = -(-x * HYP2 * ROOM_H + HYP2 * LC_OFST * ROOM_H + HYP2 * ROOM_W * (TC_OFST + ROOM_H - y)) / k;
    position->y = -(x * HYP2 * ROOM_H - HYP2 * LC_OFST * ROOM_H + HYP2 * ROOM_W * (TC_OFST - y)) / k;
}

// Выбор ячейки по экранным координатам
static inline void screen_to_field(int *fx, int *fy, float sx, float sy)
{
    SpacePosition position;
    screen_to_space(&position, sx, sy);
    Field::pos_to_cell(fx, fy, position);
}

////////////////////////////////////////////////////////////////////////////////
bool sceneSetup()
{
    bkg = hge->Texture_Load("resources\\skybkg.png");
    tile = hge->Texture_Load("resources\\tile.png");
    lwall = hge->Texture_Load("resources\\lwall.png");
    rwall = hge->Texture_Load("resources\\rwall.png");
    lbatt = hge->Texture_Load("resources\\lbatt.png");
    rbatt = hge->Texture_Load("resources\\rbatt.png");
    fball = hge->Texture_Load("resources\\fireball.png");
    exitp = hge->Texture_Load("resources\\exit.png");
    lguard = hge->Texture_Load("resources\\lguard.png");
    rguard = hge->Texture_Load("resources\\rguard.png");
    chart = hge->Texture_Load("resources\\character.png");
    chartt = hge->Texture_Load("resources\\character_t.png");

    shothe = hge->Effect_Load("resources\\shot.wav");
    hithe = hge->Effect_Load("resources\\hit.wav");
    luphe = hge->Effect_Load("resources\\gong.wav");
    if (!bkg || !tile || !lwall || !rwall || !lbatt || !rbatt || !fball || !exitp || !lguard || !rguard || !chart || !chartt
        || !shothe || !hithe || !luphe)
    {
        MessageBox(NULL, "Can't load resources", "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
        hge->System_Shutdown();
        hge->Release();
        return 0;
    }

    bgspr = new hgeSprite(bkg, 0, 0, BKG_SIZE, BKG_SIZE);
    bgspr->SetBlendMode(BLEND_ALPHABLEND | BLEND_NOZWRITE);
    tlspr = new hgeSprite(tile, 0, 0, SPR_SIZE, SPR_SIZE);
    tlspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    lwspr = new hgeSprite(lwall, 0, 0, SPR_SIZE, SPR_SIZE);
    lwspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    rwspr = new hgeSprite(rwall, 0, 0, SPR_SIZE, SPR_SIZE);
    rwspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    lbspr = new hgeSprite(lbatt, 0, 0, SPR_SIZE, SPR_SIZE);
    lbspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    rbspr = new hgeSprite(rbatt, 0, 0, SPR_SIZE, SPR_SIZE);
    rbspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    fbspr = new hgeSprite(fball, 0, 0, SPR_SIZE, SPR_SIZE);
    fbspr->SetHotSpot(SPR_SIZE / 2, SPR_SIZE);
    epspr = new hgeSprite(exitp, 0, 0, SPR_SIZE, SPR_SIZE);
    epspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    lgspr = new hgeSprite(lguard, 0, 0, SPR_SIZE, SPR_SIZE);
    lgspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    rgspr = new hgeSprite(rguard, 0, 0, SPR_SIZE, SPR_SIZE);
    rgspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    chspr = new hgeSprite(chart, 0, 0, SPR_SIZE, SPR_SIZE);
    chspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    chtspr = new hgeSprite(chartt, 0, 0, SPR_SIZE, SPR_SIZE);
    chtspr->SetHotSpot(SPR_SIZE / 2, TILE_HOT);
    fnt = new hgeFont("resources\\arial.fnt");
    fnt->SetColor(0xFFFF0010);

    return true;
}

void sceneFree()
{
    delete fnt;
    delete chtspr;
    delete chspr;
    delete rgspr;
    delete lgspr;
    delete epspr;
    delete fbspr;
    delete rbspr;
    delete lbspr;
    delete rwspr;
    delete lwspr;
    delete tlspr;
    delete bgspr;
    hge->Effect_Free(luphe);
    hge->Effect_Free(hithe);
    hge->Effect_Free(shothe);
    hge->Texture_Free(chartt);
    hge->Texture_Free(chart);
    hge->Texture_Free(rguard);
    hge->Texture_Free(lguard);
    hge->Texture_Free(exitp);
    hge->Texture_Free(fball);
    hge->Texture_Free(rbatt);
    hge->Texture_Free(lbatt);
    hge->Texture_Free(rwall);
    hge->Texture_Free(lwall);
    hge->Texture_Free(tile);
    hge->Texture_Free(bkg);

}

////////////////////////////////////////////////////////////////////////////////
bool FrameFunc()
{
    float mx, my;

    // Для антифликинга
    static bool lb_down = false;
    static bool rb_down = false;

    float dt = hge->Timer_GetDelta();
    if (dt > MAX_TIME_FRACT)
        dt = MAX_TIME_FRACT; // Замедляем время, если машина не успевает считать

    if (the_state != gsINPROGRESS)
    {
        // Обработка таймаута вывода баннеров выигрыша/поражения
        if (banner_timeout > 0.0f)
        {
            banner_timeout -= dt;
            if (banner_timeout <= 0.0f)
            {
                // Снимаем баннер и настраиваем уровень
                the_state = gsINPROGRESS;
                worldSetup();
            }
        }
        else
        {
            // Показываем баннер и выполняем базовые настройки при по смене состояния
            banner_timeout = BANNER_TOUT;
            switch (the_state)
            {
            case gsLOSS:
                the_sounds.push_back(seHIT);
                level = 0;
                break;
            case gsWIN:
                the_sounds.push_back(seLVLUP);
                ++level;
                break;
            }
        }
    }
    else
    {
        stateCheck(); // Оцениваем состояние игры
    }

    if (hge->Input_GetKeyState(HGEK_ESCAPE)) return true;
    if (hge->Input_GetKeyState(HGEK_LBUTTON) && the_state == gsINPROGRESS)
    {
        if (!the_character->path_requested && pathReadyCheck() && !lb_down)
        {
            // Будем идти в указанную позицию
            hge->Input_GetMousePos(&mx, &my);
            pathChange(mx, my);
            lb_down = true;
        }
    }
    else
        lb_down = false;
    if (hge->Input_GetKeyState(HGEK_RBUTTON) && the_state == gsINPROGRESS)
    {

        if (!the_character->path_requested && pathReadyCheck() && !rb_down)
        {
            // Пытаемся изменить состояние ячейки "свободна"/"препятствие"
            hge->Input_GetMousePos(&mx, &my);
            if (cellFlip(mx, my))
            {
                // При необходимости обсчитываем изменения пути
                if (the_character->way.path.size() > 0)
                    the_character->way_new_request(the_character->way.target.x, the_character->way.target.y);
            }
            rb_down = true;
        }
    }
    else
        rb_down = false;
    if (the_character->path_requested && pathReadyCheck())
    {
        the_character->path_requested = false;
        the_character->way_new_process();
    }
    moveDo(dt); // Рассчитываем изменения
    soundsPlay(); // Воспроизводим звуки
    return false;
}

////////////////////////////////////////////////////////////////////////////////
bool RenderFunc()
{
    hge->Gfx_BeginScene();
    bgspr->RenderStretch(0, 0, float(SCREEN_W), float(SCREEN_H));
    fieldDraw();
    // Отображаем игровую информацию
    fnt->printf(float(LC_OFST), float(LC_OFST), HGETEXT_LEFT, "Level %d", level + 1);
    switch (the_state)
    {
    case gsLOSS:
        fnt->printf(float(SCREEN_W / 2), float(SCREEN_H / 2), HGETEXT_CENTER | HGETEXT_MIDDLE, "YOU LOSS!");
        break;
    case gsWIN:
        fnt->printf(float(SCREEN_W / 2), float(SCREEN_H / 2), HGETEXT_CENTER | HGETEXT_MIDDLE, "LEVEL UP!");
        break;
    }
    hge->Gfx_EndScene();
    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Отрисовка тайла в позиции ячейки
static void tileDraw(hgeSprite *spr, int cx, int cy, float scale)
{
    SpacePosition position;
    float x, y;
    Field::cell_to_pos(&position, cx, cy);
    space_to_screen(&x, &y, position);
    spr->RenderEx(x, y, 0.0f, scale, scale);
}

// Отрисовка игрового поля
static void fieldDraw()
{
    // Рисуем пол
    for (int y = 0; y < WORLD_DIM; y++)
        for (int x = 0; x < WORLD_DIM; x++)
        {
            if (the_field.cells[y][x].attribs.count(Cell::atrOBSTACLE) == 0)
                tileDraw(tlspr, x, y, SPR_SCALE);
            if (the_field.cells[y][x].attribs.count(Cell::atrEXIT) > 0)
                tileDraw(epspr, x, y, SPR_SCALE);
        };
    // Рисуем стены
    for (int i = 0; i < WORLD_DIM; i++)
        tileDraw(rwspr, i, 0, SPR_SCALE);
    for (int i = 0; i < WORLD_DIM; i++)
        tileDraw(lwspr, 0, i, SPR_SCALE);
    // Рисуем пушки
    for (Artillery::Settings::iterator it = the_artillery.setting.begin(); it != the_artillery.setting.end(); ++it)
    {
        if (abs(it->speed.x) < FLT_EPSILON)
            tileDraw(rbspr, it->position.x, it->position.y, SPR_SCALE);
        else
            tileDraw(lbspr, it->position.x, it->position.y, SPR_SCALE);
    }
    // Заполняем список юнитов с их экранными координатами
    ScreenPositions positions;
    ScreenPos upos;
    for (UnitsList::iterator it = the_alives.begin(); it != the_alives.end(); ++it)
    {
        space_to_screen(&upos.x, &upos.y, (*it)->position);
        upos.unit = *it;
        positions.push_back(upos);
    }
    std::sort(positions.begin(), positions.end()); // Сортируем по экранному y
    // Рисуем юниты от дальних к ближним
    for (ScreenPositions::iterator it = positions.begin(); it != positions.end(); ++it)
    {
        switch (it->unit->id()) {
        case Unit::utCharacter:
            if (the_character->path_requested)
                chtspr->RenderEx(it->x, it->y, 0.0f, SPR_SCALE, SPR_SCALE);
            else
                chspr->RenderEx(it->x, it->y, 0.0f, SPR_SCALE, SPR_SCALE);
            break;
        case Unit::utFireball:
            fbspr->RenderEx(it->x, it->y, 0.0f, SPR_SCALE * 0.5f, SPR_SCALE * 0.5f);
            break;
        case Unit::utGuard:
            (it->unit->speed.x >= 0.0f ? rgspr : lgspr)->RenderEx(it->x, it->y, 0.0f, SPR_SCALE, SPR_SCALE);
        }
    }
}

// Изменение состояния указанной мышкой ячейки
static bool cellFlip(float mx, float my)
{
    int x, y, cx, cy;
    screen_to_field(&x, &y, mx, my);
    Field::pos_to_cell(&cx, &cy, the_character->position);
    if (x < 0 || x >= WORLD_DIM || y < 0 || y >= WORLD_DIM || (x == cx && y == cy))
        return false;
    if (the_field.cells[y][x].attribs.count(Cell::atrOBSTACLE) > 0)
        the_field.cells[y][x].attribs.erase(Cell::atrOBSTACLE);
    else
        the_field.cells[y][x].attribs.insert(Cell::atrOBSTACLE);
    return true;
}

// Ищем новый путь
static void pathChange(float mx, float my)
{
    int x, y;
    screen_to_field(&x, &y, mx, my);
    if (x < 0 || x >= WORLD_DIM || y < 0 || y >= WORLD_DIM)
        return;
    the_character->way_new_request(x, y);
}

static void soundsPlay()
{
    while (!the_sounds.empty())
    {
        switch (the_sounds.front())
        {
        case seSHOT:
            hge->Effect_Play(shothe);
            break;
        case seHIT:
            hge->Effect_Play(hithe);
            break;
        case seLVLUP:
            hge->Effect_Play(luphe);
            break;
        }
        the_sounds.pop_front();
    }
}

