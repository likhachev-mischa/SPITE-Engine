#include "NamedBufferRegistry.hpp"
#include "IRenderDevice.hpp"
#include "IRenderResourceManager.hpp"
#include "base/Assert.hpp"
#include "base/CollectionUtilities.hpp"
#include "base/StringInterner.hpp"

namespace spite
{
	NamedBufferRegistry::NamedBufferRegistry(IRenderDevice& device, const HeapAllocator& allocator)
		: m_device(device), m_namedBuffers(makeHeapMap<HashedString, std::pair<BufferHandle, BufferDesc>>(allocator))
	{
	}

	BufferHandle NamedBufferRegistry::createBuffer(HashedString name, const BufferDesc& desc)
	{
		SASSERTM(m_namedBuffers.find(name) == m_namedBuffers.end(),
		         "A buffer with this name is already registered.")

		BufferHandle handle = m_device.createBuffer(desc);
		m_namedBuffers[name] = {handle, desc};
		return handle;
	}

	BufferHandle NamedBufferRegistry::getOrCreateBuffer(HashedString name, const BufferDesc& desc)
	{
		auto it = m_namedBuffers.find(name);
		if (it != m_namedBuffers.end())
		{
			const auto& existingDesc = it->second.second;
			SASSERTM(existingDesc.size == desc.size, "Named buffer exists with a different size.")
			SASSERTM(existingDesc.usage == desc.usage, "Named buffer exists with a different usage.")
			return it->second.first;
		}

		BufferHandle handle = m_device.createBuffer(desc);
		m_namedBuffers.emplace(name, std::make_pair(handle, desc));
		return handle;
	}

	BufferHandle NamedBufferRegistry::getBuffer(HashedString name) const
	{
		auto it = m_namedBuffers.find(name);
		SASSERTM(it != m_namedBuffers.end(), "Named buffer not found.")
		return it->second.first;
	}

	const BufferDesc& NamedBufferRegistry::getBufferDesc(HashedString name) const
	{
		auto it = m_namedBuffers.find(name);
		SASSERTM(it != m_namedBuffers.end(), "Named buffer not found.")
		return it->second.second;
	}

	void NamedBufferRegistry::updateBuffer(HashedString name, const void* data, sizet size, sizet offset) const
	{
		BufferHandle handle = getBuffer(name);
		m_device.getResourceManager().updateBuffer(handle, data, size, offset);
	}

	void NamedBufferRegistry::updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset) const
	{
		m_device.getResourceManager().updateBuffer(handle, data, size, offset);
	}
}
