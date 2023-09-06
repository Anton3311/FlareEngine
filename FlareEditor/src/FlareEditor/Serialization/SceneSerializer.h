#pragma once

#include "Flare/Scene/Scene.h"

#include <filesystem>

namespace Flare
{
	class SceneSerializer
	{
	public:
		static void Serialize(const Ref<Scene>& scene);
		static void Serialize(const Ref<Scene>& scene, const std::filesystem::path& path);
		static void Deserialize(const Ref<Scene>& scene, const std::filesystem::path& path);
	};
}