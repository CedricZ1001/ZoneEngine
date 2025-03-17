// Copyright (c) CedricZ1, 2025
// Distributed under the MIT license. See the LICENSE file in the project root for more information.
#pragma once
#include "Test.h"

class EngineTest : public Test
{
public:
	bool initialize() override;
	void run() override;
	void shutdown() override;
protected:
private:
};