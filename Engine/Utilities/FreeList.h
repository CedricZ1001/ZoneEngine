// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "CommonHeaders.h"

namespace zone::utl
{
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
			assert(id < _array.size() && !alreadeyRemoved());
			_nextFreeIndex = *static_cast<const uint32 *const>(std::addressof(_array[id]));
			new	(std::addressof(_array[id])) T{ std::forward<params>(p)... };
		}
		++_size;
		return id;
	}

	constexpr void remove(uint32 id)
	{
		assert(id < _array.size() && !alreadeyRemoved());
		T& item{ _array[id] };
		item.~T();
		DEBUG_OP(memset(std::addressof(_array[id], 0xcc, sizeof(T))));
		*static_cast<uint32*>(std::addressof(item)) = _nextFreeIndex;
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

	constexpr uint32 empty() const
	{
		return _size == 0;
	}

	[[nodiscard]] constexpr T& operator[](uint32 id)
	{
		assert(id < _array.size() && !alreadeyRemoved());
		return _array[id];
	}

	[[nodiscard]] constexpr const T& operator[](uint32 id) const
	{
		assert(id < _array.size() && !alreadeyRemoved());
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
	utl::vector<T>		_array;
	uint32				_nextFreeIndex{ uint32_invalid_id };
	uint32				_size{ 0 };
};

} // namespace zone::utl