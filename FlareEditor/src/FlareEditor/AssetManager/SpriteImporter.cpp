#include "SpriteImporter.h"

#include "FlareCore/Profiler/Profiler.h"
#include "FlareEditor/Serialization/YAMLSerialization.h"

#include <fstream>

namespace Flare
{
	bool SpriteImporter::SerializeSprite(const Ref<Sprite>& sprite, const std::filesystem::path& path)
	{
		FLARE_PROFILE_FUNCTION();
		YAML::Emitter emitter;
		YAMLSerializer serializer(emitter, nullptr);

		serializer.Serialize(SerializationValue(*sprite));

		std::ofstream output(path);
		if (!output.is_open())
		{
			FLARE_CORE_ERROR("Failed to serialize Sprite: {}", path.string());
			return false;
		}

		output << emitter.c_str();
		output.close();

		return true;
	}

	bool SpriteImporter::DeserializeSprite(const Ref<Sprite>& sprite, const std::filesystem::path& path)
	{
		FLARE_PROFILE_FUNCTION();
		if (!std::filesystem::exists(path))
			return false;

		YAML::Node node = YAML::LoadFile(path.generic_string());
		
		YAMLDeserializer deserializer(node);
		deserializer.Serialize(SerializationValue(*sprite));

		return true;
	}
	
	Ref<Asset> SpriteImporter::ImportSprite(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<Sprite> sprite = CreateRef<Sprite>();
		if (!DeserializeSprite(sprite, metadata.Path))
			return nullptr;

		return sprite;
	}
}
