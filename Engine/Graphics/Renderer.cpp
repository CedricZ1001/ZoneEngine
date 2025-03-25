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

Surface createSurface(platform::Window window)
{
	return gfx.surface.create(window);
}

void removeSurface(surface_id id)
{
	assert(id::is_valid(id));
	gfx.surface.remove(id);
}

void Surface::resize(uint32 width, uint32 height) const 
{
	assert(isValid());
	gfx.surface.resize(_id, width, height);
}

uint32 Surface::width() const 
{
	assert(isValid());
	return gfx.surface.width(_id);
}

uint32 Surface::height() const
{
	assert(isValid());
	return gfx.surface.height(_id);
}

void Surface::render() const
{
	assert(isValid());
	gfx.surface.render(_id);
}

} // namespace zone::graphics