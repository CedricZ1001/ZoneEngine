// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#if !defined(SHIPPING)
#include "..\Content\ContentLoader.h"
#include "..\Components\Script.h"
#include "..\Platform\PlatformTypes.h"
#include "..\Platform\Platform.h"
#include "..\Graphics\Renderer.h"
#include <thread>

using namespace zone;

namespace {

graphics::RenderSurface gameWindow{};

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		if (gameWindow.window.isClosed())
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	break;
	case WM_SYSCHAR:
		if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
		{
			gameWindow.window.setFullscreen(!gameWindow.window.isFullscreen());
			return 0;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
} // anonymous namespace

bool engine_initialize()
{
	if ((!zone::content::load_game()))return false;

	platform::WindowInitInfo info
	{
		&winProc, nullptr, L"Game Window" // TODO: get the game name from the loaded game file
	};

	gameWindow.window = platform::createWindow(&info);
	if (!gameWindow.window.isValid()) return false;

	return true;
}

void engine_update()
{
	zone::script::update(10.f);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown()
{
	platform::removeWindow(gameWindow.window.getID());
	zone::content::unload_game();
}
#endif // !defined(SHIPPING)