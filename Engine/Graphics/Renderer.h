// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"
#include "..\Platform\Window.h"

namespace zone::graphics
{
class Surface
{};

struct RenderSurface 
{
	platform::Window window{};
	Surface surface{};
};

enum class GraphicsPlatform : uint32
{
	direct3d12 = 0,
	vulkan = 1,
	opengl = 2, // etc
};

bool initialize(GraphicsPlatform platform);
void shutdown();

} 