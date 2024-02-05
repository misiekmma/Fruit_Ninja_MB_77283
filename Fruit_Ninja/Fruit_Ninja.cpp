// Fruit_Ninja.cpp : Definiuje punkt wejscia dla aplikacji.
//

#include "framework.h"
#include "Fruit_Ninja.h"
#include <iostream>
#include <vector>
#include <time.h>

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // Biezaca instancja
WCHAR szTitle[MAX_LOADSTRING];                  // Tekst paska tytulu
WCHAR szWindowClass[MAX_LOADSTRING];            // Nazwa klasy okna glownego

struct Circle_STR //Struktura przedstawiajaca okregi narysowane na planszy
{
    double x, y, r;
    double dx, dy;
    COLORREF col;
};

int timer_speed = 50; // okresla, jak często obszar klienta jest przerysowywany (co 50 milisekund)
int total_level_time = 30; // okresla długosc gry (domyslnie 30 sekund)

int m_blockLength = 50; // wielkosc jednego kwadratu tworzacego plansze
int m_game_size = 0;    // 0: Small, 1:Medium, 2:Big
int m_width, m_height; // szerokość i wysokość obszaru 
int m_progressbar_height = 20;
int m_title_height = 60;
int m_width_add = 15;

vector<POINT> m_track_pt; // wektor punktow, uzywany do utrzymywania pozycji kursora na ekranie.
vector<Circle_STR> m_circleLists; //wektor zawierajacy wszystkie istniejace okregi
bool game_state = true; // jeśli falszywe, gra konczy sie
int m_score = 0;
double m_gravity = 0.1; 

clock_t game_start_time, game_current_time;
clock_t clock_mouse_move; // mierzy czas, jaki uplynal od ostatniego ruchu kursora nad aplikacja.

HCURSOR m_cursor_knife;
HCURSOR m_cursor_arrow;
HBRUSH m_back_brush[2]; //szereg pedzli sluzących do wypełniania kwadratow na planszy
HBRUSH m_prog_real;  // pedzel uzywany do wypelniania paska postepu.
HPEN m_track_pen; // dlugopis uzywany do rysowania ciec nozem
HPEN m_back_pen[2];
HGDIOBJ m_null_pen;
HFONT hfont; // czcionka uzywana do wyswietlania aktualnego wyniku
HFONT hfont_finish;  // czcionka uzywana do wyswietlania wyniku po zakonczeniu gry

// Przekaz deklaracje funkcji zawartych w tym module kodu:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Zainicjuj ciagow globalnych
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FRUITNINJA, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Wykonaj inicjalizacje aplikacji:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FRUITNINJA));

    int style;
    style = GetWindowLong(GetActiveWindow(), GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    //style &= ~WS_MINIMIZEBOX;
    style &= ~WS_THICKFRAME;
    SetWindowLong(GetActiveWindow(), GWL_STYLE, style);

    m_cursor_knife = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_KNIFE));
    m_cursor_arrow = LoadCursor(nullptr, IDC_ARROW);

    MSG msg;

    // Glowna petla message :
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

//
//  FUNKCJA: MyRegisterClass()
//
// CEL: Rejestruje klase okna.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FRUIT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FRUITNINJA);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_FRUIT));

    return RegisterClassExW(&wcex);
}

//
// FUNKCJA: InitInstance(HINSTANCE, int)
//
// CEL: Zapisuje uchwyt instancji i tworzy okno glowne
//
//   UWAGI:
//
// W tej funkcji zapisujemy uchwyt instancji w zmiennej globalnej i
// utworz i wyswietl glowne okno programu.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) 
{
    hInst = hInstance; // Przechowanie uchwyt instancji w zmiennej globalnej

    HWND hWnd = CreateWindowW(szWindowClass, L"Fruit Ninja", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
// FUNKCJA: WndProc(HWND, UINT, WPARAM, LPARAM)
//
// CEL: Przetwarza komunikaty dla okna głównego.
//
// WM_COMMAND - przetwarza menu aplikacji
// WM_PAINT – Maluje glowne okno
// WM_DESTROY - wyslij wiadomosc o wyjsciu i wroc
//
//

void SaveConfig() {
    HANDLE hFile;
    DWORD  dwBytesWritten = 0;
    char   DataBuffer[1] = { 0 };
    hFile = CreateFileW(L"FruitSetting.ini", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);                     
    DataBuffer[0] = '0' + m_game_size;

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }
    WriteFile(hFile, DataBuffer, 1, &dwBytesWritten, NULL);            
}

void ReadConfig() {
    HANDLE hFile;
    DWORD  dwBytesRead = 0;
    char   ReadBuffer[1] = { 0 };
    OVERLAPPED ol = { 0 };
    hFile = CreateFile(L"FruitSetting.ini", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    m_game_size = 0;
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (0 != ReadFileEx(hFile, ReadBuffer, 1, &ol, NULL))
    {
        m_game_size = ReadBuffer[0] - '0';
    }
    CloseHandle(hFile);
}

void drawFinishBoard(HDC hdc) {
    int w_cnt = 0, h_cnt = 0;
    if (m_game_size == 0) {
        w_cnt = 8; h_cnt = 6;
    }
    if (m_game_size == 1) {
        w_cnt = 12; h_cnt = 10;
    }
    if (m_game_size == 2) {
        w_cnt = 16; h_cnt = 12;
    }

    HPEN m_board_finish_pen;
    HBRUSH m_board_finish_brush;
    for (int i = 0; i < w_cnt; i++) {
        for (int j = 0; j < h_cnt; j++) {
            COLORREF col = 0x008000;
            if ((i + j) % 2 == 1) {
                col = 0x80ff80;
            }
            m_board_finish_pen = CreatePen(PS_SOLID, 1, col);
            m_board_finish_brush = CreateSolidBrush(col);
            SelectObject(hdc, m_board_finish_pen);
            SelectObject(hdc, m_board_finish_brush);
            Rectangle(hdc, i * m_blockLength, j * m_blockLength, (i + 1) * m_blockLength, (j + 1) * m_blockLength);
            DeleteObject(m_board_finish_pen);
            DeleteObject(m_board_finish_brush);
        }
    }

    m_null_pen = GetStockObject(NULL_PEN);
    SelectObject(hdc, m_null_pen);
    HBRUSH m_circle_brush;
    for (int i = 0; i < m_circleLists.size(); i++)
    {
        m_circle_brush = CreateSolidBrush(m_circleLists[i].col);
        SelectObject(hdc, m_circle_brush);
        Ellipse(hdc, m_circleLists[i].x - m_circleLists[i].r, m_circleLists[i].y - m_circleLists[i].r,
            m_circleLists[i].x + m_circleLists[i].r, m_circleLists[i].y + m_circleLists[i].r);
        DeleteObject(m_circle_brush);
    }

    SetTextColor(hdc, RGB(0, 255, 0));
    SetBkMode(hdc, TRANSPARENT);
    hfont = CreateFont(40, 20, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ANSI_CHARSET);
    SelectObject(hdc, hfont);
    TCHAR s[100];
    _stprintf_s(s, _T("%i"), m_score);
    SetTextAlign(hdc, TA_RIGHT | TA_TOP);
    TextOut(hdc, m_width, 0, s, lstrlen(s));
    DeleteObject(hfont);

    m_prog_real = CreateSolidBrush(RGB(0, 255, 0));
    SelectObject(hdc, m_prog_real);
    Rectangle(hdc, 0, m_height - 81, m_width, m_height - 57);
    DeleteObject(m_prog_real);
}


void newGame(HWND hWnd) {
    m_score = 0;
    game_state = true;
    game_start_time = clock();
    m_circleLists.clear();
    int genTimerIntereval = 2000;

    if (m_game_size == 0) {
        m_width = m_blockLength * 8;
        m_height = m_blockLength * 6 + m_progressbar_height + m_title_height;
    }
    if (m_game_size == 1) {
        m_width = m_blockLength * 12;
        m_height = m_blockLength * 10 + m_progressbar_height + m_title_height;
        genTimerIntereval = 1000;
    }
    if (m_game_size == 2) {
        m_width = m_blockLength * 16;
        m_height = m_blockLength * 12 + m_progressbar_height + m_title_height;
        genTimerIntereval = 500;
    }
    int w, h;
    w = GetSystemMetrics(SM_CXSCREEN);
    h = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hWnd, HWND_TOPMOST, (w - m_width) / 2, (h - m_height) / 2, m_width + m_width_add, m_height, 0);
    SetTimer(hWnd, 111, timer_speed, 0);
    SetTimer(hWnd, 222, genTimerIntereval, 0);
    SetTimer(hWnd, 444, 3000, 0);
}

void drawBoard(HDC hdc) {
    int w_cnt = 0, h_cnt = 0;

    if (m_game_size == 0) {
        w_cnt = 8; h_cnt = 6;
    }
    if (m_game_size == 1) {
        w_cnt = 12; h_cnt = 10;
    }
    if (m_game_size == 2) {
        w_cnt = 16; h_cnt = 12;
    }

    m_back_brush[0] = CreateSolidBrush(RGB(0, 0, 0));
    m_back_brush[1] = CreateSolidBrush(RGB(255, 255, 255));
    m_back_pen[0] = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    m_back_pen[1] = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    for (int i = 0; i < w_cnt; i++) {
        for (int j = 0; j < h_cnt; j++) {

            SelectObject(hdc, m_back_pen[(i + j) % 2]);
            SelectObject(hdc, m_back_brush[(i + j) % 2]);
            Rectangle(hdc, i * m_blockLength, j * m_blockLength, (i + 1) * m_blockLength, (j + 1) * m_blockLength);

        }
    }
    DeleteObject(m_back_brush[0]);
    DeleteObject(m_back_brush[1]);
    DeleteObject(m_back_pen[0]);
    DeleteObject(m_back_pen[1]);

    for (int i = 0; i < m_circleLists.size(); i++) {
        m_circleLists[i].x += m_circleLists[i].dx;
        m_circleLists[i].y += m_circleLists[i].dy;
        m_circleLists[i].dy += m_gravity;
    }
    vector<Circle_STR>::iterator it = m_circleLists.begin();

    while (it != m_circleLists.end()) {
        double dy = (*it).dy;
        double y = (*it).y;
        if (dy > 0 && y > m_height) {
            it = m_circleLists.erase(it);
        }
        else ++it;
    }

    m_null_pen = GetStockObject(NULL_PEN);
    SelectObject(hdc, m_null_pen);

    HBRUSH m_circle_brush;
    for (int i = 0; i < m_circleLists.size(); i++)
    {
        m_circle_brush = CreateSolidBrush(m_circleLists[i].col);
        SelectObject(hdc, m_circle_brush);
        Ellipse(hdc, m_circleLists[i].x - m_circleLists[i].r, m_circleLists[i].y - m_circleLists[i].r,
            m_circleLists[i].x + m_circleLists[i].r, m_circleLists[i].y + m_circleLists[i].r);
        DeleteObject(m_circle_brush);
    }

    SetTextColor(hdc, RGB(0, 255, 0));
    SetBkMode(hdc, TRANSPARENT);
    hfont = CreateFont(40, 20, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ANSI_CHARSET);
    SelectObject(hdc, hfont);
    TCHAR s[100];
    _stprintf_s(s, _T("%i"), m_score);
    SetTextAlign(hdc, TA_RIGHT | TA_TOP);
    TextOut(hdc, m_width, 0, s, lstrlen(s));
    DeleteObject(hfont);

    game_current_time = clock();
    double time_duo = ((double)game_current_time - (double)game_start_time) / CLOCKS_PER_SEC;

    m_prog_real = CreateSolidBrush(RGB(0, 255, 0));
    SelectObject(hdc, m_prog_real);
    Rectangle(hdc, 0, m_height - 81, (time_duo * m_width) / total_level_time, m_height - 57);
    DeleteObject(m_prog_real);

    if (m_track_pt.size() != 0)
    {
        m_track_pen = CreatePen(PS_SOLID, 3, RGB(255, 255, 0));
        SelectObject(hdc, m_track_pen);
        MoveToEx(hdc, m_track_pt[0].x, m_track_pt[0].y, NULL);
        LineTo(hdc, m_track_pt[1].x, m_track_pt[1].y);
        DeleteObject(m_track_pen);
        m_track_pt[1].x = m_track_pt[1].x + (m_track_pt[0].x - m_track_pt[1].x);
        m_track_pt[1].y = m_track_pt[1].y + (m_track_pt[0].y - m_track_pt[1].y);
    }

    if (time_duo >= total_level_time) {
        drawFinishBoard(hdc);
        game_state = false;
    }
}

float distance(int x1, int y1, int x2, int y2)
{ 
    return sqrt(pow(x2 - x1, 2) +
        pow(y2 - y1, 2) * 1.0);
}

void splitCircles() {
    if (m_track_pt.size() == 0) return;

    vector<int> idList;
    for (int i = m_circleLists.size() - 1; i >= 0; i--) {
        if (distance(m_circleLists[i].x, m_circleLists[i].y, m_track_pt[0].x, m_track_pt[0].y) < m_circleLists[i].r)
        {
            idList.push_back(i);
            m_score++;
        }
    }

    int dx, dy;
    dx = m_track_pt[0].x - m_track_pt[1].x;
    dy = m_track_pt[0].y - m_track_pt[1].y;

    double rx = dx / sqrt(pow(dx, 2) + pow(dy, 2));
    double ry = dy / sqrt(pow(dx, 2) + pow(dy, 2));

    for (int i = 0; i < idList.size(); i++)
    {
        if (m_circleLists[idList[i]].r > 10) {
            for (int kk = 0; kk < 4; kk++) {
                Circle_STR tmp;
                tmp.col = m_circleLists[idList[i]].col;
                tmp.x = m_circleLists[idList[i]].x + rand() % (int)(m_circleLists[idList[i]].r) - m_circleLists[idList[i]].r / 2;
                tmp.y = m_circleLists[idList[i]].y + rand() % (int)(m_circleLists[idList[i]].r) - m_circleLists[idList[i]].r / 2;
                tmp.r = m_circleLists[idList[i]].r / 3;
                tmp.dx = m_circleLists[idList[i]].dx + (rand() % 5) * 0.1 * rx;
                tmp.dy = m_circleLists[idList[i]].dy + (rand() % 5) * 0.1 * ry;
                m_circleLists.push_back(tmp);
            }
        }
        m_circleLists.erase(m_circleLists.begin() + idList[i]);
    }
}


void generateNewCircle() {
    Circle_STR circle;
    circle.r = 32;
    circle.col = RGB(rand() % 255, rand() % 255, rand() % 255);
    int rand_dy = m_height + rand() % (int)(m_height * 0.2);
    int rand_dx = 1 - rand() % 3;
    int rand_x = circle.r + rand() % (int)(m_width - circle.r * 2);
    if (m_game_size != 0)
        circle.dy = -rand_dy / 55;
    else
        circle.dy = -rand_dy / 45;
    circle.dx = rand_dx;
    circle.x = rand_x;
    circle.y = m_height;
    m_circleLists.push_back(circle);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int xPos, yPos, style;
    LRESULT result;
    HBRUSH br;
    HMENU hMenu;
    double time_duo;

    switch (message)
    {
    case WM_COMMAND:
    {
        hMenu = GetMenu(hWnd);
        int wmId = LOWORD(wParam);
        // Analizuj opcje menu:
        switch (wmId)
        {
        case ID_BOARD_SMALL:
            if (m_game_size == 0) break;
            m_game_size = 0;
            CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_BYCOMMAND | MF_CHECKED);
            CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_BOARD_BIG, MF_BYCOMMAND | MF_UNCHECKED);
            newGame(hWnd);
            break;
        case ID_BOARD_MEDIUM:
            if (m_game_size == 1) break;
            m_game_size = 1;
            CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_BYCOMMAND | MF_CHECKED);
            CheckMenuItem(hMenu, ID_BOARD_BIG, MF_BYCOMMAND | MF_UNCHECKED);
            newGame(hWnd);
            break;
        case ID_BOARD_BIG:
            if (m_game_size == 2) break;
            m_game_size = 2;
            CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, ID_BOARD_BIG, MF_BYCOMMAND | MF_CHECKED);
            newGame(hWnd);
            break;
        case ID_FILE_NEW:
            newGame(hWnd);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_CREATE:
        ReadConfig();
        hMenu = GetMenu(hWnd);
        CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_BYCOMMAND | MF_UNCHECKED);
        CheckMenuItem(hMenu, ID_BOARD_BIG, MF_BYCOMMAND | MF_UNCHECKED);
        if (m_game_size == 0) CheckMenuItem(hMenu, ID_BOARD_SMALL, MF_BYCOMMAND | MF_CHECKED);
        if (m_game_size == 1) CheckMenuItem(hMenu, ID_BOARD_MEDIUM, MF_BYCOMMAND | MF_CHECKED);
        if (m_game_size == 2) CheckMenuItem(hMenu, ID_BOARD_BIG, MF_BYCOMMAND | MF_CHECKED);
        newGame(hWnd);
        clock_mouse_move = clock();
        break;
    case WM_PAINT:
    {
        HDC hdc;
        PAINTSTRUCT ps;
        hdc = BeginPaint(hWnd, &ps);
        drawBoard(hdc);
        if (game_state == false) {
            TCHAR s[100];
            SetTextColor(hdc, RGB(255, 255, 255));
            hfont_finish = CreateFont(60, 30, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, ANSI_CHARSET);
            SelectObject(hdc, hfont_finish);
            _stprintf_s(s, _T("Score:"));
            SetTextAlign(hdc, TA_CENTER);
            TextOut(hdc, m_width / 2, m_height / 2 - 65, s, lstrlen(s));
            _stprintf_s(s, _T("%i"), m_score);
            TextOut(hdc, m_width / 2, m_height / 2 - 15, s, lstrlen(s));
            DeleteObject(hfont_finish);
            KillTimer(hWnd, 111);
            KillTimer(hWnd, 222);
            KillTimer(hWnd, 444);
        }
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_MOUSEMOVE:
        xPos = LOWORD(lParam);
        yPos = HIWORD(lParam);
        if (m_track_pt.size() == 0)
        {
            POINT pt;
            pt.x = xPos;
            pt.y = yPos;
            m_track_pt.push_back(pt);
            m_track_pt.push_back(pt);
        }
        else
        {
            m_track_pt[0].x = xPos;
            m_track_pt[0].y = yPos;
        }

        clock_mouse_move = clock();
        splitCircles();
        style = GetWindowLong(GetActiveWindow(), GWL_EXSTYLE);
        style &= ~WS_EX_LAYERED;
        SetWindowLong(GetActiveWindow(), GWL_EXSTYLE, style);
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case 111:
            InvalidateRect(hWnd, 0, TRUE);
            return 0;
        case 222:
            generateNewCircle();
            return 0;
        case 444:
            time_duo = (double(clock()) - double(clock_mouse_move)) / CLOCKS_PER_SEC;
            if (time_duo > 3.0) {
                style = GetWindowLong(GetActiveWindow(), GWL_EXSTYLE);
                style |= WS_EX_LAYERED;
                SetWindowLong(GetActiveWindow(), GWL_EXSTYLE, style);
                SetLayeredWindowAttributes(hWnd, 0, 176, LWA_ALPHA);
            }
            return 0;
        default:
            break;
        }
        break;
    case WM_NCLBUTTONDOWN:
        if (LOWORD(wParam) == HTCAPTION) {
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCAPTION || LOWORD(lParam) == HTMENU || LOWORD(lParam) == HTSYSMENU)
            SetCursor(m_cursor_arrow);
        else
            SetCursor(m_cursor_knife);
        return TRUE;
    case WM_DESTROY:
        SaveConfig();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
