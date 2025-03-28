// Copyright (c) CedricZ1, 2024
// Distributed under the MIT license. See the LICENSE file in the project root for more information.

#pragma once
#include<stdint.h>

// unsigned integers
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// signed integers
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

constexpr uint8 uint8_invalid_id{ 0xffui8 };
constexpr uint16 uint16_invalid_id{ 0xffffui16 };
constexpr uint32 uint32_invalid_id{ 0xffff'ffffui32 };
constexpr uint64 uint64_invalid_id{ 0xffff'ffff'ffff'ffffui64 };
