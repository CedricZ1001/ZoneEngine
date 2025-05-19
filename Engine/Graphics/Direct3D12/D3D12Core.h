#pragma once
#include "D3D12CommonHeaders.h"

namespace zone::graphics::d3d12 {
class DescriptorHeap;
}

namespace zone::graphics::d3d12::core {

bool initialize();
void shutdown();

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
DescriptorHeap& rtvHeap();
DescriptorHeap& dsvHeap();
DescriptorHeap& srvHeap();
DescriptorHeap& uavHeap();
DXGI_FORMAT defaultRenderTargetFormat();
uint32 currentFrameIndex();
void setDeferredReleasesFlag();

Surface createSurface(platform::Window window);
void removeSurface(surface_id id);
void resizeSurface(surface_id id, uint32, uint32);
uint32 surfaceWidth(surface_id id);
uint32 surfaceHeight(surface_id id);
void renderSurface(surface_id id);


} // namespace zone::graphics::d3d12::core