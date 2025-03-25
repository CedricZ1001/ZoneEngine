// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "Platform.h"
#include "PlatformTypes.h"
namespace zone::platform
{
#ifdef _WIN64

namespace {

struct WindowInfo 
{
	HWND	hwnd{ nullptr };
	RECT	clientArea{ 0, 0, 1920, 1080 };
	RECT	fullScreenArea{};
	POINT	topLeft{ 0, 0 };
	DWORD	style{ WS_VISIBLE };
	bool	isFullScreen{ false };
	bool	isClosed{ false };
};

utl::vector<WindowInfo> windows;
//////////////////////////////////////////////////////////////
// TODO: this part will be handled by a free-list container later
utl::vector<uint32> avaliableSlots;

uint32 addToWindows(WindowInfo info) 
{
	uint32 id{ uint32_invalid_id };
	if (avaliableSlots.empty())
	{
		id = (uint32)windows.size();
		windows.emplace_back(info);
	}
	else 
	{
		id = avaliableSlots.back();
		avaliableSlots.pop_back();
		assert(id != uint32_invalid_id);
		windows[id] = info;
	}
	return id;
}

void removeFromWindows(uint32 id) 
{
	assert(id < windows.size());
	avaliableSlots.emplace_back(id);
}
//////////////////////////////////////////////////////////////

WindowInfo& getFromID(window_id id)
{
	assert(id < windows.size());
	assert(windows[id].hwnd);
	return windows[id];
}

WindowInfo& getFromHandle(window_handle handle) 
{
	const window_id id{ (id::id_type)GetWindowLongPtr(handle,GWLP_USERDATA) };
	return getFromID(id);
}

LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	WindowInfo* info{ nullptr };
	switch (msg)
	{
	case WM_DESTROY:
		getFromHandle(hwnd).isClosed = true;
		break;
	case WM_EXITSIZEMOVE:
		info = &getFromHandle(hwnd);
		break;
	case WM_SIZE:
		if (wparam == SIZE_MAXIMIZED)
		{
			info = &getFromHandle(hwnd);
		}
		break;
	case WM_SYSCOMMAND:
		if (wparam == SC_RESTORE)
		{
			info = &getFromHandle(hwnd);
		}
		break;
	default:
		break;
	}

	if (info)
	{
		assert(info->hwnd);
		GetClientRect(info->hwnd, info->isFullScreen ? &info->fullScreenArea : &info->clientArea);
	}

	LONG_PTR long_ptr{ GetWindowLongPtr(hwnd, 0) };
	return long_ptr ? ((window_proc)long_ptr)(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
}

void resizeWindow(const WindowInfo& info, const RECT& area)
{
	RECT windowRect{ area };
	AdjustWindowRect(&windowRect, info.style, FALSE);

	const int32 width{ windowRect.right - windowRect.left };
	const int32 height{ windowRect.bottom - windowRect.top };

	MoveWindow(info.hwnd, info.topLeft.x, info.topLeft.y, width, height, true);
}

void resizeWindow(window_id id, uint32 width, uint32 height)
{
	WindowInfo& info{ getFromID(id) };

	if (info.style & WS_CHILD)
	{
		GetClientRect(info.hwnd, &info.clientArea);
	}
	else{
		//NOTE: We also resize while in fullscreen mode to support the case
		//      when the user changed the screen resolution.
		RECT& area{ info.isFullScreen ? info.fullScreenArea : info.clientArea };
		area.bottom = area.top + height;
		area.right = area.left + width;

		resizeWindow(info, area);
	}
}

void setWindowFullscreen(window_id id, bool isFullscreen)
{
	WindowInfo& info{ getFromID(id) };
	if (info.isFullScreen != isFullscreen)
	{
		info.isFullScreen = isFullscreen;

		if (isFullscreen)
		{
			GetClientRect(info.hwnd, &info.clientArea);
			RECT rect;
			GetWindowRect(info.hwnd, &rect);
			info.topLeft.x = rect.left;
			info.topLeft.y = rect.top;
			SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
			ShowWindow(info.hwnd, SW_MAXIMIZE);
		}
		else
		{
			SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
			resizeWindow(info, info.clientArea);
			ShowWindow(info.hwnd, SW_SHOWNORMAL);
		}
	}
}

bool isWindowFullscreen(window_id id) 
{
	return getFromID(id).isFullScreen;
}

window_handle getWindowHandle(window_id id)
{
	return getFromID(id).hwnd;
}

void setWindowCaption(window_id id, const wchar_t* caption)
{
	WindowInfo& info{ getFromID(id) };
	SetWindowText(info.hwnd, caption);
}

math::Vec4U getWindowSize(window_id id)
{
	WindowInfo& info{ getFromID(id) };
	RECT& area{ info.isFullScreen ? info.fullScreenArea : info.clientArea };
	return{ (uint32)area.left,(uint32)area.top ,(uint32)area.right ,(uint32)area.bottom };
}

bool isWindowClose(window_id id) 
{
	return getFromID(id).isClosed;
}

}// anonymous namespace

Window createWindow(const WindowInitInfo* const initInfo)
{
	window_proc callback{ initInfo ? initInfo->callback : nullptr };
	window_handle parent{ initInfo ? initInfo->parent : nullptr };

	// Setup a window class
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = internal_window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = callback ? sizeof(callback) : 0;
	wc.hInstance = 0;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"ZoneWindow";
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Register the window class
	RegisterClassEx(&wc);
	WindowInfo info{};

	info.clientArea.right = (initInfo && initInfo->width) ? info.clientArea.left + initInfo->width : info.clientArea.right;
	info.clientArea.bottom = (initInfo && initInfo->height) ? info.clientArea.top + initInfo->height : info.clientArea.bottom;
	info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

	RECT rect{ info.clientArea };

	// adjust the window size for correct device size
	AdjustWindowRect(&rect, info.style, FALSE);

	const wchar_t* caption{ (initInfo && initInfo->caption) ? initInfo->caption : L"Zone Game" };
	const int32 left{ initInfo ? initInfo->left : info.topLeft.x };
	const int32 top{ initInfo ? initInfo->top : info.topLeft.y };
	const int32 width{ rect.right - rect.left };
	const int32 height{ rect.bottom - rect.top };

	// Create an instance of the window class
	 info.hwnd = CreateWindowEx(
		0,					// extended style
		wc.lpszClassName,	// window class name
		caption,			// instance title
		info.style,			// window style
		left, top,			// initial window position
		width, height,		// initial window dimensions
		parent,				// handle to parent window
		NULL,				// handle to menu
		NULL,				// instance of this application
		NULL				// extra creation parameters
	);

	 if (info.hwnd)
	 {
		 DEBUG_OP(SetLastError(0));
		 const window_id id{ addToWindows(info) };
		 SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);

		 if (callback)
		 {
			 SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
		 }
		 assert(GetLastError() == 0);
		 ShowWindow(info.hwnd, SW_SHOWNORMAL);
		 UpdateWindow(info.hwnd);
		 return Window{ id };
	 }
	 return {};
}

void removeWindow(window_id id)
{
	WindowInfo& info{ getFromID(id) };
	DestroyWindow(info.hwnd);
	removeFromWindows(id);
}
#else
#error "Must implement at least one platform"
#endif 

void Window::setFullscreen(bool _isFullscreen) const
{
	assert(isValid());
	setWindowFullscreen(_id, _isFullscreen);
}

bool Window::isFullscreen() const
{
	assert(isValid());
	return isWindowFullscreen(_id);
}

void* Window::handle() const
{
	assert(isValid());
	return getWindowHandle(_id);
}

void Window::setCaption(const wchar_t* caption) const
{
	assert(isValid());
	setWindowCaption(_id, caption);
}

math::Vec4U Window::size() const
{
	assert(isValid());
	return getWindowSize(_id);
}

void Window::resize(uint32 width, uint32 height) const
{
	assert(isValid());
	resizeWindow(_id, width, height);
}

uint32 Window::width() const
{
	math::Vec4U s{ size() };
	return s.z - s.x;
}

uint32 Window::height() const
{
	math::Vec4U s{ size() };
	return s.w - s.y;
}

bool Window::isClosed() const
{
	assert(isValid());
	return isWindowClose(_id);
}

}
