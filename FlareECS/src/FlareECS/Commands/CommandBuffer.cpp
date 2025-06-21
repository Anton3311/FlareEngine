#include "CommandBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

namespace Flare
{
	EntitiesCommandBuffer::EntitiesCommandBuffer()
		: m_Storage(2048)
	{

	}

	FutureEntityCommands EntitiesCommandBuffer::GetEntity(Entity entity)
	{
		return AddEntityCommand<GetEntityCommand>(GetEntityCommand(entity));
	}

	void EntitiesCommandBuffer::DeleteEntity(Entity entity)
	{
		AddCommand<DeleteEntityCommand>(DeleteEntityCommand(entity));
	}

	size_t EntitiesCommandBuffer::Execute(World& world)
	{
		FLARE_PROFILE_FUNCTION();
		if (!m_Storage.CanRead())
			return 0;

		size_t commandCount = 0;

		while (m_Storage.CanRead())
		{
			auto [meta, command] = m_Storage.Pop();

			CommandContext context(meta, m_Storage);

			command->Apply(context, world);
			command->~Command();

			commandCount++;
		}

		m_Storage.Clear();

		return commandCount;
	}
}
