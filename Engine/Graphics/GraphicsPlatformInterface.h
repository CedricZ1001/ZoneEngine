// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"
#include "Renderer.h"

namespace zone::graphics {

struct PlatformInterface
{
	bool(*initialize)(void);
	void(*shutdown)(void);
	void(*render)(void);
};

}