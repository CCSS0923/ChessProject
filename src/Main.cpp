#include <windowsx.h>
#include <windows.h>
#include "Gui/GuiManager.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

GuiManager g_GuiManager; // 전역 인스턴스

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        g_GuiManager.Initialize(hWnd);
        return 0;

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        g_GuiManager.OnSize(width, height);
        return 0;
    }
    case WM_KEYDOWN:
        g_GuiManager.OnKeyDown((UINT)wParam);
        return 0;
    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        // withShift 파라미터 전달
        g_GuiManager.OnLButtonDown(x, y, (wParam & MK_SHIFT) != 0);
        return 0;
    }

    case WM_LBUTTONUP:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        g_GuiManager.OnLButtonUp(x, y);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        bool leftDown = (wParam & MK_LBUTTON) != 0;
        g_GuiManager.OnMouseMove(x, y, leftDown);
        return 0;
    }

    case WM_TIMER:
        g_GuiManager.OnTimer((UINT)wParam);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        g_GuiManager.OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// wWinMain은 기존과 동일하므로 생략 가능하지만, 완성도를 위해 포함
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"ChessWindowClass";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassExW(&wc))
        return 0;

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"ChessProject - Win32 + OpenCV + Stockfish",
        WS_OVERLAPPEDWINDOW, // WS_MAXIMIZEBOX 제거하지 않음 (리사이즈 테스트용)
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 900,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}