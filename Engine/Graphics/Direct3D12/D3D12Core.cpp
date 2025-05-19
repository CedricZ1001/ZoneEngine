// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Core.h"
#include "D3D12Resources.h"
#include "D3D12Surface.h"
#include "D3D12Helpers.h"

using namespace Microsoft::WRL;

namespace zone::graphics::d3d12::core {
// TODO: remove when you're done showing how to create a root signature the tedious way
void createARootSignatue();
void createARootSignature2();
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

using surfaceCollection = utl::FreeList<D3D12Surface>;

ID3D12Device8*				mainDevice{ nullptr };
IDXGIFactory7*				dxgiFactory{ nullptr };
D3D12Command				gfxCommand;
surfaceCollection			surfaces{};

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


	// TODO: remove.
	createARootSignatue();
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
	surface_id id{ surfaces.add(window) };
	surfaces[id].createSwapChain(dxgiFactory, gfxCommand.commandQueue(), renderTargetFormat);
	return Surface{ id };
}

void removeSurface(surface_id id)
{
	gfxCommand.flush();
	surfaces.remove(id);
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

void createARootSignatue()
{
	D3D12_ROOT_PARAMETER1 params[3];
	{// param 0: 2 constants
		auto& param{ params[0] };
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		D3D12_ROOT_CONSTANTS rootConstants{};
		rootConstants.Num32BitValues = 2;
		rootConstants.ShaderRegister = 0; // b0
		rootConstants.RegisterSpace = 0;
		param.Constants = rootConstants;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	{// param 1: 1 Constant Buffer View (Descriptor)
		auto& param = params[1];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		D3D12_ROOT_DESCRIPTOR1 rootDescriptor{};
		rootDescriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
		rootDescriptor.ShaderRegister = 1;
		rootDescriptor.RegisterSpace = 0;
		param.Descriptor = rootDescriptor;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
	{// param 2: descriptor table (unbounded/bindless)
		auto& param = params[2];
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		D3D12_ROOT_DESCRIPTOR_TABLE1 rootDescriptorTable{};
		rootDescriptorTable.NumDescriptorRanges = 1;
		D3D12_DESCRIPTOR_RANGE1 descriptorRange{};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.NumDescriptors = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		descriptorRange.BaseShaderRegister = 0;
		descriptorRange.RegisterSpace = 0;
		descriptorRange.RegisterSpace = 0;
		rootDescriptorTable.pDescriptorRanges = &descriptorRange;
		param.DescriptorTable = rootDescriptorTable;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC1 desc{};
	desc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	desc.NumParameters = _countof(params);
	desc.pParameters = &params[0];
	desc.NumStaticSamplers = 1;
	desc.pStaticSamplers = &samplerDesc;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
	rootSignatureDesc.Desc_1_1 = desc;

	HRESULT hr{ S_OK };
	ID3D10Blob* rootSignatureBlob{ nullptr };
	ID3D10Blob* errorBlob{ nullptr };
	if (FAILED(hr = D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &rootSignatureBlob, &errorBlob)))
	{
		DEBUG_OP(const char* errorMessage{ errorBlob ? (const char*)errorBlob->GetBufferPointer() : "" });
		DEBUG_OP(OutputDebugStringA(errorMessage););
		return;
	}

	assert(rootSignatureBlob);
	ID3D12RootSignature* rootSignature{ nullptr };
	DXCall(hr = getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	release(rootSignatureBlob);
	release(errorBlob);

	// use rootSignature
	// when renderer shuts down
	release(rootSignature);
}

void createARootSignature2()
{
	d3dx::D3D12DescriptorRange range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0 };
	d3dx::D3D12RootParameter params[3];
	params[0].asConstants(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);
	params[1].asCBV(D3D12_SHADER_VISIBILITY_PIXEL, 1);
	params[2].asDescriptorTable(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);

	d3dx::D3D12RootSignatureDesc rootSignatureDesc{ &params[0], _countof(params) };
	ID3D12RootSignature* rootSignature{ rootSignatureDesc.create() };

	// use rootSignature
	// when renderer shuts down
	release(rootSignature);
}

}