#pragma once

#include "GraphicsDescs.hpp"
#include "RenderResourceHandles.hpp"

#include "base/CollectionAliases.hpp"

namespace spite
{
	class IRenderDevice;

	// A simple registry for creating, storing, and accessing named buffers (UBOs, SSBOs).
	class NamedBufferRegistry
	{
	public:
		NamedBufferRegistry(IRenderDevice& device);

		// Creates a buffer and registers it with a unique name.
		// Asserts if a buffer with the same name already exists.
		BufferHandle createBuffer(const heap_string& name, const BufferDesc& desc);

		BufferHandle getOrCreateBuffer(const heap_string& name, const BufferDesc& desc);

		// Retrieves the handle for a named buffer.
		BufferHandle getBuffer(const heap_string& name) const;

		// Retrieves the description for a named buffer.
		const BufferDesc& getBufferDesc(const heap_string& name) const;

		// Convenience function to update a named buffer's contents.
		void updateBuffer(const heap_string& name, const void* data, sizet size, sizet offset = 0) const;

		void updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset = 0) const;

	private:
		IRenderDevice& m_device;
		heap_unordered_map<heap_string, std::pair<BufferHandle, BufferDesc>> m_namedBuffers;
	};
}
