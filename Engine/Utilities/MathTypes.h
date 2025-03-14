// Copyright (c) CedricZ1, 2024
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"

namespace zone::math {
constexpr float	pi = 3.1415926535897932384626433832795f;
constexpr float	two_pi = 2.0f * pi;
constexpr float epsilon = 1e-5f;

#ifdef _WIN64
using Vec2F = DirectX::XMFLOAT2;
using Vec2FA = DirectX::XMFLOAT2A;

using Vec3F = DirectX::XMFLOAT3;
using Vec3FA = DirectX::XMFLOAT3A;

using Vec4F = DirectX::XMFLOAT4;
using Vec4FA = DirectX::XMVECTOR;

using Vec2U = DirectX::XMUINT2;
using Vec3U = DirectX::XMUINT3;
using Vec4U = DirectX::XMUINT4;

using Vec2I = DirectX::XMINT2;
using Vec3I = DirectX::XMINT3;
using Vec4I = DirectX::XMINT4;

using Mat3x3F = DirectX::XMFLOAT3X3;
using Mat3x4F = DirectX::XMFLOAT3X4;
using Mat4x4F = DirectX::XMFLOAT4X4;
using Mat4x4FA = DirectX::XMMATRIX;
#endif // _Win64


}