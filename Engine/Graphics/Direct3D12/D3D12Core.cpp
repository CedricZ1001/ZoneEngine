// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Core.h"
#include "D3D12Resources.h"
#include "D3D12Surface.h"

using namespace Microsoft::WRL;

namespace zone::graphics::d3d12::core {
namespace {

class D3D12Command
{
public:

	D3D12Command() = default;
	DISABLE_COPY_AND_MOVE(D3D12Command);

	explicit D3D12Command(ID3D12Device8 *const device, D3D12_COMMAND_LIST_TYPE type)
	{
		HRESULT hr{ S_OK };
		D3D12_COMMAND_QUEUE_DESC queueDesc{};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.NodeMask = 0;
		queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		queueDesc.Type = type;

		DXCall(hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue)));
		if (FAILED(hr))
		{
			goto _error;
		}
		NAME_D3D12_OBJECT(_cmdQueue, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Queue" : type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"Compute Command Queue" : L"Command Queue");

		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			CommandFrame& frame{ _cmdFrames[i] };
			DXCall(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmdAllocator)));
			if (FAILED(hr))
			{
				goto _error;
			}
			NAME_D3D12_OBJECT_INDEXED(frame.cmdAllocator, i, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Allocator" : type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"Compute Command Allocator" : L"Command Allocator");
		}

		DXCall(hr = device->CreateCommandList(0, type, _cmdFrames[0].cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList)));
		if (FAILED(hr))
		{
			goto _error;
		}
		DXCall(_cmdList->Close());
		NAME_D3D12_OBJECT(_cmdList, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command List" : type == D3D12_COMMAND_LIST_TYPE_COMPUTE ? L"Compute Command List" : L"Command List");

		DXCall(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
		if (FAILED(hr))
		{
			goto _error;
		}
		NAME_D3D12_OBJECT(_fence, L"D3D12 Fence");

		_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
		assert(_fenceEvent);

		return;

	_error:
		release();
	}

	~D3D12Command()
	{
		assert(!_cmdQueue && !_cmdList && !_fence);
	}

	void beginFrame()
	{
		CommandFrame& frame{ _cmdFrames[_frameIndex] };
		frame.wait(_fenceEvent, _fence);
		DXCall(frame.cmdAllocator->Reset());
		DXCall(_cmdList->Reset(frame.cmdAllocator, nullptr));
	}

	void endFrame()
	{
		DXCall(_cmdList->Close());
		ID3D12CommandList *const cmdLists[]{ _cmdList };
		_cmdQueue->ExecuteCommandLists(_countof(cmdLists), &cmdLists[0]);

		uint64& fenceValue{ _fenceValue };
		++fenceValue;
		CommandFrame& frame{ _cmdFrames[_frameIndex] };
		frame.fenceValue = fenceValue;
		_cmdQueue->Signal(_fence, fenceValue);

		_frameIndex = (_frameIndex + 1) % FRAME_BUFFER_COUNT;
	}

	void flush()
	{
		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			_cmdFrames[i].wait(_fenceEvent, _fence);
		}
		_frameIndex = 0;
	}

	void release()
	{
		flush();
		core::release(_fence);
		_fenceValue = 0;

		CloseHandle(_fenceEvent);
		_fenceEvent = nullptr;

		core::release(_cmdQueue);
		core::release(_cmdList);

		for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
		{
			_cmdFrames[i].release();
		}
	}

	constexpr ID3D12CommandQueue* const commandQueue() const { return _cmdQueue; }
	constexpr ID3D12GraphicsCommandList6* const commandList() const { return _cmdList; }
	constexpr uint32 frameIndex() const { return _frameIndex; }

private:
	struct CommandFrame 
	{
		ID3D12CommandAllocator* cmdAllocator{ nullptr };
		uint64					fenceValue{ 0 };
		void wait(HANDLE fenceEvent, ID3D12Fence1* fence)
		{
			assert(fence && fenceEvent);
			// If the current fence value is still less than "fenceValue"
			// then we know the GPU has not finished executing the command lists
			//since it has not reached the "_cmd_queue->Signal()" command.
			if (fence->GetCompletedValue() < fenceValue)
			{
				// We have the fence create an event witch is signaled one the fence's current value equals "fenceValue"
				DXCall(fence->SetEventOnCompletion(fenceValue, fenceEvent));
				// Wait until the fence has triggered the event that its current value has reached "fenceValue"
				// indicating that command queue has finished executing.
				WaitForSingleObject(fenceEvent, INFINITE);
			}
		}

		void release()
		{
			core::release(cmdAllocator);
			fenceValue = 0;
		}
	};

	ID3D12CommandQueue*				_cmdQueue{ nullptr };
	ID3D12GraphicsCommandList6*		_cmdList{ nullptr };
	ID3D12Fence1*					_fence{ nullptr };
	uint64							_fenceValue{0};
	HANDLE							_fenceEvent{ nullptr };
	CommandFrame					_cmdFrames[FRAME_BUFFER_COUNT];
	uint32							_frameIndex{ 0 };
};

ID3D12Device8*				mainDevice{ nullptr };
IDXGIFactory7*				dxgiFactory{ nullptr };
D3D12Command				gfxCommand;
utl::vector<D3D12Surface>	surfaces{};

DescriptorHeap			rtvDescHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV };
DescriptorHeap			dsvDescHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV };
DescriptorHeap			srvDescHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
DescriptorHeap			uavDescHeap{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

utl::vector<IUnknown*>	deferredReleases[FRAME_BUFFER_COUNT]{};
uint32					deferredReleasesFlag[FRAME_BUFFER_COUNT]{};
std::mutex				deferredReleasesMutex{ };

constexpr DXGI_FORMAT renderTargetFormat{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB };
constexpr D3D_FEATURE_LEVEL minimumFeatureLevel{ D3D_FEATURE_LEVEL_11_0 };

bool failedInit()
{
	shutdown();
	return false;
}

//void LogCurrentDisplaySettings(const wchar_t* deviceName)
//{
//	DEVMODE dm;
//	ZeroMemory(&dm, sizeof(dm));
//	dm.dmSize = sizeof(dm);
//	if (EnumDisplaySettings(deviceName, ENUM_CURRENT_SETTINGS, &dm))
//	{
//		wchar_t buffer[256];
//		swprintf_s(buffer, 256, L"The current display mode:\n\tResolution: %d x %d\n\tRefresh-rate: %d Hz\n",
//			dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
//		OutputDebugString(buffer);
//	}
//	else
//	{
//		OutputDebugString(L"Failed to get the current display settings\n");
//	}
//}
//
//void LogDisplayOutputs(IDXGIAdapter4* adapter)
//{
//	IDXGIOutput* output = nullptr;
//	for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
//	{
//		DXGI_OUTPUT_DESC desc;
//		if (SUCCEEDED(output->GetDesc(&desc)))
//		{
//			wchar_t outputInfo[256];
//			swprintf_s(outputInfo, 256, L"DISPLAY OUTPUT %u:\n\tDevice Name: %s\n", i, desc.DeviceName);
//			OutputDebugString(outputInfo);
//
//			LogCurrentDisplaySettings(desc.DeviceName);
//		}
//		output->Release();
//	}
//}

IDXGIAdapter4* determineMainAdapter()
{
	IDXGIAdapter4* adapter{ nullptr };

	//get adapters in descending order of performance
	for (uint32 i{ 0 }; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		// pick the first adapter that supports the minimum feature level.
		if (SUCCEEDED(D3D12CreateDevice(adapter, minimumFeatureLevel, __uuidof(ID3D12Device), nullptr)))
		{
			LOG_DXGI_ADAPTER(adapter);
			//LogDisplayOutputs(adapter);
			return adapter;
		}
		release(adapter);
	}
	return nullptr;
}

D3D_FEATURE_LEVEL getMaxFeatureLevel(IDXGIAdapter4* adapter)
{
	constexpr D3D_FEATURE_LEVEL featureLevel[5]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelInfo{};
	featureLevelInfo.NumFeatureLevels = _countof(featureLevel);
	featureLevelInfo.pFeatureLevelsRequested = featureLevel;

	ComPtr<ID3D12Device> device;
	DXCall(D3D12CreateDevice(adapter, minimumFeatureLevel, IID_PPV_ARGS(&device)));
	DXCall(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelInfo, sizeof(featureLevelInfo)));
	return featureLevelInfo.MaxSupportedFeatureLevel;
}

void __declspec(noinline)
processDeferredReleases(uint32 frameIdx)
{
	std::lock_guard lock{ deferredReleasesMutex };

	deferredReleasesFlag[frameIdx] = 0;

	rtvDescHeap.processDeferredFree(frameIdx);
	dsvDescHeap.processDeferredFree(frameIdx);
	srvDescHeap.processDeferredFree(frameIdx);
	uavDescHeap.processDeferredFree(frameIdx);
	
	utl::vector<IUnknown*>& resources{ deferredReleases[frameIdx] };
	if (!resources.empty())
	{
		for (auto& resource : resources)
		{
			release(resource);
			resources.clear();
		}
	}
}

} // anonymous namespace

namespace detail {

void deferredRelease(IUnknown* resource)
{
	const uint32 frameIdx{ currentFrameIndex() };
	std::lock_guard lock{ deferredReleasesMutex };
	deferredReleases[frameIdx].push_back(resource);
	setDeferredReleasesFlag();
}

}// detail namespace

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
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
		{
			debugInterface->EnableDebugLayer();
		}
		else {
			OutputDebugStringA("Warning: D3D12 Debug interface is not available. Verify that Graphics Tools optional feature is installed in this device.\n");
		}

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

#ifdef _DEBUG
	{
		ComPtr<ID3D12InfoQueue> infoQueue;
		DXCall(mainDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)));
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
	}
#endif // _DEBUG


	bool result{ true };
	result &= rtvDescHeap.initialize(512, false);
	result &= dsvDescHeap.initialize(512, false);
	result &= srvDescHeap.initialize(4096, true);
	result &= uavDescHeap.initialize(512, false);
	if (!result)
	{
		return failedInit();
	}

	new (&gfxCommand) D3D12Command(mainDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
	if (!gfxCommand.commandQueue())
	{
		return failedInit();
	}

	NAME_D3D12_OBJECT(mainDevice, L"Main D3D12 Device");
	NAME_D3D12_OBJECT(rtvDescHeap.heap(), L"RTV Descriptor Heap");
	NAME_D3D12_OBJECT(dsvDescHeap.heap(), L"DSV Descriptor Heap");
	NAME_D3D12_OBJECT(srvDescHeap.heap(), L"SRV Descriptor Heap");
	NAME_D3D12_OBJECT(uavDescHeap.heap(), L"UAV Descriptor Heap");

	return true;
}

void shutdown()
{
	gfxCommand.release();

	for (uint32 i{ 0 }; i < FRAME_BUFFER_COUNT; ++i)
	{
		processDeferredReleases(i);
	}

	release(dxgiFactory);

	rtvDescHeap.release();
	dsvDescHeap.release();
	srvDescHeap.release();
	uavDescHeap.release();

	processDeferredReleases(0);

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

ID3D12Device *const getDevice()
{
	return mainDevice;
}

DescriptorHeap& rtvHeap()
{
	return rtvDescHeap;
}

DescriptorHeap& dsvHeap()
{
	return dsvDescHeap;
}

DescriptorHeap& srvHeap()
{
	return srvDescHeap;
}

DescriptorHeap& uavHeap()
{
	return uavDescHeap;
}

DXGI_FORMAT defaultRenderTargetFormat()
{
	return renderTargetFormat;
}

uint32 currentFrameIndex()
{
	return gfxCommand.frameIndex();
}

void setDeferredReleasesFlag() 
{
	deferredReleasesFlag[currentFrameIndex()] = 1;
}

Surface createSurface(platform::Window window)
{
	surfaces.emplace_back(window);
	surface_id id{ static_cast<uint32>(surfaces.size() - 1) };
	surfaces[id].createSwapChain(dxgiFactory, gfxCommand.commandQueue(), renderTargetFormat);
	return Surface{ id };
}

void removeSurface(surface_id id)
{
	gfxCommand.flush();
	surfaces[id].~D3D12Surface();
}

void resizeSurface(surface_id id, uint32, uint32)
{
	gfxCommand.flush();
	surfaces[id].resize();
}

uint32 surfaceWidth(surface_id id)
{
	return surfaces[id].width();
}

uint32 surfaceHeight(surface_id id)
{
	return surfaces[id].height();
}

void renderSurface(surface_id id)
{
	gfxCommand.beginFrame();
	ID3D12GraphicsCommandList6* cmdList{ gfxCommand.commandList() };

	const uint32 frameIdx{ currentFrameIndex() };
	if (deferredReleasesFlag[frameIdx])
	{
		processDeferredReleases(frameIdx);
	}
	const D3D12Surface& surface{ surfaces[id] };

	// Presenting swap chain buffers happens in lockstep with frame buffers.
	surface.present();

	gfxCommand.endFrame();
}


}