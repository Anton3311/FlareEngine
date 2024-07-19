#pragma once

#include "Flare/Project/Project.h"
#include "Flare/Serialization/Serialization.h"

namespace Flare
{
	class ProjectSerializer
	{
	public:
		static void Serialize(const Ref<Project>& project, const std::filesystem::path& path);
		static void Deserialize(const Ref<Project>& project, const std::filesystem::path& path);
	private:
		static std::filesystem::path s_ProjectFileExtension;
	};
}