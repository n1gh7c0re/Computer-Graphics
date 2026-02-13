#include "DX11Renderer.h"

DX11Renderer* g_renderer = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SIZE && g_renderer)
        if (wParam != SIZE_MINIMIZED)
            g_renderer->Resize(LOWORD(lParam), HIWORD(lParam));

    if (msg == WM_DESTROY)
        PostQuitMessage(0);

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance,
                      nullptr, LoadCursor(nullptr, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1),
                      nullptr, L"MinimalDX11Class" };
    RegisterClassEx(&wc);

    RECT rc = { 0, 0, 1280, 720 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(L"MinimalDX11Class", L"Тишковец Сергей – ДЗ 2",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    g_renderer = new DX11Renderer();
    g_renderer->Init(hWnd);

    ShowWindow(hWnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
            g_renderer->Render();
    }

    g_renderer->Cleanup();
    delete g_renderer;
    return (int)msg.wParam;
}