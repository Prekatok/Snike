#include <windows.h>
#include <string>
#include <vector>
#include <ctime>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

const int CELL_SIZE = 25;
const int GRID_WIDTH = 30;
const int GRID_HEIGHT = 20;
const int INITIAL_SPEED = 150;
const int SPEED_INCREMENT = 5;

enum Direction { UP, DOWN, LEFT, RIGHT };
enum GameState { RUNNING, PAUSED, GAME_OVER };

struct SnakeSegment {
    int x;
    int y;
};

std::vector<SnakeSegment> snake;
Direction snakeDirection = RIGHT;
int foodX = 0, foodY = 0;
int score = 0, speed = INITIAL_SPEED;
GameState gameState = RUNNING;
bool specialFoodActive = false;
int specialFoodX = -1, specialFoodY = -1;
ULONG_PTR gdiplusToken;
HBITMAP hBufferBitmap = NULL;
HDC hBufferDC = NULL;

void GenerateFood();
void InitializeGame();
void UpdateSnake();
void DrawGame(HDC hdc, RECT clientRect);
void TogglePause() {
    if (gameState == RUNNING) {
        gameState = PAUSED;
    }
    else if (gameState == PAUSED) {
        gameState = RUNNING;
    }
}
void CleanupBuffer();

void GenerateFood() {
    foodX = rand() % GRID_WIDTH;
    foodY = rand() % GRID_HEIGHT;

    for (const auto& segment : snake) {
        if (segment.x == foodX && segment.y == foodY) {
            GenerateFood();
            return;
        }
    }

    if (rand() % 5 == 0 && !specialFoodActive) {
        specialFoodActive = true;
        specialFoodX = rand() % GRID_WIDTH;
        specialFoodY = rand() % GRID_HEIGHT;

        for (const auto& segment : snake) {
            if (segment.x == specialFoodX && segment.y == specialFoodY) {
                specialFoodActive = false;
                break;
            }
        }
    }
}

void InitializeGame() {
    snake.clear();
    snake.push_back({ 5, 5 });
    snake.push_back({ 4, 5 });
    snake.push_back({ 3, 5 });
    snakeDirection = RIGHT;
    score = 0;
    speed = INITIAL_SPEED;
    gameState = RUNNING;
    specialFoodActive = false;
    GenerateFood();
}

void UpdateSnake() {
    if (gameState != RUNNING) return;

    SnakeSegment newHead = snake[0];

    switch (snakeDirection) {
    case UP:    newHead.y--; break;
    case DOWN:  newHead.y++; break;
    case LEFT:  newHead.x--; break;
    case RIGHT: newHead.x++; break;
    }

    if (newHead.x < 0 || newHead.x >= GRID_WIDTH ||
        newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
        gameState = GAME_OVER;
        return;
    }

    for (const auto& segment : snake) {
        if (segment.x == newHead.x && segment.y == newHead.y) {
            gameState = GAME_OVER;
            return;
        }
    }

    snake.insert(snake.begin(), newHead);

    if (newHead.x == foodX && newHead.y == foodY) {
        score += 10;
        speed = max(INITIAL_SPEED - (score / 50) * SPEED_INCREMENT, 50);
        GenerateFood();
    }
    else if (specialFoodActive && newHead.x == specialFoodX && newHead.y == specialFoodY) {
        score += 50;
        specialFoodActive = false;
    }
    else {
        snake.pop_back();
    }
}

void DrawGame(HDC hdc, RECT clientRect) {
    using namespace Gdiplus;

    if (hBufferBitmap == NULL) {
        hBufferDC = CreateCompatibleDC(hdc);
        hBufferBitmap = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
        SelectObject(hBufferDC, hBufferBitmap);
    }

    Graphics graphics(hBufferDC);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);

    SolidBrush bgBrush(Color(30, 30, 30));
    graphics.FillRectangle(&bgBrush, 0, 0, clientRect.right, clientRect.bottom);

    Pen gridPen(Color(50, 50, 50), 1);
    for (int x = 0; x <= GRID_WIDTH; x++) {
        graphics.DrawLine(&gridPen, x * CELL_SIZE, 0, x * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);
    }
    for (int y = 0; y <= GRID_HEIGHT; y++) {
        graphics.DrawLine(&gridPen, 0, y * CELL_SIZE, GRID_WIDTH * CELL_SIZE, y * CELL_SIZE);
    }

    Pen borderPen(Color(80, 80, 80), 3);
    graphics.DrawRectangle(&borderPen, 1, 1, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);

    SolidBrush foodBrush(Color(255, 80, 80));
    graphics.FillRectangle(&foodBrush, foodX * CELL_SIZE + 2, foodY * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4);

    if (specialFoodActive) {
        SolidBrush specialFoodBrush(Color(255, 215, 0));
        graphics.FillRectangle(&specialFoodBrush, specialFoodX * CELL_SIZE + 2, specialFoodY * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4);
    }

    SolidBrush snakeHeadBrush(Color(100, 255, 100));
    SolidBrush snakeBodyBrush(Color(0, 200, 0));

    for (size_t i = 0; i < snake.size(); i++) {
        if (i == 0) {
            graphics.FillRectangle(&snakeHeadBrush, snake[i].x * CELL_SIZE + 2, snake[i].y * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        }
        else {
            graphics.FillRectangle(&snakeBodyBrush, snake[i].x * CELL_SIZE + 2, snake[i].y * CELL_SIZE + 2, CELL_SIZE - 4, CELL_SIZE - 4);
        }
    }

    FontFamily fontFamily(L"Arial");
    Font font(&fontFamily, 16, FontStyleBold, UnitPixel);
    SolidBrush textBrush(Color(255, 255, 255));
    std::wstring scoreText = L"Score: " + std::to_wstring(score);
    graphics.DrawString(scoreText.c_str(), -1, &font, PointF(10, GRID_HEIGHT * CELL_SIZE + 10), &textBrush);

    if (gameState == GAME_OVER) {
        Font bigFont(&fontFamily, 24, FontStyleBold, UnitPixel);
        SolidBrush gameOverBrush(Color(255, 100, 100));
        std::wstring gameOverText = L"GAME OVER";
        RectF layoutRect(0, (GRID_HEIGHT * CELL_SIZE) / 2 - 30, clientRect.right, 50);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        graphics.DrawString(gameOverText.c_str(), -1, &bigFont, layoutRect, &format, &gameOverBrush);
    }
    else if (gameState == PAUSED) {
        Font bigFont(&fontFamily, 24, FontStyleBold, UnitPixel);
        SolidBrush pausedBrush(Color(200, 200, 100));
        std::wstring pausedText = L"PAUSED";
        RectF layoutRect(0, (GRID_HEIGHT * CELL_SIZE) / 2 - 30, clientRect.right, 50);
        StringFormat format;
        format.SetAlignment(StringAlignmentCenter);
        graphics.DrawString(pausedText.c_str(), -1, &bigFont, layoutRect, &format, &pausedBrush);
    }

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hBufferDC, 0, 0, SRCCOPY);
}

void CleanupBuffer() {
    if (hBufferBitmap) {
        DeleteObject(hBufferBitmap);
        hBufferBitmap = NULL;
    }
    if (hBufferDC) {
        DeleteDC(hBufferDC);
        hBufferDC = NULL;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        srand((unsigned int)time(NULL));
        InitializeGame();
        SetTimer(hwnd, 1, speed, NULL);
        break;
    }

    case WM_SIZE:
        CleanupBuffer();
        break;

    case WM_KEYDOWN:
        if (gameState == GAME_OVER && wParam == VK_SPACE) {
            InitializeGame();
            SetTimer(hwnd, 1, speed, NULL);
            break;
        }

        if (wParam == 'P' || wParam == VK_PAUSE) {
            TogglePause();
            break;
        }

        if (gameState != RUNNING) break;

        switch (wParam) {
        case VK_UP:    if (snakeDirection != DOWN)  snakeDirection = UP;    break;
        case VK_DOWN:  if (snakeDirection != UP)    snakeDirection = DOWN;  break;
        case VK_LEFT:  if (snakeDirection != RIGHT) snakeDirection = LEFT;  break;
        case VK_RIGHT: if (snakeDirection != LEFT)  snakeDirection = RIGHT; break;
        }
        break;

    case WM_TIMER:
        if (gameState == RUNNING) {
            UpdateSnake();
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        DrawGame(hdc, clientRect);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        CleanupBuffer();
        Gdiplus::GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"SnakeGameClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassW(&wc);

    int windowWidth = GRID_WIDTH * CELL_SIZE + 20;
    int windowHeight = GRID_HEIGHT * CELL_SIZE + 80;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Snake Game",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
