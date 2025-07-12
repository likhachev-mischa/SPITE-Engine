#include "ComponentMetadata.hpp"

namespace spite
{
	ComponentMetadata::ComponentMetadata(ComponentID id,
	                                     std::type_index type,
	                                     sizet size,
	                                     sizet alignment,
	                                     bool isTriviallyRelocatable,
	                                     DestructorFn destructorFn,
	                                     void* destructorUserData,
	                                     MoveAndDestroyFn moveAndDestroyFn): id(id), type(type), size(size),
	                                                                         alignment(alignment),
	                                                                         isTriviallyRelocatable(
		                                                                         isTriviallyRelocatable),
	                                                                         destructor(destructorFn),
	                                                                         destructorUserData(destructorUserData),
	                                                                         moveAndDestroy(moveAndDestroyFn)
	{
	}
}
