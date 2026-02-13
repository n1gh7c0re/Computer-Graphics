#include "DX11Renderer.h"
#include <assert.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// Шейдеры
const char* g_VSCode = R"(
struct VSInput { float3 pos : POSITION; float4 color : COLOR; };
struct VSOutput { float4 pos : SV_Position; float4 color : COLOR; };
VSOutput vs(VSInput v) { VSOutput o; o.pos = float4(v.pos, 1.0f); o.color = v.color; return o; }
)";

const char* g_PSCode = R"(
struct VSOutput { float4 pos : SV_Position; float4 color : COLOR; };
float4 ps(VSOutput p) : SV_Target0 { return p.color; }
)";

static const Vertex Vertices[] =
{
    { -0.5f, -0.5f, 0.0f, 0xFF0000FF },
    {  0.5f, -0.5f, 0.0f, 0xFF00FF00 },
    {  0.0f,  0.5f, 0.0f, 0xFFFF0000 }
};

static const unsigned short Indices[] = { 0, 2, 1 };

void DX11Renderer::Init(HWND hwnd)
{
    hWnd = hwnd;

    IDXGIFactory* factory = nullptr;
    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);

    IDXGIAdapter* adapter = nullptr;
    UINT i = 0;
    while (factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);
        if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0) break;
        adapter->Release();
        i++;
    }

    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL level;
    D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
        levels, 1, D3D11_SDK_VERSION, &device, &level, &context);

    adapter->Release();
    factory->Release();

    // Swap Chain
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    factory->CreateSwapChain(device, &scd, &swapChain);
    factory->Release();

    CreateRenderTargetView();
    CreateGeometry();
    CreateShaders();
}

void DX11Renderer::CreateRenderTargetView()
{
    ID3D11Texture2D* bb = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    device->CreateRenderTargetView(bb, nullptr, &backBufferRTV);
    bb->Release();
}

void DX11Renderer::CreateGeometry()
{
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(Vertices);
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA data = { Vertices };
    device->CreateBuffer(&bd, &data, &vertexBuffer);

    bd.ByteWidth = sizeof(Indices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    data.pSysMem = Indices;
    device->CreateBuffer(&bd, &data, &indexBuffer);
}

HRESULT DX11Renderer::CompileShader(const char* source, const char* entry, const char* target, ID3DBlob** blob)
{
    ID3DBlob* errors = nullptr;
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entry, target, flags, 0, blob, &errors);
    if (errors)
    {
        OutputDebugStringA((const char*)errors->GetBufferPointer());
        errors->Release();
    }
    return hr;
}

void DX11Renderer::CreateShaders()
{
    ID3DBlob* vsBlob = nullptr;
    CompileShader(g_VSCode, "vs", "vs_5_0", &vsBlob);
    device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    vsBlob->Release();

    ID3DBlob* psBlob = nullptr;
    CompileShader(g_PSCode, "ps", "ps_5_0", &psBlob);
    device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    psBlob->Release();
}

void DX11Renderer::Resize(UINT newW, UINT newH)
{
    if (newW == 0 || newH == 0 || (newW == width && newH == height)) return;
    width = newW; height = newH;

    SAFE_RELEASE(backBufferRTV);
    swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    CreateRenderTargetView();
}

void DX11Renderer::Render()
{
    context->ClearState();
    ID3D11RenderTargetView* rtvs[] = { backBufferRTV };
    context->OMSetRenderTargets(1, rtvs, nullptr);

    const FLOAT clear[4] = { 0.1f, 0.2f, 0.4f, 1.0f };
    context->ClearRenderTargetView(backBufferRTV, clear);

    D3D11_VIEWPORT vp = { 0, 0, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
    context->RSSetViewports(1, &vp);

    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vbs[] = { vertexBuffer };
    UINT strides[] = { sizeof(Vertex) };
    UINT offsets[] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(inputLayout);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(vertexShader, nullptr, 0);
    context->PSSetShader(pixelShader, nullptr, 0);

    context->DrawIndexed(3, 0, 0);
    swapChain->Present(1, 0);
}

void DX11Renderer::Cleanup()
{
#ifdef _DEBUG
    ID3D11Debug* debug = nullptr;
    if (SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug)))
    {
        debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        debug->Release();
    }
#endif

    SAFE_RELEASE(pixelShader);
    SAFE_RELEASE(vertexShader);
    SAFE_RELEASE(inputLayout);
    SAFE_RELEASE(indexBuffer);
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(backBufferRTV);
    SAFE_RELEASE(swapChain);
    SAFE_RELEASE(context);
    SAFE_RELEASE(device);
}