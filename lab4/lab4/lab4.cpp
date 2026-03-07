#include "framework.h"
#include "lab4.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

// DirectX  global variables:
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;

ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11DepthStencilView* pDepthStencilView = nullptr;

ID3D11Buffer* m_pSceneBuffer = nullptr;
ID3D11Buffer* m_pGeomBuffer = nullptr;
ID3D11Buffer* m_pVertexBuffer = nullptr;
ID3D11Buffer* m_pIndexBuffer = nullptr;

ID3D11PixelShader* m_pPixelShader = nullptr;
ID3D11VertexShader* m_pVertexShader = nullptr;
ID3D11InputLayout* m_pInputLayout = nullptr;

ID3D11RasterizerState* m_pRasterizerState = nullptr;

ID3D11Buffer* m_pSphereGeomBuffer = nullptr;
ID3D11Buffer* m_pSphereVertexBuffer = nullptr;
ID3D11Buffer* m_pSphereIndexBuffer = nullptr;
ID3D11PixelShader* m_pSpherePixelShader = nullptr;
ID3D11VertexShader* m_pSphereVertexShader = nullptr;
ID3D11InputLayout* m_pSphereInputLayout = nullptr;
UINT m_sphereIndexCount = 0;

ID3D11Texture2D* m_pCubemapTexture = nullptr;
ID3D11ShaderResourceView* m_pCubemapView = nullptr;

ID3D11Texture2D* m_pTexture = nullptr;
ID3D11ShaderResourceView* m_pTextureView = nullptr;
ID3D11SamplerState* m_pSampler = nullptr;


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HRESULT             CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode = nullptr);

bool                Update();
void                Term();
bool                Render();
void                ResizeWindow(int width, int height);
void                RenderSphere();

HRESULT             InitDirectX(HWND hWnd);
BOOL                InitInstance(HINSTANCE, int);
HRESULT             InitScene();
HRESULT             InitSphere();
HRESULT             InitCubemap();


UINT WindowWidth = 800;
UINT WindowHeight = 500;

Camera m_camera;

bool m_rbPressed;
int m_prevMouseX;
int m_prevMouseY;
bool m_rotateModel;
double m_angle;

size_t m_prevUSec;

static const float CameraRotationSpeed = (float)M_PI * 3.0f;
static const float ModelRotationSpeed = (float)M_PI / 3.0f;


namespace
{
    void GetSphereDataSize(size_t latCells, size_t lonCells, size_t& indexCount, size_t& vertexCount)
    {
        vertexCount = (latCells + 1) * (lonCells + 1);
        indexCount = latCells * lonCells * 6;
    }

    void CreateSphere(size_t latCells, size_t lonCells, UINT16* pIndices, Point3f* pPos)
    {
        for (size_t lat = 0; lat < latCells + 1; lat++)
        {
            for (size_t lon = 0; lon < lonCells + 1; lon++)
            {
                int index = (int)(lat * (lonCells + 1) + lon);
                float lonAngle = 2.0f * (float)M_PI * lon / lonCells + (float)M_PI;
                float latAngle = -(float)M_PI / 2 + (float)M_PI * lat / latCells;

                Point3f r = Point3f{
                    sinf(lonAngle) * cosf(latAngle),
                    sinf(latAngle),
                    cosf(lonAngle) * cosf(latAngle)
                };

                pPos[index] = r * 0.5f;
            }
        }

        for (size_t lat = 0; lat < latCells; lat++)
        {
            for (size_t lon = 0; lon < lonCells; lon++)
            {
                size_t index = lat * lonCells * 6 + lon * 6;
                pIndices[index + 0] = (UINT16)(lat * (latCells + 1) + lon + 0);
                pIndices[index + 2] = (UINT16)(lat * (latCells + 1) + lon + 1);
                pIndices[index + 1] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
                pIndices[index + 3] = (UINT16)(lat * (latCells + 1) + lon + 1);
                pIndices[index + 5] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon + 1);
                pIndices[index + 4] = (UINT16)(lat * (latCells + 1) + latCells + 1 + lon);
            }
        }
    }

}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LAB4, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    std::wstring dir;
    dir.resize(MAX_PATH + 1);
    GetCurrentDirectory(MAX_PATH + 1, &dir[0]);
    size_t configPos = dir.find(L"x64");
    if (configPos != std::wstring::npos)
    {
        SetCurrentDirectory(dir.c_str());
    }

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HWND hWnd = FindWindow(L"MyClass", L"Тишковец Сергей – ДЗ 4");

    if (FAILED(InitDirectX(hWnd)))
    {
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();
        if (g_pRenderTargetView) g_pRenderTargetView->Release();
        if (g_pSwapChain) g_pSwapChain->Release();
        if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
        if (g_pd3dDevice) g_pd3dDevice->Release();

        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LAB4));

    MSG msg;

    bool exit = false;
    while (!exit)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT)
            {
                exit = true;
            }
        }
        if (Update())
        {
            Render();
        }
    }

    if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();

    Term();

    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
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
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LAB4));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LAB4);
    wcex.lpszClassName = L"MyClass";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(L"MyClass", L"Тишковец Сергей – ДЗ 4", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);


    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    //SetWindowText(hWnd, L"Тишковец Сергей – ДЗ 4");

    // Adjust window size
    {
        RECT rc;
        rc.left = 0;
        rc.right = WindowWidth;
        rc.top = 0;
        rc.bottom = WindowHeight;

        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

        MoveWindow(hWnd, 100, 100, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }

    return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_SIZE:
        if (g_pSwapChain && wParam != SIZE_MINIMIZED)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            ResizeWindow(rc.right - rc.left, rc.bottom - rc.top);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_LBUTTONDOWN:
        m_rbPressed = true;
        m_prevMouseX = LOWORD(lParam);
        m_prevMouseY = HIWORD(lParam);
        break;

    case WM_LBUTTONUP:
        m_rbPressed = false;
        break;

    case WM_MOUSEMOVE:
        if (m_rbPressed)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            int dx = x - m_prevMouseX;
            int dy = y - m_prevMouseY;

            m_camera.phi -= dx * 0.01f;
            m_camera.theta -= dy * 0.01f;

            m_prevMouseX = x;
            m_prevMouseY = y;
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


HRESULT InitDirectX(HWND hWnd)
{
    HRESULT result;
    IDXGIFactory* pFactory = nullptr;
    result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);

    IDXGIAdapter* pSelectedAdapter = NULL;
    if (SUCCEEDED(result))
    {
        IDXGIAdapter* pAdapter = NULL;
        UINT adapterIdx = 0;
        while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter)))
        {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);

            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
            {
                pSelectedAdapter = pAdapter;
                break;
            }

            SAFE_RELEASE(pAdapter);

            adapterIdx++;
        }
    }
    assert(pSelectedAdapter != NULL);

    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

    if (SUCCEEDED(result))
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        result = D3D11CreateDevice(pSelectedAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
            flags, levels, 1, D3D11_SDK_VERSION, &g_pd3dDevice, &level, &g_pd3dDeviceContext);
        assert(level == D3D_FEATURE_LEVEL_11_0);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Width = WindowWidth;
        swapChainDesc.BufferDesc.Height = WindowHeight;
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.OutputWindow = hWnd;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags = 0;

        result = pFactory->CreateSwapChain(g_pd3dDevice, &swapChainDesc, &g_pSwapChain);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        ID3D11Texture2D* pBackBuffer = nullptr;
        result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        assert(SUCCEEDED(result));

        if (FAILED(result))
            return result;

        result = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        SAFE_RELEASE(pBackBuffer);
    }

    ID3D11Texture2D* pDepthStencilTexture = nullptr;
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = WindowWidth;
    descDepth.Height = WindowHeight;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    result = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencilTexture);
    assert(SUCCEEDED(result));

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    result = g_pd3dDevice->CreateDepthStencilView(pDepthStencilTexture, &descDSV, &pDepthStencilView);
    assert(SUCCEEDED(result));

    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, pDepthStencilView);

    pDepthStencilTexture->Release();

    if (SUCCEEDED(result))
    {
        result = InitScene();
    }

    if (SUCCEEDED(result))
    {
        m_camera.poi = Point3f{ 0,0,0 };
        m_camera.r = 5.0f;
        m_camera.phi = -(float)M_PI / 6;
        m_camera.theta = (float)M_PI / 4;
    }

    SAFE_RELEASE(pFactory);
    SAFE_RELEASE(pSelectedAdapter);

    if (FAILED(result))
    {
        Term();
    }

    return result;
}


bool Render()
{
    g_pd3dDeviceContext->ClearState();

    ID3D11RenderTargetView* views[] = { g_pRenderTargetView };
    g_pd3dDeviceContext->OMSetRenderTargets(1, views, nullptr);

    static const float clearColor[4] = { 0.73f, 0.96f, 0.96f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    g_pd3dDeviceContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)WindowWidth;
    viewport.Height = (FLOAT)WindowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    g_pd3dDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = WindowWidth;
    rect.bottom = WindowHeight;
    g_pd3dDeviceContext->RSSetScissorRects(1, &rect);

    RenderSphere();

    g_pd3dDeviceContext->RSSetState(m_pRasterizerState);

    ID3D11SamplerState* samplers[] = { m_pSampler };
    g_pd3dDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_pTextureView };
    g_pd3dDeviceContext->PSSetShaderResources(0, 1, resources);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 20 };
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, m_pGeomBuffer };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
    g_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(36, 0, 0);

    HRESULT result = g_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(result));

    return SUCCEEDED(result);

}


void ResizeWindow(int width, int height)
{
    if (width != WindowWidth || height != WindowHeight)
    {
        SAFE_RELEASE(g_pRenderTargetView);

        HRESULT result = g_pSwapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            WindowWidth = width;
            WindowHeight = height;

            ID3D11Texture2D* pBackBuffer = NULL;
            result = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            assert(SUCCEEDED(result));
            if (SUCCEEDED(result))
            {
                result = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
                assert(SUCCEEDED(result));

                SAFE_RELEASE(pBackBuffer);
            }
        }
    }
}


HRESULT InitScene()
{
    static const TextureVertex Vertices[24] = {
        // Bottom face
        {-0.5, -0.5,  0.5, 0, 1},
        { 0.5, -0.5,  0.5, 1, 1},
        { 0.5, -0.5, -0.5, 1, 0},
        {-0.5, -0.5, -0.5, 0, 0},
        // Top face
        {-0.5,  0.5, -0.5, 0, 1},
        { 0.5,  0.5, -0.5, 1, 1},
        { 0.5,  0.5,  0.5, 1, 0},
        {-0.5,  0.5,  0.5, 0, 0},
        // Front face
        { 0.5, -0.5, -0.5, 0, 1},
        { 0.5, -0.5,  0.5, 1, 1},
        { 0.5,  0.5,  0.5, 1, 0},
        { 0.5,  0.5, -0.5, 0, 0},
        // Back face
        {-0.5, -0.5,  0.5, 0, 1},
        {-0.5, -0.5, -0.5, 1, 1},
        {-0.5,  0.5, -0.5, 1, 0},
        {-0.5,  0.5,  0.5, 0, 0},
        // Left face
        { 0.5, -0.5,  0.5, 0, 1},
        {-0.5, -0.5,  0.5, 1, 1},
        {-0.5,  0.5,  0.5, 1, 0},
        { 0.5,  0.5,  0.5, 0, 0},
        // Right face
        {-0.5, -0.5, -0.5, 0, 1},
        { 0.5, -0.5, -0.5, 1, 1},
        { 0.5,  0.5, -0.5, 1, 0},
        {-0.5,  0.5, -0.5, 0, 0}
    };
    static const UINT16 Indices[36] = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22
    };

    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };


    HRESULT result = S_OK;

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Vertices;
        data.SysMemPitch = sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pVertexBuffer, "VertexBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pIndexBuffer, "IndexBuffer");
        }
    }

    ID3DBlob* pVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTexture.vs", (ID3D11DeviceChild**)&m_pVertexShader, &pVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SimpleTexture.ps", (ID3D11DeviceChild**)&m_pPixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 2, pVertexShaderCode->GetBufferPointer(), pVertexShaderCode->GetBufferSize(), &m_pInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pInputLayout, "InputLayout");
        }
    }

    SAFE_RELEASE(pVertexShaderCode);

    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(GeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        GeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixIdentity();

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBuffer;
        data.SysMemPitch = sizeof(geomBuffer);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pGeomBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pGeomBuffer, "GeomBuffer");
        }
    }

    // Create scene buffer
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SceneBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, nullptr, &m_pSceneBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSceneBuffer, "SceneBuffer");
        }
    }

    if (SUCCEEDED(result))
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.AntialiasedLineEnable = FALSE;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthBias = 0;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthBiasClamp = 0.0f;
        desc.DepthClipEnable = TRUE;
        desc.ScissorEnable = FALSE;
        desc.MultisampleEnable = FALSE;

        result = g_pd3dDevice->CreateRasterizerState(&desc, &m_pRasterizerState);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pRasterizerState, "RasterizerState");
        }
    }
    if (SUCCEEDED(result))
    {
        ID3D11DepthStencilState* pDepthStencilState = nullptr;
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
        depthStencilDesc.StencilEnable = FALSE;

        result = g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);
        assert(SUCCEEDED(result));

        g_pd3dDeviceContext->OMSetDepthStencilState(pDepthStencilState, 0);

        SAFE_RELEASE(pDepthStencilState);
    }

    // Load texture
    DXGI_FORMAT textureFmt;
    if (SUCCEEDED(result))
    {
        const std::wstring TextureName = L"bmw.dds";

        TextureDesc textureDesc;
        bool ddsRes = LoadDDS(TextureName.c_str(), textureDesc);

        textureFmt = textureDesc.fmt;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureDesc.fmt;
        desc.ArraySize = 1;
        desc.MipLevels = textureDesc.mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = textureDesc.height;
        desc.Width = textureDesc.width;

        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);
        const char* pSrcData = reinterpret_cast<const char*>(textureDesc.pData);

        std::vector<D3D11_SUBRESOURCE_DATA> data;
        data.resize(desc.MipLevels);
        for (UINT32 i = 0; i < desc.MipLevels; i++)
        {
            data[i].pSysMem = pSrcData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;

            pSrcData += pitch * blockHeight;
            blockHeight = std::max(1u, blockHeight / 2);
            blockWidth = std::max(1u, blockWidth / 2);
            pitch = blockWidth * GetBytesPerBlock(desc.Format);
        }
        result = g_pd3dDevice->CreateTexture2D(&desc, data.data(), &m_pTexture);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pTexture, WCSToMBS(TextureName));
        }

        free(textureDesc.pData);
    }
    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.Format = textureFmt;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 11;
        desc.Texture2D.MostDetailedMip = 0;

        //result = g_pd3dDevice->CreateShaderResourceView(m_pTexture, nullptr, &m_pTextureView);
        result = g_pd3dDevice->CreateShaderResourceView(m_pTexture, &desc, &m_pTextureView);
        assert(SUCCEEDED(result));
    }
    if (SUCCEEDED(result))
    {
        D3D11_SAMPLER_DESC desc = {};

        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD = -FLT_MAX;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;

        result = g_pd3dDevice->CreateSamplerState(&desc, &m_pSampler);
        assert(SUCCEEDED(result));
    }

    if (SUCCEEDED(result))
    {
        result = InitSphere();
        assert(SUCCEEDED(result));
    }
    if (SUCCEEDED(result))
    {
        result = InitCubemap();
        assert(SUCCEEDED(result));
    }

    return result;
}


HRESULT CompileAndCreateShader(const std::wstring& path, ID3D11DeviceChild** ppShader, ID3DBlob** ppCode)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, path.c_str(), L"rb");
    assert(pFile != nullptr);
    if (pFile == nullptr)
    {
        return E_FAIL;
    }

    fseek(pFile, 0, SEEK_END);
    long long size = _ftelli64(pFile);
    fseek(pFile, 0, SEEK_SET);

    std::vector<char> data;
    data.resize(size + 1);

    size_t rd = fread(data.data(), 1, size, pFile);
    assert(rd == (size_t)size);

    fclose(pFile);

    std::wstring ext = Extension(path);

    std::string entryPoint = "";
    std::string platform = "";

    if (ext == L"vs")
    {
        entryPoint = "vs";
        platform = "vs_5_0";
    }
    else if (ext == L"ps")
    {
        entryPoint = "ps";
        platform = "ps_5_0";
    }

    // Setup flags
    UINT flags1 = 0;
#ifdef _DEBUG
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

    // Try to compile
    ID3DBlob* pCode = nullptr;
    ID3DBlob* pErrMsg = nullptr;
    HRESULT result = D3DCompile(data.data(), data.size(), WCSToMBS(path).c_str(), nullptr, nullptr, entryPoint.c_str(), platform.c_str(), flags1, 0, &pCode, &pErrMsg);
    if (!SUCCEEDED(result) && pErrMsg != nullptr)
    {
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());
    }
    assert(SUCCEEDED(result));

    if (pErrMsg != nullptr)
    {
        pErrMsg->Release();
        pErrMsg = nullptr;

    }

    // Create shader itself if anything else is OK
    if (SUCCEEDED(result))
    {
        if (ext == L"vs")
        {
            ID3D11VertexShader* pVertexShader = nullptr;
            result = g_pd3dDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &pVertexShader);
            if (SUCCEEDED(result))
            {
                *ppShader = pVertexShader;
            }
        }
        else if (ext == L"ps")
        {
            ID3D11PixelShader* pPixelShader = nullptr;
            result = g_pd3dDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, &pPixelShader);
            if (SUCCEEDED(result))
            {
                *ppShader = pPixelShader;
            }
        }
    }
    if (SUCCEEDED(result))
    {
        result = SetResourceName(*ppShader, WCSToMBS(path).c_str());

    }

    if (ppCode)
    {
        *ppCode = pCode;
    }
    else
    {
        pCode->Release();
    }

    return result;
}


bool Update() {
    size_t usec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (m_prevUSec == 0)
    {
        m_prevUSec = usec;
    }

    double deltaSec = (usec - m_prevUSec) / 1000000.0;

    ///

    m_angle = m_angle + deltaSec * ModelRotationSpeed;

    GeomBuffer geomBuffer;

    DirectX::XMMATRIX m = DirectX::XMMatrixRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), -(float)m_angle);

    geomBuffer.m = m;

    g_pd3dDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &geomBuffer, 0, 0);

    m_prevUSec = usec;


    DirectX::XMMATRIX v;
    Point4f cameraPos;
    {
        Point3f pos = m_camera.poi + Point3f{ cosf(m_camera.theta) * cosf(m_camera.phi), sinf(m_camera.theta), cosf(m_camera.theta) * sinf(m_camera.phi) } * m_camera.r;
        float upTheta = m_camera.theta + (float)M_PI / 2;
        Point3f up = Point3f{ cosf(upTheta) * cosf(m_camera.phi), sinf(upTheta), cosf(upTheta) * sinf(m_camera.phi) };

        v = DirectX::XMMatrixLookAtLH(
            DirectX::XMVectorSet(pos.x, pos.y, pos.z, 0.0f),
            DirectX::XMVectorSet(m_camera.poi.x, m_camera.poi.y, m_camera.poi.z, 0.0f),
            DirectX::XMVectorSet(up.x, up.y, up.z, 0.0f)
        );

        cameraPos = pos;
    }

    float f = 100.0f;
    float n = 0.1f;
    float fov = (float)M_PI / 3;
    float c = 1.0f / tanf(fov / 2);
    float aspectRatio = (float)WindowHeight / WindowWidth;
    DirectX::XMMATRIX p = DirectX::XMMatrixPerspectiveLH(tanf(fov / 2) * 2 * n, tanf(fov / 2) * 2 * n * aspectRatio, n, f);

    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = g_pd3dDeviceContext->Map(m_pSceneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    assert(SUCCEEDED(result));
    if (SUCCEEDED(result))
    {
        SceneBuffer& sceneBuffer = *reinterpret_cast<SceneBuffer*>(subresource.pData);

        sceneBuffer.vp = DirectX::XMMatrixMultiply(v, p);
        sceneBuffer.cameraPos = cameraPos;

        g_pd3dDeviceContext->Unmap(m_pSceneBuffer, 0);
    }

    return SUCCEEDED(result);
}

void Term()
{
    SAFE_RELEASE(m_pRasterizerState);

    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pVertexShader);

    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);

    SAFE_RELEASE(m_pSceneBuffer);
    SAFE_RELEASE(m_pGeomBuffer);

    SAFE_RELEASE(g_pRenderTargetView);
    SAFE_RELEASE(g_pSwapChain);
    SAFE_RELEASE(g_pd3dDeviceContext);
    SAFE_RELEASE(pDepthStencilView);
    SAFE_RELEASE(m_pTexture);
    SAFE_RELEASE(m_pTextureView);
    SAFE_RELEASE(m_pSampler);
    SAFE_RELEASE(m_pCubemapTexture);
    SAFE_RELEASE(m_pCubemapView);
    SAFE_RELEASE(m_pSphereGeomBuffer);
    SAFE_RELEASE(m_pSphereVertexBuffer);
    SAFE_RELEASE(m_pSphereIndexBuffer);
    SAFE_RELEASE(m_pSphereVertexShader);
    SAFE_RELEASE(m_pSpherePixelShader);
    SAFE_RELEASE(m_pSphereInputLayout);

#ifdef _DEBUG
    if (g_pd3dDevice != nullptr)
    {
        ID3D11Debug* pDebug = nullptr;
        HRESULT result = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&pDebug);
        assert(SUCCEEDED(result));
        if (pDebug != nullptr)
        {
            if (pDebug->AddRef() != 3)
            {
                pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
            }
            pDebug->Release();
            SAFE_RELEASE(pDebug);
        }
    }
#endif // _DEBUG

    SAFE_RELEASE(g_pd3dDevice);
}


HRESULT InitSphere()
{
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    HRESULT result = S_OK;

    static const size_t SphereSteps = 32;

    std::vector<Point3f> sphereVertices;
    std::vector<UINT16> indices;

    size_t indexCount;
    size_t vertexCount;

    GetSphereDataSize(SphereSteps, SphereSteps, indexCount, vertexCount);

    sphereVertices.resize(vertexCount);
    indices.resize(indexCount);

    m_sphereIndexCount = (UINT)indexCount;

    CreateSphere(SphereSteps, SphereSteps, indices.data(), sphereVertices.data());

    // Create vertex buffer
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(sphereVertices.size() * sizeof(Point3f));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = sphereVertices.data();
        data.SysMemPitch = (UINT)(sphereVertices.size() * sizeof(Point3f));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereVertexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereVertexBuffer, "SphereVertexBuffer");
        }
    }

    // Create index buffer
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)(indices.size() * sizeof(UINT16));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = indices.data();
        data.SysMemPitch = (UINT)(indices.size() * sizeof(UINT16));
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereIndexBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereIndexBuffer, "SphereIndexBuffer");
        }
    }

    ID3DBlob* pSphereVertexShaderCode = nullptr;
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SphereTexture.vs", (ID3D11DeviceChild**)&m_pSphereVertexShader, &pSphereVertexShaderCode);
    }
    if (SUCCEEDED(result))
    {
        result = CompileAndCreateShader(L"SphereTexture.ps", (ID3D11DeviceChild**)&m_pSpherePixelShader);
    }

    if (SUCCEEDED(result))
    {
        result = g_pd3dDevice->CreateInputLayout(InputDesc, 1, pSphereVertexShaderCode->GetBufferPointer(), pSphereVertexShaderCode->GetBufferSize(), &m_pSphereInputLayout);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereInputLayout, "SphereInputLayout");
        }
    }

    //SAFE_RELEASE(pSphereVertexShaderCode);
    pSphereVertexShaderCode->Release();

    // Create geometry buffer
    if (SUCCEEDED(result))
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SphereGeomBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        SphereGeomBuffer geomBuffer;
        geomBuffer.m = DirectX::XMMatrixIdentity();
        geomBuffer.size.x = 2.0f;
        //geomBuffer.m = DirectX::XMMatrixTranslation(2.0f, 0.0f, 0.0f);

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBuffer;
        data.SysMemPitch = sizeof(geomBuffer);
        data.SysMemSlicePitch = 0;

        result = g_pd3dDevice->CreateBuffer(&desc, &data, &m_pSphereGeomBuffer);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pSphereGeomBuffer, "SphereGeomBuffer");
        }
    }

    return result;
}


HRESULT InitCubemap()
{
    HRESULT result = S_OK;

    DXGI_FORMAT textureFmt;
    if (SUCCEEDED(result))
    {
        const std::wstring TextureNames[6] =
        {
            L"px.dds", L"nx.dds",
            L"py.dds", L"ny.dds",
            L"pz.dds", L"nz.dds"
        };
        TextureDesc texDescs[6];
        bool ddsRes = true;
        for (int i = 0; i < 6 && ddsRes; i++)
        {
            ddsRes = LoadDDS(TextureNames[i].c_str(), texDescs[i], true);
        }

        textureFmt = texDescs[0].fmt; // Assume all are the same

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Format = textureFmt;
        desc.ArraySize = 6;
        desc.MipLevels = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Height = texDescs[0].height;
        desc.Width = texDescs[0].width;

        UINT32 blockWidth = DivUp(desc.Width, 4u);
        UINT32 blockHeight = DivUp(desc.Height, 4u);
        UINT32 pitch = blockWidth * GetBytesPerBlock(desc.Format);

        D3D11_SUBRESOURCE_DATA data[6];
        for (int i = 0; i < 6; i++)
        {
            data[i].pSysMem = texDescs[i].pData;
            data[i].SysMemPitch = pitch;
            data[i].SysMemSlicePitch = 0;
        }
        result = g_pd3dDevice->CreateTexture2D(&desc, data, &m_pCubemapTexture);
        assert(SUCCEEDED(result));
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapTexture, "CubemapTexture");
        }
        for (int i = 0; i < 6; i++)
        {
            free(texDescs[i].pData);
        }
    }
    if (SUCCEEDED(result))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format = textureFmt;
        desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
        desc.TextureCube.MipLevels = 1;
        desc.TextureCube.MostDetailedMip = 0;

        result = g_pd3dDevice->CreateShaderResourceView(m_pCubemapTexture, &desc, &m_pCubemapView);
        if (SUCCEEDED(result))
        {
            result = SetResourceName(m_pCubemapView, "CubemapView");
        }
    }

    return result;
}


void RenderSphere()
{
    ID3D11SamplerState* samplers[] = { m_pSampler };
    g_pd3dDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_pCubemapView };
    g_pd3dDeviceContext->PSSetShaderResources(0, 1, resources);

    g_pd3dDeviceContext->IASetIndexBuffer(m_pSphereIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { m_pSphereVertexBuffer };
    UINT strides[] = { 12 };
    UINT offsets[] = { 0 };
    ID3D11Buffer* cbuffers[] = { m_pSceneBuffer, m_pSphereGeomBuffer };
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    g_pd3dDeviceContext->IASetInputLayout(m_pSphereInputLayout);
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pd3dDeviceContext->VSSetShader(m_pSphereVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 2, cbuffers);
    g_pd3dDeviceContext->PSSetShader(m_pSpherePixelShader, nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(m_sphereIndexCount, 0, 0);
}