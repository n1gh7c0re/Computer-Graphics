
#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             
// Файлы заголовков Windows
#include <windows.h>
// Файлы заголовков среды выполнения C
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

    std::vector<char> res;
    res.resize(len + 1);

    size_t resLen = 0;
    wcstombs_s(&resLen, res.data(), res.size(), wstr.c_str(), len);

    return res.data();
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

struct GeomBuffer
{
    DirectX::XMMATRIX m;
};

struct SceneBuffer
{
    DirectX::XMMATRIX vp;
};

