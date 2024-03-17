#include <iostream>
#include <windows.h>
#include <TlHelp32.h>

#include "memory.h"
#include "vector.h"

//kresliace funkcie
void DrawBorderBox(HDC hdc, int x, int y, int w, int h, COLORREF borderColor) {
    HBRUSH hBorderBrush = CreateSolidBrush(borderColor);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBorderBrush);

    RECT rect = { x, y, x + w, y + h };
    FrameRect(hdc, &rect, hBorderBrush);

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBorderBrush);
}

void DrawLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    MoveToEx(hdc, x1, y1, NULL);
    LineTo(hdc, x2, y2);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

//struktura adres ktore budeme potrebovat
struct Adresy
{
    static constexpr int dwEntityList = 0x18C2D58;
    static constexpr int m_hPlayerPawn = 0x7E4;
    static constexpr int m_vOldOrigin = 0x127C;
    static constexpr int dwViewMatrix = 0x19241A0;
    static constexpr int dwLocalPlayerPawn = 0x17371A8;
    static constexpr int m_iHealth = 0x334;
    static constexpr int m_iTeamNum = 0x3CB;
};

void renderAllPlayers(HDC hdc) {
    for (int i = 0; i < 64; i++) {
        //Vas hrac za ktoreho hrate
        const auto local_player_pawn = VARS::memRead<std::uintptr_t>(VARS::baseAddress + Adresy::dwLocalPlayerPawn);

        //ciselne udaje mam zo pseudokodu zo suboru client.dll(pouzil som na to IDA Pro)
        const auto Entity = VARS::memRead<std::uintptr_t>(VARS::baseAddress + Adresy::dwEntityList);
        if (Entity == 0)
            continue;

        const auto listEntity = VARS::memRead<std::uintptr_t>(Entity + ((8 * (i & 0x7FFF) >> 9) + 16));
        if (listEntity == 0)
            continue;

        const auto entityController = VARS::memRead<std::uintptr_t>(listEntity + (120) * (i & 0x1FF));
        if (entityController == 0)
            continue;

        const auto entityControllerPawn = VARS::memRead<std::uintptr_t>(entityController + Adresy::m_hPlayerPawn);
        if (entityControllerPawn == 0)
            continue;

        const auto listEntity2 = VARS::memRead<std::uintptr_t>(Entity + (0x8 * ((entityControllerPawn & 0x7FFF) >> 9) + 16));
        if (listEntity2 == 0)
            continue;

        const auto entityPawn = VARS::memRead<std::uintptr_t>(listEntity2 + (120) * (entityControllerPawn & 0x1FF));

        if (entityPawn == 0)
            continue;

        //3D suradnice hraca
        const auto oldOrigin = VARS::memRead<Vector3>(entityPawn + Adresy::m_vOldOrigin);

        std::cout << oldOrigin.x << " " << oldOrigin.y << " " << oldOrigin.z << std::endl;

        //adresa, v ktorej sa nachadzaju informacie na to aby som premenil 3D suradnice na 2D suradnice(suradnice na obrazovke)
        const auto viewMatrix = VARS::memRead<view_matrix_t>(VARS::baseAddress + Adresy::dwViewMatrix);

        //3D pozicia hlavy
        Vector3 head;
        head.x = oldOrigin.x;
        head.y = oldOrigin.y;
        head.z = oldOrigin.z + 75.f;

        //2D pozicia entity
        Vector3 screenpos = oldOrigin.WTS(viewMatrix);
        //2D pozicia hlavy entity
        Vector3 screenhead = head.WTS(viewMatrix);

        //vyska a sirka entity
        float height = screenpos.y - screenhead.y;
        float width = height / 2.4f;

        //udaje o entite (zivoty, team)
        const auto entityPLRHealth = VARS::memRead<int>(entityPawn + Adresy::m_iHealth);
        const auto entityPLRTeam = VARS::memRead<int>(entityPawn + Adresy::m_iTeamNum);

        const auto localPLRTeam = VARS::memRead<int>(local_player_pawn + Adresy::m_iTeamNum);

        //zistujeme ci entita je na obrazovke
        if (screenpos.z >= 0.01f) {
            //zistujeme ci niesme my
            if (entityPawn != local_player_pawn) {
                //zistujeme ci entita zije
                if (entityPLRHealth != 0) {
                    //zistujeme ci entita je v mojom teame
                    if (localPLRTeam == entityPLRTeam) {
                        //kreslenie boxu cez entitu
                        DrawBorderBox(hdc, screenhead.x - width / 2, screenhead.y,
                            width, height, RGB(255, 255, 255));
                        //kreslenie ciary, ktora ide z hora az ku hlave entity
                        DrawLine(hdc, GetSystemMetrics(SM_CXSCREEN) / 2,
                            GetSystemMetrics(SM_CYSCREEN) / 2 / 2 / 2,
                            screenhead.x - width / 2 / 5, screenhead.y,
                            RGB(255, 255, 255));
                    }
                    else {
                        //deje sa tu to iste ale farba je ina ak hrac sa nenachadza v mojom teame
                        DrawBorderBox(hdc, screenhead.x - width / 2, screenhead.y, width, height, RGB(255, 0, 0));
                        DrawLine(hdc, GetSystemMetrics(SM_CXSCREEN) / 2,
                            GetSystemMetrics(SM_CYSCREEN) / 2 / 2 / 2,
                            screenhead.x - width / 2 / 5, screenhead.y,
                            RGB(255, 0, 0));
                    }
                }
            }
        }
    }
}

HDC g_hdcBuffer = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int frameCount;

    const int resetCondition = 200;

    switch (uMsg) {
    case WM_PAINT: {
        //tu vykonavame kreslenie

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)(COLOR_BACKGROUND + 1));

        // double buffering aby kresby menej blikali
        if (g_hdcBuffer == NULL) {
            g_hdcBuffer = CreateCompatibleDC(hdc);
        }

        //kreslenie boxu okolo hracov
        renderAllPlayers(hdc);

        BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, g_hdcBuffer, 0, 0,
            SRCCOPY);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER:
        //resetovanie kresieb a zmena pozicie krezieb

        if (frameCount < resetCondition) {

        }
        else {
            renderAllPlayers(g_hdcBuffer);
        }
        frameCount++;
        InvalidateRect(hwnd, NULL, FALSE);

        return 0;
    case WM_CREATE:
        SetTimer(hwnd, 1, 24, NULL);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        if (g_hdcBuffer != NULL) {
            DeleteDC(g_hdcBuffer);
        }

        PostQuitMessage(0);
        return 0;
    case WM_LBUTTONDOWN: {
        //ak omylom kliknem na jednu z kresieb tak nech sa nic nestane.
        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int main()
{
    //neviditelne okno, ktore prekryva celu nasu obrazovku a umoznuje nam vidiet nase kresby
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"GDIOverlay";
    RegisterClass(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT, L"GDIOverlay",
        L"GDI Overlay", WS_POPUP, 0, 0, screenWidth, screenHeight, NULL,
        NULL, GetModuleHandle(NULL), NULL);


    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    //sprava ok kresbach
    MSG msg;
    while (true) {
        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
