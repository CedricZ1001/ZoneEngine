// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.

#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include "TestRenderer.h"

#ifdef TEST_RENDERER

using namespace zone;

graphics::RenderSurface  _surfaces[4];

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		bool allClosed{ true };
		for (uint32 i{ 0 }; i < _countof(_surfaces); ++i)
		{
			if (!_surfaces[i].window.isClosed())
			{
				allClosed = false;
			}
		}
		if (allClosed)
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	break;
	case WM_SYSCHAR:
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
		{
			platform::Window win{ platform::window_id{(id::id_type)GetWindowLongPtr(hwnd,GWLP_USERDATA)} };
			win.setFullscreen(!win.isFullscreen());
			return 0;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void createRenderSurface(graphics::RenderSurface& surface, platform::WindowInitInfo info) 
{
	surface.window = platform::createWindow(&info);
}

void destroyRenderSurface(graphics::RenderSurface& surface)
{
	platform::removeWindow(surface.window.getID());
}

bool EngineTest::initialize()
{
	bool result{ graphics::initialize(graphics::GraphicsPlatform::direct3d12) };
	if (!result) return result;

	platform::WindowInitInfo info[]
	{
		{&winProc,nullptr,L"Test window 1",100,100,400,800},
		{&winProc,nullptr,L"Test window 2",350,150,400,800},
		{&winProc,nullptr,L"Test window 3",600,200,400,800},
		{&winProc,nullptr,L"Test window 4",950,250,400,800},
	};
	static_assert(_countof(info) == _countof(_surfaces));

	for (uint32 i{ 0 }; i < _countof(_surfaces); ++i)
	{
		createRenderSurface(_surfaces[i], info[i]);
	}
	return result;
}

void EngineTest::run()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	graphics::render();
}

void EngineTest::shutdown()
{
	for (uint32 i{ 0 }; i < _countof(_surfaces); ++i)
	{
		destroyRenderSurface(_surfaces[i]);
	}

	graphics::shutdown();
}

#endif