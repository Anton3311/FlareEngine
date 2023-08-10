#pragma once

#include "Flare/AssetManager/Asset.h"

#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace YAML
{
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& vector)
		{
			Node node;
			node.push_back(vector.x);
			node.push_back(vector.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& out)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			out.x = node[0].as<float>();
			out.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& vector)
		{
			Node node;
			node.push_back(vector.x);
			node.push_back(vector.y);
			node.push_back(vector.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& out)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			out.x = node[0].as<float>();
			out.y = node[1].as<float>();
			out.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& vector)
		{
			Node node;
			node.push_back(vector.x);
			node.push_back(vector.y);
			node.push_back(vector.z);
			node.push_back(vector.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& out)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			out.x = node[0].as<float>();
			out.y = node[1].as<float>();
			out.z = node[2].as<float>();
			out.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Flare::AssetHandle>
	{
		static Node encode(const Flare::AssetHandle& handle)
		{
			return Node((uint64_t)handle);
		}

		static bool decode(const Node& node, Flare::AssetHandle& out)
		{
			if (!node.IsScalar())
				return false;

			out = node.as<uint64_t>();
			return true;
		}
	};
}

inline YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec2& vector)
{
	emitter << YAML::Flow;
	emitter << YAML::BeginSeq << vector.x << vector.y << YAML::EndSeq;
	return emitter;
}

inline YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec3& vector)
{
	emitter << YAML::Flow;
	emitter << YAML::BeginSeq << vector.x << vector.y << vector.z << YAML::EndSeq;
	return emitter;
}

inline YAML::Emitter& operator<<(YAML::Emitter& emitter, const glm::vec4& vector)
{
	emitter << YAML::Flow;
	emitter << YAML::BeginSeq << vector.x << vector.y << vector.z << vector.w << YAML::EndSeq;
	return emitter;
}

inline YAML::Emitter& operator<<(YAML::Emitter& emitter, Flare::AssetHandle handle)
{
	emitter << (uint64_t)handle;
	return emitter;
}