#include "PCH.h"
#include "HierarchyCommands.h"

#include "Flare/Scene/Hierarchy.h"

namespace Flare
{
	SetParentCommand::SetParentCommand(FutureEntity targetEntity, FutureEntity newParent)
		: m_NewParent(newParent), m_TargetEntity(targetEntity) {}

	void SetParentCommand::Apply(CommandContext& context, World& world)
	{
		Entity child = context.GetEntity(m_TargetEntity);
		Entity parent = context.GetEntity(m_NewParent);
		HierarchyHelper::SetParent(world, child, parent);
	}

	DetachFromParentCommand::DetachFromParentCommand(FutureEntity entity)
		: m_Entity(entity)
	{
	}

	void DetachFromParentCommand::Apply(CommandContext& context, World& world)
	{
		HierarchyHelper::DetachFromParent(world, context.GetEntity(m_Entity));
	}
}
