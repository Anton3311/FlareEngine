#include "Serialization.h"

YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec3& vector)
{
	emitter << YAML::Flow;
	emitter << YAML::BeginSeq << vector.x << vector.y << vector.z << YAML::EndSeq;
	return emitter;
}

YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec4& vector)
{
	emitter << YAML::Flow;
	emitter << YAML::BeginSeq << vector.x << vector.y << vector.z << vector.w << YAML::EndSeq;
	return emitter;
}

YAML::Emitter& operator<<(YAML::Emitter& emitter, Flare::AssetHandle handle)
{
	emitter << (uint64_t)handle;
	return emitter;
}