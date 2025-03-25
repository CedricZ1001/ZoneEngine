// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"
#include "..\Platform\Window.h"

namespace zone::graphics
{
DEFINE_TYPED_ID(surface_id)

class Surface
{
public:
	constexpr explicit Surface(surface_id id) : _id{ id } {};
	constexpr Surface() = default;
	constexpr surface_id getID() const { return _id; }
	constexpr bool isValid() const { return id::is_valid(_id); }

	void resize(uint32 width, uint32 height) const;
	uint32 width() const;
	uint32 height() const;
	void render() const;
private:
	surface_id _id{ id::invalid_id };
};

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
void render();

Surface createSurface(platform::Window window);
void removeSurface(surface_id id);
} 