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
	DescriptorHeap*		container{ nullptr };
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
	void processDeferredFree(uint32 frameIdx);

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
	utl::vector<uint32>					_deferredFreeIndices[FRAME_BUFFER_COUNT]{};
	std::mutex							_mutex{};
	uint32								_capacity{ 0 };
	uint32								_size{ 0 };
	uint32								_descriptorSize{ };
	const D3D12_DESCRIPTOR_HEAP_TYPE	_type{};
};

struct D3D12TextureInitInfo
{
	ID3D12Heap1*						heap{ nullptr };
	ID3D12Resource*						resource{ nullptr };
	D3D12_SHADER_RESOURCE_VIEW_DESC*	srvDesc{ nullptr };
	D3D12_RESOURCE_DESC*				desc{ nullptr };
	D3D12_RESOURCE_ALLOCATION_INFO1		allocationInfo{};
	D3D12_RESOURCE_STATES				initialState{ };
	D3D12_CLEAR_VALUE					clearValue{};
};

class D3D12Texture
{
public:
	constexpr static uint32 maxMips{ 14 }; // support up to 16k resolutions.
	D3D12Texture() = default;
	explicit D3D12Texture(D3D12TextureInitInfo info);
	DISABLE_COPY(D3D12Texture);
	constexpr D3D12Texture(D3D12Texture&& other)
		: _resource{ other._resource }, _srv{ other._srv }
	{
		other.reset();
	}

	constexpr D3D12Texture& operator = (D3D12Texture&& other)
	{
		assert(this != &other);
		if (this != &other)
		{
			release();
			move(other);
		}
		return *this;
	}

	~D3D12Texture() { release(); }

	void release();
	constexpr ID3D12Resource *const resource() const { return _resource; }
	constexpr DescriptorHandle srv() const { return _srv; }
private:

	constexpr void move(D3D12Texture& other)
	{ 
		_resource = other._resource;
		_srv = other._srv;
		other.reset();
	}

	constexpr void reset()
	{
		_resource = nullptr;
		_srv = {};
	}

	ID3D12Resource*		_resource{ nullptr };
	DescriptorHandle	_srv;
};

class D3D12RenderTexture
{
public:
	D3D12RenderTexture() = default;
	explicit D3D12RenderTexture(D3D12TextureInitInfo info);
	DISABLE_COPY(D3D12RenderTexture);
	constexpr D3D12RenderTexture(D3D12RenderTexture&& other)
		:_texture{ std::move(other._texture) }, _mipCount{ other._mipCount }
	{
		for (uint32 i{ 0 }; i < _mipCount; ++i)
		{
			_rtv[i] = other._rtv[i];
		}
		other.reset();
	}
	constexpr D3D12RenderTexture& operator=(D3D12RenderTexture&& other)
	{
		assert(this != &other);
		if (this != &other)
		{
			release();
			move(other);
		}
		return *this; 
	}

	~D3D12RenderTexture() { release(); }

	void release();
	constexpr uint32 mipCount() const { return _mipCount; }
	constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(uint32 mipIndex) const { assert(mipIndex < _mipCount); return _rtv[mipIndex].cpu; }
	constexpr DescriptorHandle srv() const { return _texture.srv(); }
	constexpr ID3D12Resource *const resource()	const { return _texture.resource(); }

private:
	constexpr void move(D3D12RenderTexture& other)
	{
		_texture = std::move(other._texture);
		_mipCount = other._mipCount;
		for (uint32 i{ 0 }; i < _mipCount; ++i)
		{
			_rtv[i] = other._rtv[i];
		}
		other.reset();
	}

	constexpr void reset()
	{
		for (uint32 i{0}; i<_mipCount;++i)
		{
			_rtv[i] = {};
		}
		_mipCount = 0;
	}

	D3D12Texture			_texture{};
	DescriptorHandle		_rtv[D3D12Texture::maxMips]{};
	uint32					_mipCount{ 0 };
};

class D3D12DepthBuffer
{
public:
	D3D12DepthBuffer() = default;
	explicit D3D12DepthBuffer(D3D12TextureInitInfo info);
	DISABLE_COPY(D3D12DepthBuffer);
	constexpr D3D12DepthBuffer(D3D12DepthBuffer&& other)
		:_texture{ std::move(other._texture) }, _dsv{ other._dsv }
	{
		other._dsv = {};
	}

	constexpr D3D12DepthBuffer& operator=(D3D12DepthBuffer&& other)
	{
		assert(this != &other);
		if (this != &other)
		{
			_texture = std::move(other._texture);
			_dsv = other._dsv;
			other._dsv = {};
		}
		return *this;
	}

	~D3D12DepthBuffer() { release(); }

	void release();
	constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const {  return _dsv.cpu; }
	constexpr DescriptorHandle srv() const { return _texture.srv(); }
	constexpr ID3D12Resource *const resource()	const { return _texture.resource(); }
private:
	D3D12Texture			_texture{};
	DescriptorHandle		_dsv{};
};

}