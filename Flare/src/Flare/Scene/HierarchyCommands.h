#pragma once

#include "FlareECS/Commands/Command.h"

namespace Flare
{
	class FLARE_API SetParentCommand : public Command
	{
	public:
		SetParentCommand(FutureEntity targetEntity, FutureEntity newParent);

		virtual void Apply(CommandContext& context, World& world) override;
	private:
		FutureEntity m_NewParent;
		FutureEntity m_TargetEntity;
	};

	class FLARE_API DetachFromParentCommand : public Command
	{
	public:
		DetachFromParentCommand(FutureEntity entity);

		virtual void Apply(CommandContext& context, World& world) override;
	private:
		FutureEntity m_Entity;
	};
}
