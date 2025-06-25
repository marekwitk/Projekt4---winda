#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <algorithm>
using namespace Gdiplus;

#pragma comment(lib, "gdiplus.lib")

ULONG_PTR gdiplusToken;

struct ButtonInfo {
    int  x, y, width, height;
    int  floor;
    int  number;
    bool prawaStrona;
};
std::vector<ButtonInfo> przyciski;

struct ProstokatAnimowany {
    int   number;
    float x;
    bool  wWindzie = false;
    bool  wychodzi = false;
    int   dirWyjscia = 0;
    int   slot = -1;
};
std::vector<ProstokatAnimowany> prostokatikiNaPietrze[6];

const int   odleglosc = 25;
const float predkoscAnimacji = 4.0f;
const float predkoscWyjscia = 8.0f;
const int   boxWidth = 20;
const int   MAXSLOTS = 8;

int    pozycjaWindy = 1;
int    docelowePietro = 1;
float  aktualnaPozycjaPix = 0;
const int TIMER_ID = 1;

int   stanDrzwi = 0;
bool  drzwiBylyOtwarte = false;
float szerokoscLewegoBoku = 1.0f;
float szerokoscPrawegoBoku = 1.0f;
DWORD czasOtwarciaDrzwi = 0;
float wysokosciPomaranczowychLinii[6] = { 0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

enum KierunekJazdy { IDLE = 0, W_GORE = +1, W_DOL = -1 };
KierunekJazdy trybJazdy = IDLE;

void CompressSlots()
{
    std::vector<ProstokatAnimowany*> inside;
    for (int f = 1; f <= 5; ++f)
        for (auto& p : prostokatikiNaPietrze[f])
            if (p.wWindzie)
                inside.push_back(&p);

    std::sort(inside.begin(), inside.end(),
        [](const ProstokatAnimowany* a, const ProstokatAnimowany* b)
        {
            return a->slot < b->slot;
        });

    for (size_t i = 0; i < inside.size(); ++i)
        inside[i]->slot = static_cast<int>(i);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        int wysokosc_ekranu = rect.bottom;
        double wysokosc_prostokata = 0.95 * wysokosc_ekranu;
        double wysokosc_windy = 0.1 * wysokosc_prostokata;
        double przesuniecie_pietra = 0.15 * wysokosc_prostokata;
        int y = (wysokosc_ekranu - (int)wysokosc_prostokata) / 2;

        float nowaPozycja = y + wysokosc_prostokata - wysokosc_windy
            - docelowePietro * przesuniecie_pietra;

        if (fabs(aktualnaPozycjaPix - nowaPozycja) < 0.1f)
            aktualnaPozycjaPix = nowaPozycja;

        InvalidateRect(hWnd, NULL, TRUE);
        SetTimer(hWnd, TIMER_ID, 16, NULL);
    }
    break;

    case WM_LBUTTONDOWN:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        int szerokosc_ekranu = rect.right;

        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        for (const auto& b : przyciski)
        {
            if (x >= b.x && x <= b.x + b.width &&
                y >= b.y && y <= b.y + b.height)
            {
                float startX = b.prawaStrona
                    ? szerokosc_ekranu
                    : -20.0f;
                prostokatikiNaPietrze[b.floor].push_back(
                    ProstokatAnimowany{ b.number, startX });

                InvalidateRect(hWnd, NULL, FALSE);
                break;
            }
        }
    }
    break;

    case WM_TIMER:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        int wysokosc_ekranu = rect.bottom;
        int szerokosc_ekranu = rect.right;

        double wysokosc_prostokata = 0.95 * wysokosc_ekranu;
        double wysokosc_windy = 0.1 * wysokosc_prostokata;
        double przesuniecie_pietra = 0.15 * wysokosc_prostokata;

        double szerokosc_prostokata = 0.2 * szerokosc_ekranu;
        double szerokosc_windy = 0.95 * szerokosc_prostokata;

        int x1 = (szerokosc_ekranu - (int)szerokosc_windy) / 2;
        int x2 = (szerokosc_ekranu + (int)szerokosc_windy) / 2 + 6;
        int x3 = x1 - 6;
        int y = (wysokosc_ekranu - (int)wysokosc_prostokata) / 2;

     
        float celPix;

        celPix = y + wysokosc_prostokata - wysokosc_windy - docelowePietro * przesuniecie_pietra;

        float predkosc = 5.0f;


        bool kabinaPusta = true;
        for (int f = 1; f <= 5 && kabinaPusta; ++f)
            for (auto& p : prostokatikiNaPietrze[f])
                if (p.wWindzie) { kabinaPusta = false; break; }

     
        if (stanDrzwi == 0 && kabinaPusta)
        {
            static DWORD idleStart = 0;
            auto pix2floor = [&](float pix)->int
                {
                    return (int)round((y + wysokosc_prostokata - wysokosc_windy - pix)
                        / przesuniecie_pietra);
                };
            int floorNow = pix2floor(aktualnaPozycjaPix);

            int foundFloor = 0, bestDist = 99;
            for (int f = 1; f <= 5; ++f)
            {
                bool prawa = (f == 2 || f == 4);
                int  bx = prawa ? x2 + 10 : x3 - 30;

                for (auto& p : prostokatikiNaPietrze[f])
                    if (!p.wWindzie && fabs(p.x - bx) < 1.0f)
                    {
                        int d = abs(f - floorNow);
                        if (d < bestDist) { bestDist = d; foundFloor = f; }
                        break;
                    }
            }

            if (foundFloor)
            {
                docelowePietro = foundFloor;
                idleStart = 0;

                if (foundFloor == floorNow)
                {
                    stanDrzwi = 1;
                    czasOtwarciaDrzwi = GetTickCount();
                }
            }
            else
            {
                if (!idleStart) idleStart = GetTickCount();
                if (GetTickCount() - idleStart > 5000 && floorNow != 1)
                {
                    docelowePietro = 1;
                    trybJazdy = W_DOL;
                    idleStart = 0;
                }
            }
        }

        if (fabs(aktualnaPozycjaPix - celPix) < predkosc)
        {
            aktualnaPozycjaPix = celPix;
            if (stanDrzwi == 0 && !drzwiBylyOtwarte)
            {
                stanDrzwi = 1;
                czasOtwarciaDrzwi = GetTickCount();
            }
        }
        else
        {
            aktualnaPozycjaPix += (aktualnaPozycjaPix < celPix) ? predkosc : -predkosc;
            stanDrzwi = 0;
            drzwiBylyOtwarte = false;
            szerokoscLewegoBoku = szerokoscPrawegoBoku = 1.0f;
        }

        float krok = 0.05f;
        if (stanDrzwi == 1)
        {
            int pietro = docelowePietro;
            wysokosciPomaranczowychLinii[pietro] -= krok;
            if (wysokosciPomaranczowychLinii[pietro] < 0.0f)
                wysokosciPomaranczowychLinii[pietro] = 0.0f;
            if (docelowePietro % 2 == 1)
            {
                szerokoscLewegoBoku -= krok;
                if (szerokoscLewegoBoku <= 0.0f) szerokoscLewegoBoku = 0.0f;
            }
            else
            {
                szerokoscPrawegoBoku -= krok;
                if (szerokoscPrawegoBoku <= 0.0f) szerokoscPrawegoBoku = 0.0f;
            }

            if (szerokoscLewegoBoku == 0.0f || szerokoscPrawegoBoku == 0.0f)
            {
                stanDrzwi = 2;
                drzwiBylyOtwarte = true;
            }
        }

        if (stanDrzwi == 2)
        {
            int  floor = docelowePietro;
            pozycjaWindy = docelowePietro;

            for (int f = 1; f <= 5; ++f)
            {
                for (auto& p : prostokatikiNaPietrze[f])
                {
                    if (p.wWindzie && p.number == docelowePietro)
                    {
                        p.wychodzi = true;
                        p.wWindzie = false;
                        p.dirWyjscia = (docelowePietro == 2 ||
                            docelowePietro == 4) ? +1 : -1;
                    }
                }
            }

            CompressSlots();

            int zajete = 0;
            for (int ff = 1; ff <= 5; ++ff)
                for (auto& p : prostokatikiNaPietrze[ff])
                    if (p.wWindzie) ++zajete;
            int wolne = MAXSLOTS - zajete;

            bool prawaStrona = (floor == 2 || floor == 4);
            int  baseX = prawaStrona ? x2 + 10 : x3 - 10 - boxWidth;

            std::vector<ProstokatAnimowany*> kolejka;
            for (auto& p : prostokatikiNaPietrze[floor])
                if (!p.wWindzie && !p.wychodzi)
                    kolejka.push_back(&p);

            std::sort(kolejka.begin(), kolejka.end(),
                [&](ProstokatAnimowany* a, ProstokatAnimowany* b)
                {
                    return fabs(a->x - baseX) < fabs(b->x - baseX);
                });

            for (size_t i = 0; i < kolejka.size(); ++i)
            {
                float queueTarget = prawaStrona
                    ? baseX + i * odleglosc
                    : baseX - i * odleglosc;

                if (fabs(kolejka[i]->x - queueTarget) < predkoscAnimacji)
                    kolejka[i]->x = queueTarget;
                else
                    kolejka[i]->x += (kolejka[i]->x < queueTarget)
                    ? predkoscAnimacji
                    : -predkoscAnimacji;
            }

    
            if (GetTickCount() - czasOtwarciaDrzwi >= 1000)
            {
                if (!kolejka.empty() && wolne > 0 && fabs(kolejka[0]->x - baseX) < 0.5f)
                {
                    bool slotZajety[MAXSLOTS] = { false };
                    for (int ff = 1; ff <= 5; ++ff)
                        for (auto& q : prostokatikiNaPietrze[ff])
                            if (q.wWindzie && q.slot >= 0 && q.slot < MAXSLOTS)
                                slotZajety[q.slot] = true;

                    int freeSlot = -1;
                    for (int s = 0; s < MAXSLOTS; ++s)
                        if (!slotZajety[s]) { freeSlot = s; break; }

                    if (freeSlot != -1)
                    {
                        ProstokatAnimowany* neo = kolejka[0];
                        neo->wWindzie = true;
                        neo->slot = freeSlot;
                        --wolne;

                       
                        bool bylaPusta = true;
                        for (int ff = 1; ff <= 5 && bylaPusta; ++ff)
                            for (auto& q : prostokatikiNaPietrze[ff])
                                if (q.wWindzie && q.slot != freeSlot)
                                    bylaPusta = false;

                        if (bylaPusta) {
                            if (neo->number > floor) trybJazdy = W_GORE;
                            else if (neo->number < floor) trybJazdy = W_DOL;
                            else trybJazdy = IDLE;
                        }
                    }
                    kolejka.erase(kolejka.begin());
                }
            }

            auto slotPos = [&](int slot)
                {
                    return x1 + szerokosc_windy - 4
                        - boxWidth
                        - slot * odleglosc;
                };

            for (int ff = 1; ff <= 5; ++ff)
                for (auto& p : prostokatikiNaPietrze[ff])
                {
                    if (!p.wWindzie || p.slot < 0) continue;
                    float tx = slotPos(p.slot);
                    if (fabs(p.x - tx) < predkoscAnimacji)
                        p.x = tx;
                    else
                        p.x += (p.x < tx) ? predkoscAnimacji : -predkoscAnimacji;
                }

            for (int f = 1; f <= 5; ++f)
            {
                for (auto& p : prostokatikiNaPietrze[f])
                    if (p.wychodzi)
                        p.x += p.dirWyjscia * predkoscWyjscia;

                prostokatikiNaPietrze[f].erase(
                    std::remove_if(prostokatikiNaPietrze[f].begin(),
                        prostokatikiNaPietrze[f].end(),
                        [&](const ProstokatAnimowany& p)
                        {
                            return p.wychodzi &&
                                (p.x < -boxWidth ||
                                    p.x > szerokosc_ekranu + boxWidth);
                        }),
                    prostokatikiNaPietrze[f].end());
            }

            if (GetTickCount() - czasOtwarciaDrzwi > 6000)
                stanDrzwi = 3;
        }
        else if (stanDrzwi == 3)
        {
            int pietro = docelowePietro;
            wysokosciPomaranczowychLinii[pietro] += krok;
            if (wysokosciPomaranczowychLinii[pietro] > 1.0f)
                wysokosciPomaranczowychLinii[pietro] = 1.0f;

            if (docelowePietro % 2 == 1)
            {
                szerokoscLewegoBoku += krok;
                if (szerokoscLewegoBoku >= 1.0f)
                {
                    szerokoscLewegoBoku = 1.0f;
                    stanDrzwi = 0;
                }
            }
            else
            {
                szerokoscPrawegoBoku += krok;
                if (szerokoscPrawegoBoku >= 1.0f)
                {
                    szerokoscPrawegoBoku = 1.0f;
                    stanDrzwi = 0;
                }
            }
        }

       
        if (stanDrzwi == 0)
        {
            int zajeteAll = 0;
            for (int ff = 1; ff <= 5; ++ff)
                for (auto& pp : prostokatikiNaPietrze[ff])
                    if (pp.wWindzie)
                        ++zajeteAll;
            bool pelnaKabina = (zajeteAll >= MAXSLOTS);

            if (trybJazdy == W_GORE)
            {
                int nextFloor = 0;
                for (int f = pozycjaWindy + 1; f <= 5; ++f)
                {
                    
                    bool ktosWysiada = false;
                    for (int ff = 1; ff <= 5; ++ff)
                        for (auto& pp : prostokatikiNaPietrze[ff])
                            if (pp.wWindzie && pp.number == f)
                                ktosWysiada = true;

                    if (ktosWysiada) { nextFloor = f; break; }

                   
                    if (!pelnaKabina) {
                        bool prawa = (f == 2 || f == 4);
                        int bx = prawa ? x2 + 10 : x3 - 30;
                        for (auto& p : prostokatikiNaPietrze[f])
                            if (!p.wWindzie && fabs(p.x - bx) < 1.0f && p.number > f)
                            {
                                nextFloor = f;
                                goto FOUND_NEXTFLOOR_GORA;
                            }
                    }
                }
            FOUND_NEXTFLOOR_GORA:
                if (nextFloor)
                    docelowePietro = nextFloor;
                else
                    trybJazdy = W_DOL;
            }
            else if (trybJazdy == W_DOL)
            {
                int nextFloor = 0;
                for (int f = pozycjaWindy - 1; f >= 1; --f)
                {
                    bool ktosWysiada = false;
                    for (int ff = 1; ff <= 5; ++ff)
                        for (auto& pp : prostokatikiNaPietrze[ff])
                            if (pp.wWindzie && pp.number == f)
                                ktosWysiada = true;

                    if (ktosWysiada) { nextFloor = f; break; }

                    if (!pelnaKabina) {
                        bool prawa = (f == 2 || f == 4);
                        int bx = prawa ? x2 + 10 : x3 - 30;
                        for (auto& p : prostokatikiNaPietrze[f])
                            if (!p.wWindzie && fabs(p.x - bx) < 1.0f && p.number < f)
                            {
                                nextFloor = f;
                                goto FOUND_NEXTFLOOR_DOL;
                            }
                    }
                }
            FOUND_NEXTFLOOR_DOL:
                if (nextFloor)
                    docelowePietro = nextFloor;
                else
                    trybJazdy = W_GORE;
            }
        }





        for (int floor = 1; floor <= 5; ++floor)
        {
            bool prawaStrona = (floor == 2 || floor == 4);
            int  baseX = prawaStrona ? x2 + 10 : x3 - 10 - 20;

            if (stanDrzwi == 2 && floor == docelowePietro)
                continue;

            int queueIdx = 0;
            for (auto& p : prostokatikiNaPietrze[floor])
            {
                if (p.wWindzie || p.wychodzi)
                    continue;
                if (fabs(p.x - baseX) < 1.0f)
                {
                    ++queueIdx;
                    continue;
                }
                float targetX = prawaStrona
                    ? baseX + queueIdx * odleglosc
                    : baseX - queueIdx * odleglosc;
                if (fabs(p.x - targetX) < predkoscAnimacji)
                    p.x = targetX;
                else
                    p.x += (p.x < targetX) ? predkoscAnimacji : -predkoscAnimacji;
                ++queueIdx;
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);
        int szerokosc_ekranu = rect.right;
        int wysokosc_ekranu = rect.bottom;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, szerokosc_ekranu, wysokosc_ekranu);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

        Graphics graphics(memDC);
        graphics.Clear(Color(255, 255, 255, 255));

        double wysokosc_prostokata = 0.95 * wysokosc_ekranu;
        double szerokosc_prostokata = 0.2 * szerokosc_ekranu;

        int x = (szerokosc_ekranu - (int)szerokosc_prostokata) / 2;
        int y = (wysokosc_ekranu - (int)wysokosc_prostokata) / 2;

        double wysokosc_windy = 0.1 * wysokosc_prostokata;
        double szerokosc_windy = 0.95 * szerokosc_prostokata;

        int x1 = (szerokosc_ekranu - (int)szerokosc_windy) / 2;
        int x2 = (szerokosc_ekranu + (int)szerokosc_windy) / 2 + 6;
        int x3 = x1 - 6;

        double przesuniecie_pietra = 0.15 * wysokosc_prostokata;
        double y1 = aktualnaPozycjaPix;

        double pietro_1 = wysokosc_ekranu - 7 * y;
        double pietro_2 = pietro_1 - 0.15 * wysokosc_prostokata;
        double pietro_3 = pietro_1 - 0.3 * wysokosc_prostokata;
        double pietro_4 = pietro_1 - 0.45 * wysokosc_prostokata;
        double pietro_5 = pietro_1 - 0.6 * wysokosc_prostokata;

        Pen redPen(Color(255, 255, 0, 0), 4);
        Pen bluePen(Color(255, 0, 0, 255), 2);
        Pen invisiblePen(Color(0, 255, 255, 255), 2);
        Pen orangePen(Color(255, 255, 165, 0), 2);

        graphics.DrawRectangle(&invisiblePen, (INT)x, (INT)y, (INT)szerokosc_prostokata, (INT)wysokosc_prostokata);

        if (szerokoscLewegoBoku > 0.0f)
        {
            float dlugoscLinii = wysokosc_windy * szerokoscLewegoBoku;
            graphics.DrawLine(&redPen, (INT)x1, (INT)y1, (INT)x1, (INT)(y1 + dlugoscLinii));
        }
        graphics.DrawLine(&redPen, (INT)x1, (INT)y1, (INT)(x1 + szerokosc_windy), (INT)y1);
        if (szerokoscPrawegoBoku > 0.0f)
        {
            float dlugoscLinii = wysokosc_windy * szerokoscPrawegoBoku;
            graphics.DrawLine(&redPen, (INT)(x1 + szerokosc_windy), (INT)y1, (INT)(x1 + szerokosc_windy), (INT)(y1 + dlugoscLinii));
        }
        graphics.DrawLine(&redPen, (INT)x1, (INT)(y1 + wysokosc_windy), (INT)(x1 + szerokosc_windy), (INT)(y1 + wysokosc_windy));

        graphics.DrawLine(&bluePen, 0, (INT)pietro_1, x3, (INT)pietro_1);
        graphics.DrawLine(&bluePen, x2, (INT)pietro_2, szerokosc_ekranu, (INT)pietro_2);
        graphics.DrawLine(&bluePen, 0, (INT)pietro_3, x3, (INT)pietro_3);
        graphics.DrawLine(&bluePen, x2, (INT)pietro_4, szerokosc_ekranu, (INT)pietro_4);
        graphics.DrawLine(&bluePen, 0, (INT)pietro_5, x3, (INT)pietro_5);


        graphics.DrawLine(&orangePen, x3,
            (INT)(pietro_1 - wysokosc_windy),
            x3,
            (INT)(pietro_1 - wysokosc_windy + wysokosc_windy * wysokosciPomaranczowychLinii[1]));

        graphics.DrawLine(&orangePen, x2,
            (INT)(pietro_2 - wysokosc_windy),
            x2,
            (INT)(pietro_2 - wysokosc_windy + wysokosc_windy * wysokosciPomaranczowychLinii[2]));

        graphics.DrawLine(&orangePen, x3,
            (INT)(pietro_3 - wysokosc_windy),
            x3,
            (INT)(pietro_3 - wysokosc_windy + wysokosc_windy * wysokosciPomaranczowychLinii[3]));

        graphics.DrawLine(&orangePen, x2,
            (INT)(pietro_4 - wysokosc_windy),
            x2,
            (INT)(pietro_4 - wysokosc_windy + wysokosc_windy * wysokosciPomaranczowychLinii[4]));

        graphics.DrawLine(&orangePen, x3,
            (INT)(pietro_5 - wysokosc_windy),
            x3,
            (INT)(pietro_5 - wysokosc_windy + wysokosc_windy * wysokosciPomaranczowychLinii[5]));







        graphics.DrawLine(&bluePen, x3, wysokosc_ekranu - y, x2, wysokosc_ekranu - y);
        graphics.DrawLine(&bluePen, x2, ((INT)pietro_2), x2, wysokosc_ekranu - y);
        graphics.DrawLine(&bluePen, x2, wysokosc_ekranu - wysokosc_prostokata, x2, ((INT)pietro_4) - wysokosc_windy);
        graphics.DrawLine(&bluePen, x2, ((INT)pietro_4), x2, ((INT)pietro_2) - wysokosc_windy);

        graphics.DrawLine(&bluePen, x3, ((INT)pietro_1), x3, wysokosc_ekranu - y);

        graphics.DrawLine(&bluePen, x3, ((INT)pietro_1 - wysokosc_windy), x3, (INT)pietro_3);
        graphics.DrawLine(&bluePen, x3, ((INT)pietro_3 - wysokosc_windy), x3, (INT)pietro_5);
        graphics.DrawLine(&bluePen, x3, ((INT)pietro_5 - wysokosc_windy), x3, wysokosc_ekranu - wysokosc_prostokata);
        graphics.DrawLine(&bluePen, x3, wysokosc_ekranu - wysokosc_prostokata, x2, wysokosc_ekranu - wysokosc_prostokata);


        graphics.DrawRectangle(&bluePen, ((szerokosc_ekranu - szerokosc_prostokata) / 2 + 5 * wysokosc_windy), 0, 2 * wysokosc_windy, wysokosc_windy);

        int liczbaWWindzie = 0;
        for (int f = 1; f <= 5; ++f)
            for (auto& p : prostokatikiNaPietrze[f])
                if (p.wWindzie) ++liczbaWWindzie;

        int masa = liczbaWWindzie * 70;          

        FontFamily ff(L"Arial");
        Font        fontLabel(&ff, 14);          
        SolidBrush  brush(Color(255, 0, 0, 0));  

        float rx = (szerokosc_ekranu - szerokosc_prostokata) / 2.0f + 5.0f * (float)wysokosc_windy;
        float ry = 0.0f;
        float rw = 2.0f * (float)wysokosc_windy;
        float rh = (float)wysokosc_windy;

        WCHAR buf[64];
        swprintf(buf, 64, L"Masa: %d kg", masa);


        StringFormat sf;
        sf.SetAlignment(StringAlignmentCenter);
        sf.SetLineAlignment(StringAlignmentCenter);

        graphics.DrawString(buf, -1, &fontLabel,
            RectF(rx, ry, rw, rh),
            &sf, &brush);




        FontFamily fontFamily(L"Arial");
        Font font(&fontFamily, 10);  
        SolidBrush blackBrush(Color(255, 0, 0, 0));  

        int buttonWidth = 40;   
        int buttonHeight = 20;  
        int buttonMargin = 20; 

        przyciski.clear();  


        auto drawButtonsForFloor = [&](double pietroY, int floorNumber, bool prawaStrona)
            {
                for (int i = 0; i < 5; ++i)
                {
                    if (i + 1 == floorNumber) continue;
                    int buttonYBottom = (int)pietroY;
                    int buttonYTop = buttonYBottom - (i + 1) * buttonHeight;

                    int buttonX = prawaStrona
                        ? szerokosc_ekranu - buttonMargin - buttonWidth
                        : buttonMargin;

                    graphics.DrawRectangle(&bluePen, buttonX, buttonYTop, buttonWidth, buttonHeight);


                    WCHAR text[2];
                    wsprintf(text, L"%d", i + 1);
                    graphics.DrawString(text, -1, &font,
                        PointF(buttonX + 12, buttonYTop + 4),
                        &blackBrush);

                    przyciski.push_back(ButtonInfo{ buttonX, buttonYTop, buttonWidth, buttonHeight, floorNumber, i + 1, prawaStrona });
                }
            };


  
        drawButtonsForFloor(pietro_1, 1, false);  
        drawButtonsForFloor(pietro_2, 2, true);   
        drawButtonsForFloor(pietro_3, 3, false);  
        drawButtonsForFloor(pietro_4, 4, true);   
        drawButtonsForFloor(pietro_5, 5, false);  




        for (int floor = 1; floor <= 5; ++floor)
        {
            if (prostokatikiNaPietrze[floor].empty())
                continue;

            double pietroY = 0;
            bool   prawaStrona = false;
            switch (floor)
            {
            case 1: pietroY = pietro_1; prawaStrona = false; break;
            case 2: pietroY = pietro_2; prawaStrona = true;  break;
            case 3: pietroY = pietro_3; prawaStrona = false; break;
            case 4: pietroY = pietro_4; prawaStrona = true;  break;
            case 5: pietroY = pietro_5; prawaStrona = false; break;
            }

            const int boxWidth = 20;
            const int boxHeight = 40;

            for (auto& p : prostokatikiNaPietrze[floor])
            {
                int boxY = (p.wWindzie || p.wychodzi)
                    ? (int)(y1 + wysokosc_windy) - boxHeight
                    : (int)pietroY - boxHeight;


                int boxX = (int)p.x;

                graphics.DrawRectangle(&redPen, boxX, boxY, boxWidth, boxHeight);

                WCHAR txt[2]; wsprintf(txt, L"%d", p.number);
                graphics.DrawString(txt, -1, &font,
                    PointF(boxX + 4, boxY + 10), &blackBrush);
            }

        }




        BitBlt(hdc, 0, 0, szerokosc_ekranu, wysokosc_ekranu, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    const wchar_t CLASS_NAME[] = L"GDIPlusWindowClass";
    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc)) return 1;

    HWND hWnd = CreateWindowEx(
        WS_EX_COMPOSITED,
        CLASS_NAME,
        L"Moja winda – GDI+",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
