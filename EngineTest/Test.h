// Copyright (c) CedricZ1, 2024
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include<thread>
#include<chrono>
#include<string>

#define TEST_ENTITY_COMPONENTS 0
#define TEST_WINDOW 0
#define TEST_RENDERER 1

class Test
{
public:
	virtual bool initialize() = 0;
	virtual void run() = 0;
	virtual void shutdown() = 0;
};

#if _WIN64
#include <Windows.h>
class TimeIt
{
public:
	using clock = std::chrono::high_resolution_clock;
	using timeStamp = std::chrono::steady_clock::time_point;

	void begin()
	{
		_start = clock::now();
	}

	void end()
	{
		auto deltaTime = clock::now() - _start;
		_msAvg += (static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(deltaTime).count() - _msAvg) / static_cast<float>(_counter));
		++_counter;

		if (std::chrono::duration_cast<std::chrono::seconds>(clock::now() - _seconds).count() >= 1)
		{
			OutputDebugStringA("Avg. frame (ms): ");
			OutputDebugStringA(std::to_string(_msAvg).c_str());
			OutputDebugStringA((" " + std::to_string(_counter)).c_str());
			OutputDebugStringA(" FPS");
			OutputDebugStringA("\n");
			_msAvg = 0.0f;
			_counter = 1;
			_seconds = clock::now();
		}
	}

private:
	float		_msAvg{ 0.0f };
	int		_counter{ 1 };
	timeStamp	_start;
	timeStamp	_seconds{ clock::now() };
};
#endif // _WIN64