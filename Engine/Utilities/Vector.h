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

	// Constructor resizes the vector and initializes 'count' items using 'value'.
	constexpr explicit Vector(uint64 count, const T& value)
	{
		resize(count, value);
	}

	constexpr Vector(const Vector& other)
	{
		*this = other;
	}

	constexpr Vector(const Vector&& other) : _capacity{ other._capacity }, _size{ other._size }, _data{ other._data }
	{
		other.reset();
	}

	constexpr Vector& operator= (const Vector& other)
	{
		assert(this != std::addressof(other));
		if (this != std::addressof(other))
		{
			clear();
			reserve(other._size);
			for (auto& item : other)
			{
				emplace_back(item);
			}
			assert(_size == other._size);
		}

		return *this;
	}

	constexpr Vector& operator= (Vector&& other)
	{
		assert(this != std::addressof(other));
		if (this != std::addressof(other))
		{
			destroy();
			move(other);
		}

		return *this;
	}

	~Vector() 
	{
		destroy();
	}

	// copy- or move-constructs an item at the end of the vector.

	// Resizes the vector and initializes new items with their default value.
	constexpr void resize(uint64 newSize)
	{
		static_assert(std::is_default_constructible_v<T>, "Type must be default_constructible.");

		if (newSize > _size)
		{
			reserve(newSize);
			while (_size < newSize)
			{
				emplace_back();
			}
		}

		else if (newSize < _size)
		{
			if constexpr (destruct)
			{
				destructRange(newSize, _size);
			}
		}

		assert(newSize == _size);
	}

	// Resizes the vector and initializes new items by copying 'value'.
	constexpr void resize(uint64 newSize, const T& value)
	{
		static_assert(std::is_copy_constructible_v<T>, "Type must be copy_constructible.");

		if (newSize > _size)
		{
			reserve(newSize);
			while (_size < newSize)
			{
				emplace_back(value);
			}
		}

		else if (newSize < _size)
		{
			if constexpr (destruct)
			{
				destructRange(newSize, _size);
			}
		}

		assert(newSize == _size);
	}

	// Allocates memory
	constexpr void reserve(uint64 newCapacity)
	{
		if (newCapacity > _capacity)
		{
			// NOTE: realloc() will automatically copy the data in the buffer
			//		 if a new region of memory is allocated
			void* newBuffer{ realloc(_data, newCapacity * sizeof(T)) };
			assert(newBuffer);
			if (newBuffer)
			{
				_data = static_cast<T*>(newBuffer);
				_capacity = newCapacity;
			}
		}
	}

	// Clears the vector and destructs items as specified in template argument.
	constexpr void clear()
	{
		if constexpr (destruct)
		{
			destructRange(0, _size);
		}
		_size = 0;
	}

private:

	constexpr void move(Vector& other)
	{
		_capacity = other._capacity;
		_size = other._size;
		_data = other._data;
	}

	constexpr void reset()
	{
		_capacity = 0;
		_size = 0;
		_data = nullptr;
	}

	constexpr void destructRange(uint64 first, uint64 last)
	{
		assert(destruct);
		assert(first <= _size && last <= _size && first <= last);
		if (_data)
		{
			for (; first != last; ++first)
			{
				_data[first].~T();
			}
		}
	}

	constexpr void destroy()
	{
		assert([&] {return _capacity ? _data != nullptr : _data == nullptr; }());
		clear();
		_capacity = 0;
		if (_data)
		{
			free(_data);
		}
		_data = nullptr;
	}

	uint64		_capacity{ 0 };
	uint64		_size{ 0 };
	T*			_data{ nullptr };
};
}