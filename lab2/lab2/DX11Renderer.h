#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

struct Vertex
{
    float x, y, z;
    unsigned int color;        // R8G8B8A8_UNORM
};

struct DX11Renderer
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* backBufferRTV = nullptr;

    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Buffer* indexBuffer = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;

    UINT                    width = 1280;
    UINT                    height = 720;
    HWND                    hWnd = nullptr;

    void Init(HWND hwnd);
    void CreateRenderTargetView();
    void CreateGeometry();
    void CreateShaders();
    void Resize(UINT newW, UINT newH);
    void Render();
    void Cleanup();

private:
    HRESULT CompileShader(const char* source, const char* entry, const char* target, ID3DBlob** blob);
};