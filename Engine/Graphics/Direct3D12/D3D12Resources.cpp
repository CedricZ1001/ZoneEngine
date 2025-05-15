// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Resources.h"
#include "D3D12Core.h"
#include "D3D12Helpers.h"

namespace zone::graphics::d3d12 {

bool DescriptorHeap::initialize(uint32 capacity, bool isShaderVisible)
{
	std::lock_guard lock{ _mutex };
	assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
	assert(!(_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
	if (_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || _type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
	{
		isShaderVisible = false;
	}

	release();

	auto *const device{ core::getDevice() };
	assert(device);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NumDescriptors = capacity;
	desc.Type = _type;
	desc.NodeMask = 0;

	HRESULT hr{ S_OK };
	DXCall(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap)));
	if (FAILED(hr))
	{
		return false;
	}

	_freeHandles = std::move(std::make_unique<uint32[]>(capacity));
	_capacity = capacity;
	_size = 0;

	for (uint32 i{ 0 }; i < capacity; ++i)
	{
		_freeHandles[i] = i;
	}

	DEBUG_OP(for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i) assert(_deferredFreeIndices[i].empty()));

	_descriptorSize = device->GetDescriptorHandleIncrementSize(_type);
	_cpuStart = _heap->GetCPUDescriptorHandleForHeapStart();
	_gpuStart = isShaderVisible ? _heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

	return true;

}

void DescriptorHeap::release()
{
	assert(!_size);
	core::deferredRelease(_heap);
}

void DescriptorHeap::processDeferredFree(uint32 frameIdx)
{
	std::lock_guard lock{ _mutex };
	assert(frameIdx < FRAME_BUFFER_COUNT);

	utl::vector<uint32>& indices{ _deferredFreeIndices[frameIdx] };
	if (!indices.empty())
	{
		for (auto index : indices)
		{
			--_size;
			_freeHandles[_size] = index;
		}
		indices.clear();
	}
}

DescriptorHandle DescriptorHeap::allocate()
{
	std::lock_guard lock{ _mutex };
	assert(_heap);
	assert(_size < _capacity);

	const uint32 index{ _freeHandles[_size] };
	const uint32 offset{ index * _descriptorSize };
	++_size;

	DescriptorHandle handle;
	handle.cpu.ptr = _cpuStart.ptr + offset;
	if (isShaderVisible())
	{
		handle.gpu.ptr = _gpuStart.ptr + offset;
	}

	DEBUG_OP(handle.container = this);
	DEBUG_OP(handle.index = index);
	return handle;
}

void DescriptorHeap::free(DescriptorHandle& handle)
{
	if (!handle.isValid())
	{
		return;
	}
	std::lock_guard lock{ _mutex };
	assert(_heap && _size);
	assert(handle.container == this);
	assert(handle.cpu.ptr >= _cpuStart.ptr);
	assert((handle.cpu.ptr - _cpuStart.ptr) % _descriptorSize == 0);
	assert(handle.index < _capacity);
	const uint32 index{ static_cast<uint32>(handle.cpu.ptr - _cpuStart.ptr) / _descriptorSize };
	assert(handle.index == index);

	const uint32 frameIndex{ core::currentFrameIndex() };
	_deferredFreeIndices[frameIndex].push_back(index);
	core::setDeferredReleasesFlag();
	handle = {};
}

D3D12Texture::D3D12Texture(D3D12TextureInitInfo info)
{
	auto *const device{ core::getDevice() };
	assert(device);

	D3D12_CLEAR_VALUE *const clearValue
	{
		(info.desc && (info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET || info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) ? &info.clearValue : nullptr
	};

	if (info.resource)
	{
		assert(!info.heap);
		_resource = info.resource;
	}
	else if (info.heap)
	{
		assert(!info.resource);
		DXCall(device->CreatePlacedResource(info.heap, info.allocationInfo.Offset, info.desc, info.initialState, clearValue, IID_PPV_ARGS(&_resource)));
	}
	else
	{
		assert(!info.heap && !info.resource);
		DXCall(device->CreateCommittedResource(&d3dx::HeapProperties.defaultHeap, D3D12_HEAP_FLAG_NONE, info.desc, info.initialState, clearValue, IID_PPV_ARGS(&_resource)));
	}

	assert(_resource);
	_srv = core::srvHeap().allocate();
	device->CreateShaderResourceView(_resource, info.srvDesc, _srv.cpu);
}

void D3D12Texture::release()
{
	core::srvHeap().free(_srv);
	core::deferredRelease(_resource);
}

D3D12RenderTexture::D3D12RenderTexture(D3D12TextureInitInfo info)
	:_texture{ info }
{
	assert(info.desc);
	_mipCount = resource()->GetDesc().MipLevels;
	assert(_mipCount && _mipCount <= D3D12Texture::maxMips);

	DescriptorHeap& rtvHeap{ core::rtvHeap() };
	D3D12_RENDER_TARGET_VIEW_DESC desc{};
	desc.Format = info.desc->Format;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	auto *const device{ core::getDevice() };
	assert(device);

	for (uint32 i{ 0 }; i < _mipCount; ++i)
	{
		_rtv[i] = rtvHeap.allocate();
		device->CreateRenderTargetView(resource(), &desc, _rtv[i].cpu);
		++desc.Texture2D.MipSlice;
	}
}

void D3D12RenderTexture::release()
{
	for (uint32 i{ 0 }; i < _mipCount; ++i)
	{
		core::rtvHeap().free(_rtv[i]);
	}
	_texture.release();
	_mipCount = 0;
}

D3D12DepthBuffer::D3D12DepthBuffer(D3D12TextureInitInfo info)
{
	assert(info.desc);
	const DXGI_FORMAT dsvFormat{ info.desc->Format };

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	if (info.desc->Format == DXGI_FORMAT_D32_FLOAT)
	{
		info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	}

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

	assert(!info.srvDesc && !info.resource);
	info.srvDesc = &srvDesc;
	_texture = D3D12Texture(info);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = dsvFormat;
	dsvDesc.Texture2D.MipSlice = 0;

	_dsv = core::dsvHeap().allocate();

	auto *const device{ core::getDevice() };
	assert(device);
	device->CreateDepthStencilView(resource(), &dsvDesc, _dsv.cpu);
}

void D3D12DepthBuffer::release()
{
	core::dsvHeap().free(_dsv);
	_texture.release();
}

}