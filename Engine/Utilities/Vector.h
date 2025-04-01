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
class vector
{
public:
	// Default constructor. Doesn't allocate memory
	vector() = default;

	// Constructor that allocates memory for count elements
	constexpr vector(uint64 count)
	{
		resize(count);
	}

	// Constructor resizes the vector and initializes 'count' items using 'value'.
	constexpr explicit vector(uint64 count, const T& value)
	{
		resize(count, value);
	}

	template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
	constexpr explicit vector(it first, it last)
	{
		for (; first != last; ++first)
		{
			emplace_back(*first);
		}
	}

	constexpr vector(const vector& other)
	{
		*this = other;
	}

	constexpr vector(vector&& other) : _capacity{ other._capacity }, _size{ other._size }, _data{ other._data }
	{
		other.reset();
	}

	constexpr vector& operator= (const vector& other)
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

	constexpr vector& operator= (vector&& other)
	{
		assert(this != std::addressof(other));
		if (this != std::addressof(other))
		{
			destroy();
			move(other);
		}

		return *this;
	}

	~vector() 
	{
		destroy();
	}

	constexpr void push_back(const T& value)
	{
		emplace_back(value);
	}

	constexpr void push_back(T&& value)
	{
		emplace_back(std::move(value));
	}

	// copy- or move-constructs an item at the end of the vector.
	template<typename... params>
	constexpr decltype(auto) emplace_back(params&&... p)
	{
		if (_size == _capacity)
		{
			reserve(((_capacity + 1) * 3) >> 1); // reserve 50% more
		}
		assert(_size < _capacity);

		new (std::addressof(_data[_size])) T(std::forward<params>(p)...);
		++_size;
		return _data[_size - 1];
	}

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

	// Removes the item at specified index
	constexpr T *const erase(uint64 index)
	{
		assert(_data && index < _size);
		return erase(std::addressof(_data[index]));
	}

	// Removes the item at specified location.
	constexpr T *const erase(T *const item)
	{
		assert(_data && item >= std::addressof(_data[0]) && item < std::addressof(_data[_size]));
		if constexpr(destruct)
		{
			item->~T();
		}
		--_size;
		if (item < std::addressof(_data[_size]))
		{
			memcpy(item, item + 1, (std::addressof(_data[_size]) - item) * sizeof(T));
		}

		return item;
	}

	// Same as erase() but faster(copy the last item.)
	constexpr T *const erase_unordered(uint64 index)
	{
		assert(_data && index < _size);
		return erase_unordered(std::addressof(_data[index]));
	}

	// Same as erase() but faster(copy the last item.)
	constexpr T *const erase_unordered(T *const item)
	{
		assert(_data && item >= std::addressof(_data[0]) && item < std::addressof(_data[_size]));
		if constexpr (destruct)
		{
			item->~T();
		}
		--_size;
		if (item < std::addressof(_data[_size]))
		{
			memcpy(item, std::addressof(_data[_size]), sizeof(T));
		}

		return item;
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

	// Swaps two vectors
	constexpr void swap(vector& other)
	{
		if (this != std::addressof(other))
		{
			auto temp(other);
			other = *this;
			*this = temp;
		}
	}

	// Pointer to the start of data. Might be null.
	[[nodiscard]] constexpr T* data()
	{
		return _data;
	}

	// Pointer to the start of data. Might be null.
	[[nodiscard]] constexpr T *const data() const
	{
		return _data;
	}

	// Returns true if vector is empty.
	[[nodiscard]] constexpr bool empty() const
	{
		return _size == 0;
	}

	// Return the number of items in the vector.
	[[nodiscard]] constexpr uint64 size() const
	{
		return _size;
	}

	// Returns the current capacity of the vector.
	[[nodiscard]] constexpr uint64 capacity() const
	{
		return _capacity;
	}

	// Indexing operator. Returns a reference to the item at specified index.
	[[nodiscard]] constexpr T& operator[](uint64 index)
	{
		assert(_data && index < _size);
		return _data[index];
	}

	// Indexing operator. Returns a constant reference to the item at specified index.
	[[nodiscard]] constexpr const T& operator[](uint64 index) const
	{
		assert(_data && index < _size);
		return _data[index];
	}

	// Returns a reference to the first item. 
	[[nodiscard]] constexpr T& front()
	{
		assert(_data && _size);
		return _data[0];
	}

	// Returns a constant reference to the first item. 
	[[nodiscard]] constexpr const T& front() const
	{
		assert(_data && _size);
		return _data[0];
	}

	// Returns a reference to the last item. 
	[[nodiscard]] constexpr T& back()
	{
		assert(_data && _size);
		return _data[_size - 1];
	}

	// Returns a constant reference to the last item.
	[[nodiscard]] constexpr const T& back() const
	{
		assert(_data && _size);
		return _data[_size - 1];
	}

	// Returns a pointer to the first item. 
	[[nodiscard]] constexpr T* begin()
	{
		assert(_data);
		return std::addressof(_data[0]);
	}

	// Returns a constant pointer to the first item. 
	[[nodiscard]] constexpr const T* begin() const
	{
		assert(_data);

		return std::addressof(_data[0]);
	}

	// Returns a pointer to the last item. 
	[[nodiscard]] constexpr T* end()
	{
		assert(_data);
		return std::addressof(_data[_size]);
	}

	// Returns a constant pointer to the last item. 
	[[nodiscard]] constexpr const T* end() const
	{
		assert(_data);
		return std::addressof(_data[_size]);
	}
private:

	constexpr void move(vector& other)
	{
		_capacity = other._capacity;
		_size = other._size;
		_data = other._data;
		other.reset();
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