#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <assert.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

struct DX11Renderer
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* backBufferRTV = nullptr;

    UINT                    width = 1280;
    UINT                    height = 720;
    HWND                    hWnd = nullptr;

    void Init(HWND hwnd)
    {
        hWnd = hwnd;

        HRESULT hr;

        IDXGIFactory* factory = nullptr;
        hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
        assert(SUCCEEDED(hr));

        IDXGIAdapter* adapter = nullptr;
        UINT i = 0;
        while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
        {
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);
            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
                break;

            SAFE_RELEASE(adapter);
            i++;
        }
        assert(adapter != nullptr);

        UINT createDeviceFlags = 0;
#ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL featureLevel;

        hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
            createDeviceFlags, featureLevels, 1, D3D11_SDK_VERSION,
            &device, &featureLevel, &context);
        assert(SUCCEEDED(hr) && featureLevel == D3D_FEATURE_LEVEL_11_0);

        SAFE_RELEASE(adapter);

        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = 2;
        scd.BufferDesc.Width = width;
        scd.BufferDesc.Height = height;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferDesc.RefreshRate.Numerator = 0;
        scd.BufferDesc.RefreshRate.Denominator = 1;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hWnd;
        scd.SampleDesc.Count = 1;
        scd.SampleDesc.Quality = 0;
        scd.Windowed = TRUE;
        scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        hr = factory->CreateSwapChain(device, &scd, &swapChain);
        assert(SUCCEEDED(hr));

        SAFE_RELEASE(factory);

        CreateRenderTargetView();
    }

    void CreateRenderTargetView()
    {
        ID3D11Texture2D* backBuffer = nullptr;
        HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        assert(SUCCEEDED(hr));

        hr = device->CreateRenderTargetView(backBuffer, nullptr, &backBufferRTV);
        assert(SUCCEEDED(hr));

        SAFE_RELEASE(backBuffer);
    }

    void Resize(UINT newWidth, UINT newHeight)
    {
        if (newWidth == 0 || newHeight == 0 ||
            (newWidth == width && newHeight == height))
            return;

        width = newWidth;
        height = newHeight;

        SAFE_RELEASE(backBufferRTV);

        HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
        assert(SUCCEEDED(hr));

        CreateRenderTargetView();
    }

    void Render()
    {
        context->ClearState();

        ID3D11RenderTargetView* rtvs[] = { backBufferRTV };
        context->OMSetRenderTargets(1, rtvs, nullptr);

        const FLOAT clearColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
        context->ClearRenderTargetView(backBufferRTV, clearColor);

        swapChain->Present(1, 0);
    }

    void Cleanup()
    {
        SAFE_RELEASE(backBufferRTV);
        SAFE_RELEASE(swapChain);

#ifdef _DEBUG
        if (device)
        {
            ID3D11Debug* debug = nullptr;
            if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug)))
            {
                debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
                debug->Release();
            }
        }
#endif

        SAFE_RELEASE(context);
        SAFE_RELEASE(device);
    }
};

DX11Renderer* g_renderer = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (g_renderer && wParam != SIZE_MINIMIZED)
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            g_renderer->Resize(width, height);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MinimalDX11Class";
    RegisterClassEx(&wc);

    const int clientWidth = 1280;
    const int clientHeight = 720;

    RECT rc = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindow(
        L"MinimalDX11Class", L"Тишковец Сергей – ДЗ 1",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr);

    g_renderer = new DX11Renderer();
    g_renderer->Init(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            g_renderer->Render();
        }
    }

    g_renderer->Cleanup();
    delete g_renderer;

    return (int)msg.wParam;
}