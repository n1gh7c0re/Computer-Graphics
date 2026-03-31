// header.h: включаемый файл для стандартных системных включаемых файлов
// или включаемые файлы для конкретного проекта
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN    
#define NOMINMAX

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>

#include <string>
#include <vector>

#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>

#include <chrono>

#define MAX_LOADSTRING 100
#define _USE_MATH_DEFINES

#include <math.h>

#include "Matrix.h"
#include "DDS.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"

#define SAFE_RELEASE(p)\
{\
    if (p != nullptr)\
    {\
        p->Release();\
        p = nullptr;\
    }\
}

inline HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name)
{
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}

inline std::wstring Extension(const std::wstring& filename)
{
    size_t dotPos = filename.rfind(L'.');
    if (dotPos != std::wstring::npos)
    {
        return filename.substr(dotPos + 1);
    }
    return L"";
}

inline std::string WCSToMBS(const std::wstring& wstr)
{
    size_t len = wstr.length();

    // We suppose that on Windows platform wchar_t is 2 byte length
    std::vector<char> res;
    res.resize(len + 1);

    size_t resLen = 0;
    wcstombs_s(&resLen, res.data(), res.size(), wstr.c_str(), len);

    return res.data();
}

template <typename T>
T DivUp(const T& a, const T& b)
{
    return (a + b - (T)1) / b;
}

inline UINT32 GetBytesPerBlock(const DXGI_FORMAT& fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 8;
        break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        return 16;
        break;

    default:
        OutputDebugStringA(("Unsupported texture format: " + std::to_string((int)fmt)).c_str());
        return 4;
    }
    assert(0);
    return 0;
}

inline double randNorm()
{
    return (double)rand() / RAND_MAX;
}

inline float randNormf()
{
    return (float)rand() / RAND_MAX;
}


struct Camera
{
    Point3f poi;
    float r;
    float phi;
    float theta;
};

struct Vertex
{
    float x, y, z;
    COLORREF color;
};


struct TextureVertex
{
    float x, y, z;
    float u, v;
};

struct ColorVertex
{
    float x, y, z;
    COLORREF color;
};

struct TextureTangentVertex
{
    Point3f pos;
    Point3f tangent;
    Point3f norm;
    Point2f uv;
};

struct GeomBuffer
{
    DirectX::XMMATRIX m;
    DirectX::XMMATRIX normalM;
    Point4f shineSpeedTexIdNM;
    Point4f posAngle;
};

struct Light
{
    Point4f pos = Point4f{ 0,0,0,0 };
    Point4f color = Point4f{ 1,1,1,0 };
};


struct SceneBuffer
{
    DirectX::XMMATRIX vp;
    Point4f cameraPos;
    Point4i lightCount;
    Point4i postProcess; 
    Light lights[10];
    Point4f ambientColor;
};

struct AABB
{
    Point3f vmin = Point3f{
        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()
    };
    Point3f vmax = Point3f{
        std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()
    };

    inline Point3f GetVert(int idx) const
    {
        return Point3f
        {
            (idx & 1) == 0 ? vmin.x : vmax.x,
            (idx & 2) == 0 ? vmin.y : vmax.y,
            (idx & 4) == 0 ? vmin.z : vmax.z
        };
    }
};

struct SphereGeomBuffer
{
    DirectX::XMMATRIX m;
    Point4f size;
};

struct RectGeomBuffer
{
    DirectX::XMMATRIX m;
    Point4f color;
};

