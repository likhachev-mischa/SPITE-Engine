#pragma once

namespace spite
{
	// An interface that defines the contract for API-specific window operations,
	// such as creating a rendering surface or querying required extensions.
	class IWindowApiBinding
	{
	public:
		virtual ~IWindowApiBinding() = default;
	};
}
