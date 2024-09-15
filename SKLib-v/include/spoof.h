#pragma once

#include "cpp.h"
#include "gpuspoof.h"
#include "efispoof.h"
#include "diskspoof.h"
#include "volumespoof.h"

namespace spoofer {
	NTSTATUS SpoofAll(DWORD64 seed = TEST_SEED);

	extern DWORD64 seed;
}
