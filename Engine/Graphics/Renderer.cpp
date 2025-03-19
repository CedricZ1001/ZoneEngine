// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "Renderer.h"
#include "GraphicsPlatformInterface.h"
#include "Direct3D12/D3D12Interface.h"

namespace zone::graphics {
namespace {

PlatformInterface gfx{};

bool setPlatformInterface(GraphicsPlatform platform)
{
	switch (platform)
	{
	case GraphicsPlatform::direct3d12:
		d3d12::getPlatformInterface(gfx);
		break;
	case GraphicsPlatform::vulkan:
	case GraphicsPlatform::opengl:
	default:
		return false;
	}
	return true;
}

} // anonymous namespace

bool initialize(GraphicsPlatform platform)
{
	return setPlatformInterface(platform) && gfx.initialize();
}

void shutdown()
{
	gfx.shutdown();
}

void render()
{
	//gfx.render();
}

}