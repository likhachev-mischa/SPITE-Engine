#include "NamedBufferRegistry.hpp"
#include "IRenderDevice.hpp"
#include "IRenderResourceManager.hpp"
#include "base/Assert.hpp"

namespace spite
{
	NamedBufferRegistry::NamedBufferRegistry(IRenderDevice& device)
		: m_device(device)
	{
	}

	BufferHandle NamedBufferRegistry::createBuffer(const heap_string& name, const BufferDesc& desc)
	{
		SASSERTM(m_namedBuffers.find(name) == m_namedBuffers.end(), "A buffer with this name is already registered.")

		BufferHandle handle = m_device.createBuffer(desc);
		m_namedBuffers[name] = {handle, desc};
		return handle;
	}

	BufferHandle NamedBufferRegistry::getBuffer(const heap_string& name) const
	{
		auto it = m_namedBuffers.find(name);
		SASSERTM(it != m_namedBuffers.end(), "Named buffer not found.")
		return it->second.first;
	}

	const BufferDesc& NamedBufferRegistry::getBufferDesc(const heap_string& name) const
	{
		auto it = m_namedBuffers.find(name);
		SASSERTM(it != m_namedBuffers.end(), "Named buffer not found.")
		return it->second.second;
	}

	void NamedBufferRegistry::updateBuffer(const heap_string& name, const void* data, sizet size, sizet offset) const
	{
		BufferHandle handle = getBuffer(name);
		m_device.getResourceManager().updateBuffer(handle, data, size, offset);
	}

	void NamedBufferRegistry::updateBuffer(BufferHandle handle, const void* data, sizet size, sizet offset) const
	{
		m_device.getResourceManager().updateBuffer(handle, data, size, offset);
	}

	BufferHandle NamedBufferRegistry::getOrCreateBuffer(const heap_string& name, const BufferDesc& desc)
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
}
