// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "D3D12CommonHeaders.h"


namespace zone::graphics::d3d12::core {

bool initialize();
void shutdown();
void render();

template<typename T>
constexpr void release(T*& resource)
{
	if (resource)
	{
		resource->Release();
		resource = nullptr;
	}
}

namespace detail {
	void deferredRelease(IUnknown* resource);
}

template<typename T>
constexpr void deferredRelease(T*& resource)
{
	if (resource)
	{
		detail::deferredRelease(resource);
		resource = nullptr;
	}
}

ID3D12Device *const getDevice();
uint32 currentFrameIndex();
void setDeferredReleasesFlag();
}