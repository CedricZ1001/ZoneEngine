// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"

namespace zone::utl
{

#if USE_STL_VECTOR
#pragma message("WARNING: using utl::FreeList with std::vector result in duplicate calls to class constructor! .")
#endif

template<typename T>
class FreeList
{
	static_assert(sizeof(T) >= sizeof(uint32), "FreeList requires T to be at least 4 bytes");
public:
	FreeList() = default;
	explicit FreeList(uint32 capacity) 
	{
		_array.reserve(capacity);
	}

	~FreeList()
	{
		assert(!_size);
#if USE_STL_VECTOR
		memset(_array.data(), 0, _array.size() * sizeof(T));
#endif
	}

	template<class... params>
	constexpr uint32 add(params&&... p)
	{
		uint32 id{ uint32_invalid_id };
		if (_nextFreeIndex == uint32_invalid_id)
		{
			id = static_cast<uint32>(_array.size());
			_array.emplace_back(T{ std::forward<params>(p)... });
		}
		else
		{
			id = _nextFreeIndex;
			assert(id < _array.size() && !alreadeyRemoved(id));
			_nextFreeIndex = *reinterpret_cast<const uint32*>(std::addressof(_array[id]));
			new	(std::addressof(_array[id])) T{ std::forward<params>(p)... };
		}
		++_size;
		return id;
	}

	constexpr void remove(uint32 id)
	{
		assert(id < _array.size() && !alreadeyRemoved(id));
		T& item{ _array[id] };
		item.~T();
		DEBUG_OP(memset(std::addressof(_array[id]), 0xcc, sizeof(T)));
		*reinterpret_cast<uint32*>(std::addressof(_array[id])) = _nextFreeIndex;
		_nextFreeIndex = id;
		--_size;
	}

	constexpr uint32 size() const
	{
		return _size;
	}

	constexpr uint32 capacity() const
	{
		return _array.size();
	}

	constexpr bool empty() const
	{
		return _size == 0;
	}

	[[nodiscard]] constexpr T& operator[](uint32 id)
	{
		assert(id < _array.size() && !alreadeyRemoved(id));
		return _array[id];
	}

	[[nodiscard]] constexpr const T& operator[](uint32 id) const
	{
		assert(id < _array.size() && !alreadeyRemoved(id));
		return _array[id];
	}
private:
	constexpr bool alreadeyRemoved(uint32 id)
	{
		if constexpr (sizeof(T) > sizeof(uint32))
		{
			uint32 i{ sizeof(uint32) }; // skip the first 4 bytes.
			const uint8 *const p{ (const uint8 *const)std::addressof(_array[id]) };
			while ((p[i] == 0xcc) && (i < sizeof(T)))
			{
				++i;
			}
			return i == sizeof(T);
		}
		else
		{
			return true;
		}
	}

#if USE_STL_VECTOR
	utl::vector<T>				_array;
#else
	utl::vector<T, false>		_array;
#endif
	uint32						_nextFreeIndex{ uint32_invalid_id };
	uint32						_size{ 0 };
};

} // namespace zone::utl