// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "D3D12CommonHeaders.h"

namespace zone::graphics::d3d12 {

struct DescriptorHandle {

	D3D12_CPU_DESCRIPTOR_HANDLE		cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE		gpu{};

	constexpr bool isValid() const { return cpu.ptr != 0; }
	constexpr bool isShaderVisible() const { return gpu.ptr != 0; }

#ifdef _DEBUG
private:
	friend class DescriptorHeap;
	DescriptorHandle*	container{ nullptr };
	uint32				index{ uint32_invalid_id };
#endif // _DEBUG

};


class DescriptorHeap
{
public:
	explicit DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) :_type{ type } {}
	DISABLE_COPY_AND_MOVE(DescriptorHeap);
	~DescriptorHeap() { assert(!_heap); }
	bool initialize(uint32 capacity, bool isShaderVisible);
	void release();

	[[nodiscard]] DescriptorHandle allocate();
	void free(DescriptorHandle& handle);

	constexpr D3D12_DESCRIPTOR_HEAP_TYPE type() const { return _type; }
	constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpuStart() const { return _cpuStart; }
	constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpuStart() const { return _gpuStart; }
	constexpr ID3D12DescriptorHeap *const heap() const { return _heap; }
	constexpr uint32 capacity() const { return _capacity; }
	constexpr uint32 size() const { return _size; }
	constexpr uint32 descriptorSize() const { return _descriptorSize; }
	constexpr bool isShaderVisible() const { return _gpuStart.ptr != 0; }

private:
	ID3D12DescriptorHeap*				_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE			_cpuStart{};
	D3D12_GPU_DESCRIPTOR_HANDLE			_gpuStart{};
	std::unique_ptr<uint32[]>			_freeHandles{};
	uint32								_capacity{ 0 };
	uint32								_size{ 0 };
	uint32								_descriptorSize{ };
	const D3D12_DESCRIPTOR_HEAP_TYPE	_type{};
};

}