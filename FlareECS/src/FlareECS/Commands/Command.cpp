#include "Command.h"

#include "FlareECS/Commands/CommandsStorage.h"

namespace Flare
{
    Entity CommandContext::GetEntity(FutureEntity futureEntity)
    {
        auto entity = m_Storage.Read<Entity>(futureEntity.Location);
        FLARE_CORE_ASSERT(entity.has_value());

        return *entity.value();
    }

    void CommandContext::SetEntity(FutureEntity futureEntity, Entity entity)
    {
        FLARE_CORE_ASSERT(m_Storage.Write<Entity>(futureEntity.Location, entity));
    }
}
