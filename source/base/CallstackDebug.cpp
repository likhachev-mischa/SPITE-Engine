#include "CallstackDebug.hpp"

#include <external/tracy/client/TracyCallstack.hpp>
#include <external/tracy/common/TracyAlloc.hpp>

#include "base/Logging.hpp"


namespace spite
{
#ifdef TRACY_HAS_CALLSTACK 
	void logCallstack(int depth, const char* context_message)
	{
		if (context_message)
		{
			SDEBUG_LOG("Callstack requested: %s\n", context_message)
		}

		// Ensure Tracy's callstack system is initialized.
		// This is often handled by Tracy's main initialization (e.g. FrameMark).
		// If using callstack functions in a very isolated way, you might need to ensure
		// tracy::InitCallstack() has been called at startup and tracy::EndCallstack() at shutdown.
		// For most use cases where Tracy is active, this should be fine.

		void* callstack_raw = tracy::Callstack(depth);
		if (!callstack_raw)
		{
			SDEBUG_LOG("Failed to capture callstack (is TRACY_HAS_CALLSTACK enabled and Tracy initialized?).\n");
			return;
		}

		uintptr_t* callstack_frames_ptr = (uintptr_t*)callstack_raw;
		uintptr_t frame_count = *callstack_frames_ptr++; // The first element is the count of frames

		SDEBUG_LOG("--- Callstack (%zu frames) ---\n", (size_t)frame_count);

		for (uintptr_t i = 0; i < frame_count; ++i)
		{
			uint64_t frame_address = (uint64_t)callstack_frames_ptr[i];
			const char* sym_name = tracy::DecodeCallstackPtrFast(frame_address);
			// SDEBUG_LOG uses printf-style formatting.
			// We need to ensure sym_name is not null for %s.
			SDEBUG_LOG("  [%2zu] 0x%016llx - %s\n",
			           (size_t)i,
			           frame_address,
			           sym_name ? sym_name : "[unknown symbol]");
		}
		SDEBUG_LOG("--- End of Callstack ---\n");

		tracy::tracy_free(callstack_raw); // Free the memory allocated by tracy::Callstack
	}
#else
	void logCallstack(int depth = 15, const char* context_message = nullptr)
	{
		if (context_message)
		{
			SDEBUG_LOG("Callstack requested: %s (TRACY_HAS_CALLSTACK not enabled)\n", context_message);
		}
		else
		{
			SDEBUG_LOG("Callstack requested (TRACY_HAS_CALLSTACK not enabled)\n");
		}
	}
#endif // TRACY_HAS_CALLSTACK
}
