// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Surface.h"
#include "D3D12Core.h"

namespace zone::graphics::d3d12 {

namespace 
{

constexpr DXGI_FORMAT toNonSRGB(DXGI_FORMAT format)
{
	if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
	{
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	}

	return format;
}

} // anonymous namespace

void D3D12Surface::createSwapChain(IDXGIFactory7 * factory, ID3D12CommandQueue * cmdQueue, DXGI_FORMAT format)
{
	assert(factory && cmdQueue);
	release();

	// NOTE: allow the tearing
	//if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &_allowTearing, sizeof(uint32))) && _allowTearing)
	//{
	//	_presentFlags = DXGI_PRESENT_ALLOW_TEARING;
	//}

	DXGI_SWAP_CHAIN_DESC1 desc{};
	desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	desc.BufferCount = BUFFER_COUNT;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = _allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	desc.Format = toNonSRGB(format);
	desc.Width = _window.width();
	desc.Height = _window.height();
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.Stereo = false;

	IDXGISwapChain1* swapChain;
	HWND hwnd{ static_cast<HWND>(_window.handle()) };
	DXCall(factory->CreateSwapChainForHwnd(cmdQueue, hwnd, &desc, nullptr, nullptr, &swapChain));
	DXCall(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
	DXCall(swapChain->QueryInterface(IID_PPV_ARGS(&_swapChain)));
	core::release(swapChain);

	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();

	for (uint32 i{ 0 }; i < BUFFER_COUNT; ++i)
	{
		_renderTargetData[i].rtv = core::rtvHeap().allocate();
	}

	finalize();
}

void D3D12Surface::present() const
{
	assert(_swapChain);
	DXCall(_swapChain->Present(0, 0));
	_currentBackBufferIndex = _swapChain->GetCurrentBackBufferIndex();
}

void D3D12Surface::resize()
{

}

void D3D12Surface::finalize()
{
	// create RTVs for back-buffers
	for (uint32 i{ 0 }; i < BUFFER_COUNT; ++i)
	{
		RenderTargetData& data{ _renderTargetData[i] };
		assert(!data.resource);
		DXCall(_swapChain->GetBuffer(i, IID_PPV_ARGS(&data.resource)));
		D3D12_RENDER_TARGET_VIEW_DESC desc{};
		desc.Format = core::defaultRenderTargetFormat();
		desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		core::getDevice()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
	}

	DXGI_SWAP_CHAIN_DESC desc{};
	DXCall(_swapChain->GetDesc(&desc));
	const uint32 width{ desc.BufferDesc.Width };
	const uint32 height{ desc.BufferDesc.Height };
	assert(_window.width() == width && _window.height() == height);

	// set viewport and scissor rect
	_viewport.TopLeftX = 0.0f;
	_viewport.TopLeftY = 0.0f;
	_viewport.Width = static_cast<float>(width);
	_viewport.Height = static_cast<float>(height);
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;

	_scissorRect = { 0,0,static_cast<int32>(width), static_cast<int32>(height) };

}

void D3D12Surface::release()
{
	for (uint32 i{ 0 }; i < BUFFER_COUNT; ++i)
	{
		RenderTargetData& data{ _renderTargetData[i] };
		core::release(data.resource);
		core::rtvHeap().free(data.rtv);
	}
	core::release(_swapChain);

}

} // namespace zone::graphics::d3d12
