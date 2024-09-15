#pragma once

#include "cpp.h"
#include "gpuspoof.h"
#include "diskspoof.h"

namespace spoofer {
	NTSTATUS SpoofAll(DWORD64 seed = TEST_SEED);

	extern DWORD64 seed;
}
