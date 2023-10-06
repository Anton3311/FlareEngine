#include "CommandBuffer.h"

#include "FlareECS/World.h"

namespace Flare
{
	CommandBuffer::CommandBuffer(World& world)
		: m_Storage(2048), m_World(world)
	{

	}

	FutureEntityCommands CommandBuffer::GetEntity(Entity entity)
	{
		return AddEntityCommand<GetEntityCommand>(GetEntityCommand(entity));
	}

	void CommandBuffer::DeleteEntity(Entity entity)
	{
		AddCommand<DeleteEntityCommand>(DeleteEntityCommand(entity));
	}

	void CommandBuffer::Execute()
	{
		if (!m_Storage.CanRead())
			return;

		while (m_Storage.CanRead())
		{
			auto [meta, command] = m_Storage.Pop();

			CommandContext context(meta, m_Storage);

			command->Apply(context, m_World);
			command->~Command();
		}

		m_Storage.Clear();
	}
}
