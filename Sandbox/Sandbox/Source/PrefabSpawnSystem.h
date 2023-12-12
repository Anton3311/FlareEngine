#pragma once

#include <FlareECS/System/SystemInitializer.h>
#include <FlareECS/Entity/ComponentInitializer.h>

#include <Flare/AssetManager/Asset.h>
#include <Flare/AssetManager/AssetManager.h>

struct PrefabSpawner
{
	FLARE_COMPONENT;

	PrefabSpawner()
		: Enabled(false), PrefabHandle(12561055223819313244), Period(1.0f), TimeLeft(0.0f) {}

	bool Enabled;
	Flare::AssetHandle PrefabHandle;

	float TimeLeft;
	float Period;
};

struct PrefabSpawnSystem : Flare::System
{
	FLARE_SYSTEM;

	virtual void OnConfig(Flare::SystemConfig& config) override;
	virtual void OnUpdate(Flare::SystemExecutionContext& context) override;
private:
	Flare::Query m_Query;
};
