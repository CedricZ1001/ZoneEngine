// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"
#include "Graphics/Renderer.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")

namespace zone::graphics::d3d12 {
constexpr uint32 FRAME_BUFFER_COUNT{ 3 };
}

// Assert that COM call to D3D API succeeded
#ifdef _DEBUG
#ifndef DXCall
#define DXCall(x)									\
if(FAILED(x)){										\
	char lineNumber[32];							\
	sprintf_s(lineNumber, "%u", __LINE__);			\
	OutputDebugStringA("Error in: ");				\
	OutputDebugStringA(__FILE__);					\
	OutputDebugStringA("\nLine: ");					\
	OutputDebugStringA(lineNumber);					\
	OutputDebugStringA("\n");						\
	OutputDebugStringA(#x);							\
	OutputDebugStringA("\n");						\
	__debugbreak();									\
}
#endif // !DXCall
#else
#ifndef DXCall
#define DXCall(x) x
#endif // !DXCall
#endif // _DEBUG


#ifdef _DEBUG
// Name the D3D12 object and output the creation information
#define NAME_D3D12_OBJECT(obj, name){                          \
 (obj)->SetName(name);                                         \
 OutputDebugStringW(L"::D3D12 Object Created: ");              \
 OutputDebugStringW(name);                                     \
 OutputDebugStringW(L"\n");                                    \
}

// Name the indexed D3D12 object and output the creation information
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name){	\
wchar_t fullName[128];								\
if(swprintf_s(fullName,L"%s[%u]", name, n) >0){		\
	obj->SetName(fullName);							\
	OutputDebugString(L"::D3D12 Object Created: ");	\
	OutputDebugString(fullName);					\
	OutputDebugString(L"\n");						\
}}

// Output adapter information and Video RAM size
#define LOG_DXGI_ADAPTER(adapter){												\
DXGI_ADAPTER_DESC1 desc;														\
(adapter)->GetDesc1(&desc);														\
wchar_t fullName[128];															\
swprintf_s(fullName, 128, L"ADAPTER: %s\nVRAM SIZE: %Iu MB\n", desc.Description, (desc.DedicatedVideoMemory / (1024 * 1024)));	\
OutputDebugString(fullName);													\
}

#else
#define NAME_D3D12_OBJECT(x, name)
#define NAME_D3D12_OBJECT_INDEXED(obj, n, name)
#define LOG_DXGI_ADAPTER(adapter)
#endif // _DEBUG
