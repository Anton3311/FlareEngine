#pragma once

#include <FlareCore/Serialization/TypeSerializer.h>
#include <FlareCore/Serialization/SerializationStream.h>

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

template<>
struct Flare::TypeSerializer<PrefabSpawner>
{
	void OnSerialize(PrefabSpawner& prefabSpawner, Flare::SerializationStream& stream)
	{
		stream.Serialize("Enabled", Flare::SerializationValue(prefabSpawner.Enabled));
		stream.Serialize("PrefabHandle", Flare::SerializationValue(prefabSpawner.PrefabHandle));
		stream.Serialize("TimeLeft", Flare::SerializationValue(prefabSpawner.TimeLeft));
		stream.Serialize("Period", Flare::SerializationValue(prefabSpawner.Period));
	}
};

struct PrefabSpawnSystem : Flare::System
{
	FLARE_SYSTEM;

	virtual void OnConfig(Flare::SystemConfig& config) override;
	virtual void OnUpdate(Flare::SystemExecutionContext& context) override;
private:
	Flare::Query m_Query;
};
