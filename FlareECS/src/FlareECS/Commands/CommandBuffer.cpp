#include "CommandBuffer.h"

namespace Flare
{
	CommandBuffer::CommandBuffer()
		: m_Storage(2048)
	{

	}

	void CommandBuffer::Execute(World& world)
	{
		while (m_Storage.CanRead())
		{
			auto [meta, command] = m_Storage.Pop();
			meta.Apply((Command*)command, world);
		}

		m_Storage.Clear();
	}
}
