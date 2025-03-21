// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Resources.h"
#include "D3D12Core.h"

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

	ID3D12Device *const device{ core::getDevice() };
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

}