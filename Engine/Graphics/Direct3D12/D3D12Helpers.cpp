#include "D3D12Helpers.h"
#include "D3D12Core.h"

namespace zone::graphics::d3d12::d3dx {
	namespace {
		
	} // anonymous namespace

	ID3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& desc)
	{
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC versionedDesc{};
		versionedDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		versionedDesc.Desc_1_1 = desc;

		using namespace Microsoft::WRL;
		ComPtr<ID3D10Blob> SignatureBlob{ nullptr };
		ComPtr<ID3D10Blob> errorBlob{ nullptr };
		HRESULT hr{ S_OK };
		if (FAILED(hr = D3D12SerializeVersionedRootSignature(&versionedDesc, &SignatureBlob, &errorBlob)))
		{
			DEBUG_OP(const char* errorMessage{ errorBlob ? (const char*)errorBlob->GetBufferPointer() : "" });
			DEBUG_OP(OutputDebugStringA(errorMessage););
			return nullptr;
		}

		ID3D12RootSignature* signature{ nullptr };
		DXCall(hr = core::getDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(),
														  SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&signature)));

		if (FAILED(hr)) {
			core::release(signature);
		}

		return signature;
	}
}