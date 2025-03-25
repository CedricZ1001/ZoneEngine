// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"
#include "Platform/Window.h"

namespace zone::graphics {

struct PlatformInterface
{
	bool(*initialize)(void);
	void(*shutdown)(void);

	struct {
		Surface(*create)(platform::Window);
		void(*remove)(surface_id);
		void(*resize)(surface_id, uint32, uint32);
		uint32 (*width)(surface_id);
		uint32 (*height)(surface_id);
		void(*render)(surface_id);
	} surface;
};

}