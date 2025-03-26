// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"

namespace zone::utl {

// A vector class similar to std::vector with basic functionality
// The user can specify in the template argument whether they want
// elements' destructor to be called when being removed or while
// clearing/destructing the vector
template<typename T,bool destruct = true>
class Vector
{
public:
	// Default constructor. Doesn't allocate memory
	Vector() = default;

	// Constructor that allocates memory for count elements
	constexpr Vector(uint64 count)
	{
		resize(count);
	}

	~Vector();

private:

};
}