#pragma once

namespace spite
{
	enum class ExecutionStage
	{
		// Runs first. Good for input polling, clearing buffers, etc.
		PreUpdate, 
		
		// Main game logic. Physics, AI, etc.
		Update,

		// Runs after main logic. Good for camera updates, preparing render data.
		PostUpdate, 

		// Runs last. For submitting render commands, cleanup.
		Render
	};
}
