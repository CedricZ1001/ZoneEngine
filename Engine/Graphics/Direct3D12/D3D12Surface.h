// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "D3D12CommonHeaders.h"
#include "D3D12Resources.h"

namespace zone::graphics::d3d12 {

class D3D12Surface
{
public:
	explicit D3D12Surface(platform::Window window) : _window{ window } 
	{
		assert(_window.handle());
	}

#if USE_STL_VECTOR
	DISABLE_COPY(D3D12Surface);
	constexpr D3D12Surface(D3D12Surface&& other) : _swapChain{ other._swapChain }, _window{ other._window }, _currentBackBufferIndex{ other._currentBackBufferIndex },
		_viewport{ other._viewport }, _scissorRect{ other._scissorRect }, _allowTearing{ other._allowTearing }, _presentFlags{ other._presentFlags }
	{
		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			_renderTargetData[i].resource = other._renderTargetData[i].resource;
			_renderTargetData[i].rtv = other._renderTargetData[i].rtv;
		}

		other.reset();
	}

	constexpr D3D12Surface& operator= (D3D12Surface&& other)
	{
		assert(this != &other);
		if (this != &other)
		{
			release();
			move(other);
		}
		return *this;
	}

#endif

	~D3D12Surface() 
	{ 
		release(); 
	}

	void createSwapChain(IDXGIFactory7* factory, ID3D12CommandQueue* cmdQueue, DXGI_FORMAT format);
	void present() const;
	void resize();

	constexpr uint32 width() const { return static_cast<uint32>(_viewport.Width); }
	constexpr uint32 height() const { return static_cast<uint32>(_viewport.Height); }
	constexpr ID3D12Resource *const backBuffer() const { return _renderTargetData[_currentBackBufferIndex].resource; }
	constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv() const { return _renderTargetData[_currentBackBufferIndex].rtv.cpu; }
	constexpr const D3D12_VIEWPORT& viewport() const { return _viewport; }
	constexpr const D3D12_RECT& scissorRect() const { return _scissorRect; }

private:
	void release();
	void finalize();

#if USE_STL_VECTOR
	constexpr void move(D3D12Surface& other)
	{
		_swapChain = other._swapChain;
		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			_renderTargetData[i] = other._renderTargetData[i];
		}
		_window = other._window;
		_currentBackBufferIndex = other._currentBackBufferIndex;
		_allowTearing = other._allowTearing;
		_presentFlags = other._presentFlags;
		_viewport = other._viewport;
		_scissorRect = other._scissorRect;

		other.reset();
	}

	constexpr void reset()
	{
		_swapChain = nullptr;
		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			_renderTargetData[i] = {};
		}
		_window = {};
		_currentBackBufferIndex = 0;
		_allowTearing = 0;
		_presentFlags = 0;
		_viewport = {};
		_scissorRect = {};

	}
#endif

	struct RenderTargetData
	{
		ID3D12Resource* resource{ nullptr };
		DescriptorHandle rtv{};
	};

	IDXGISwapChain4*		_swapChain{ nullptr };
	RenderTargetData		_renderTargetData[FRAME_BUFFER_COUNT]{};
	platform::Window		_window{};
	mutable uint32			_currentBackBufferIndex{ 0 };
	uint32					_allowTearing{ 0 };
	uint32					_presentFlags{ 0 };
	D3D12_VIEWPORT			_viewport{};
	D3D12_RECT				_scissorRect{};
};

} // namespace zone::graphics::d3d12