// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "D3D12Core.h"

namespace zone::graphics::d3d12::core {

namespace {

ID3D12Device8* mainDevice;

} // anonymous namespace

bool initialize() 
{
	// determine which adapter (i.e. graphics card) to use
	// determine what is the maximum feature level that is supporter
	// create a ID3D12Device (this a virtual adapter)
}

void shutdow()
{

}

}