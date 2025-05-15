// Copyright (c) CedricZ1, 2024
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#define USE_STL_VECTOR 0
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include<vector>
#include <algorithm>
namespace zone::utl {
template<typename T>
using vector = std::vector<T>;

template<typename T>
void erase_unordered(T& vec, size_t index)
{
	if (vec.size() > 1) 
	{
		std::iter_swap(vec.begin() + index, vec.end() - 1);
		vec.pop_back();
	}
	else 
	{
		vec.clear();
	}
}

}
#else
#include "Vector.h"

namespace zone::utl {
template<typename T>
void erase_unordered(T& v, size_t index)
{
	v.erase_unordered(index);
}
}
#endif

#if USE_STL_DEQUE
#include<deque>
namespace zone::utl {
template<typename T>
using deque = std::deque<T>;
}
#endif

namespace zone::utl {

}

#include "FreeList.h"