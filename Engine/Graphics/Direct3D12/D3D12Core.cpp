// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Core.h"

using namespace Microsoft::WRL;

namespace zone::graphics::d3d12::core {

namespace {

ID3D12Device8* mainDevice{ nullptr };
IDXGIFactory7* dxgiFactory{ nullptr };

constexpr D3D_FEATURE_LEVEL minimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

bool failedInit()
{
	shutdown();
	return false;
}

IDXGIAdapter4* determineMainAdapter()
{
	IDXGIAdapter4* adapter{ nullptr };

	//get adapters in descending order of performance
	for (uint32 i{ 0 }; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		// pick the first adapter that supports the minimum feature level.
		if (SUCCEEDED(D3D12CreateDevice(adapter, minimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
		{
			return adapter;
		}
		release(adapter);
	}
	return nullptr;
}

D3D_FEATURE_LEVEL getMaxFeatureLevel(IDXGIAdapter4* adapter)
{
	constexpr D3D_FEATURE_LEVEL featureLevel[4]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo{};
	featureLevelInfo.NumFeatureLevels = _countof(featureLevel);
	featureLevelInfo.pFeatureLevelsRequested = featureLevel;

	ComPtr<ID3D12Device> device;
	DXCall(D3D12CreateDevice(adapter, minimumFeatureLevel, IID_PPV_ARGS(&device)));
	DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));
	return featureLevelInfo.MaxSupportedFeatureLevel;
}

} // anonymous namespace

bool initialize() 
{
	// determine what is the maximum feature level that is supporter
	// create a ID3D12Device (this a virtual adapter)

	if (mainDevice)
	{
		shutdown();
	}

	uint32 dxgiFactoryFlag{ 0 };
#ifdef _DEBUG
	// Enable debugging layer. Requires "Graphics Tools" optional feature
	{
		ComPtr<ID3D12Debug3> debugInterface;
		DXCall(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
		debugInterface->EnableDebugLayer();
		dxgiFactoryFlag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif //_DEBUG

	HRESULT hr{ S_OK };
	DXCall(hr = CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&dxgiFactory)));
	if (FAILED(hr))
	{
		return failedInit();
	}

	// determine which adapter (i.e. graphics card) to use
	ComPtr<IDXGIAdapter4> mainAdapter;
	
	mainAdapter.Attach(determineMainAdapter());
	if (!mainAdapter)
	{
		return failedInit();
	}

	D3D_FEATURE_LEVEL maxFeatureLevel{ getMaxFeatureLevel(mainAdapter.Get()) };
	assert(maxFeatureLevel >= minimumFeatureLevel);
	if (maxFeatureLevel < minimumFeatureLevel)
	{
		return failedInit();
	}

	DXCall(hr = D3D12CreateDevice(mainAdapter.Get(), maxFeatureLevel, IID_PPV_ARGS(&mainDevice)));
	if (FAILED(hr))
	{
		return failedInit();
	}

	NAME_D3D12_OBJECT(mainDevice, L"Main D3D12 Device");

#ifdef _DEBUG
	{
		ComPtr<ID3D12InfoQueue> infoQueue;
		DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif // _DEBUG

	return true;
}

void shutdown()
{
	release(dxgiFactory);

#ifdef _DEBUG
	{
		{
			ComPtr<ID3D12InfoQueue> infoQueue;
			DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
		}

		ComPtr<ID3D12DebugDevice2> debugDevice;
		DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&debugDevice)));
		release(mainDevice);
		DXCall(debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
	}
#endif // _DEBUG

	release(mainDevice);
}

}