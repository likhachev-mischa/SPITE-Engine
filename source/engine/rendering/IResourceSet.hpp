#pragma once

namespace spite
{
    class IResourceSet
	{
	public:
		virtual ~IResourceSet() = default;

		virtual sizet getOffset() const = 0;
		virtual sizet getSize() const = 0;
	};
}