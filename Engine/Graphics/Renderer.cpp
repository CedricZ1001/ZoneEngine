// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "Renderer.h"
#include "GraphicsPlatformInterface.h"

namespace zone::graphics {
namespace {

PlatformInterface gfx{};

bool setPlatformInterface(GraphicsPlatform platform)
{

}

} // anonymous namespace

bool initialize(GraphicsPlatform platform)
{
	switch (platform)
	{
	case GraphicsPlatform::direct3d12:
		d3d12::getPlatformInterface(gfx);
		break;
	case GraphicsPlatform::vulkan:
		break;
	case GraphicsPlatform::opengl:
		break;
	default:
		break;
	}
}

void shutdown()
{
	gfx.shutdown();
}

}