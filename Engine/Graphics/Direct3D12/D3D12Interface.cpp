// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#include "CommonHeaders.h"
#include "D3D12Interface.h"
#include "D3D12Core.h"
#include "Graphics\GraphicsPlatformInterface.h"

namespace zone::graphics::d3d12 {

void getPlatformInterface(PlatformInterface& pi)
{
	pi.initialize = core::initialize;
	pi.shutdown = core::shutdown;
}

}
