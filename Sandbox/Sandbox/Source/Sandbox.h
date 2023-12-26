#pragma once

#include <FlareCore/Serialization/TypeInitializer.h>
#include <FlareCore/Serialization/TypeSerializer.h>
#include <FlareCore/Serialization/SerializationStream.h>

#include <Flare/AssetManager/AssetManager.h>

#include <FlareECS/System/SystemInitializer.h>
#include <FlareECS/Entity/ComponentInitializer.h>

namespace Sandbox
{
	using namespace Flare;
	struct RotatingQuadData
	{
		FLARE_COMPONENT;
		float RotationSpeed;
		AssetHandle PrefabHandle;
	};

	struct SomeComponent
	{
		FLARE_COMPONENT;

		int a, b;

		SomeComponent()
			: a(100), b(-234) {}
	};

	struct TestComponent
	{
		FLARE_COMPONENT;
		int a;

		TestComponent()
			: a(1000) {}
	};
}

template<>
struct Flare::TypeSerializer<Sandbox::RotatingQuadData>
{
	void OnSerialize(Sandbox::RotatingQuadData& data, Flare::SerializationStream& stream)
	{
		stream.Serialize("RotationSpeed", Flare::SerializationValue(data.RotationSpeed));
		stream.Serialize("PrefabHandle", Flare::SerializationValue(data.PrefabHandle));
	}
};

template<>
struct Flare::TypeSerializer<Sandbox::SomeComponent>
{
	void OnSerialize(Sandbox::SomeComponent& data, Flare::SerializationStream& stream)
	{
		stream.Serialize("a", Flare::SerializationValue(data.a));
		stream.Serialize("b", Flare::SerializationValue(data.b));
	}
};

template<>
struct Flare::TypeSerializer<Sandbox::TestComponent>
{
	void OnSerialize(Sandbox::TestComponent& data, Flare::SerializationStream& stream)
	{
		stream.Serialize("a", Flare::SerializationValue(data.a));
	}
};
