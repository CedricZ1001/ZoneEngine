// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once

namespace zone::graphics {
struct PlatformInterface;

namespace d3d12 {
void getPlatformInterface(PlatformInterface& pi);
}
}