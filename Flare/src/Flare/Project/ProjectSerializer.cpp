#include "ProjectSerializer.h"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace Flare
{
	std::filesystem::path ProjectSerializer::s_ProjectFileExtension = ".flareproj";

	void ProjectSerializer::Serialize(const Ref<Project>& project, const std::filesystem::path& path)
	{
		YAML::Emitter emitter;
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Project" << YAML::BeginMap;
		emitter << YAML::Key << "Name" << project->Name << YAML::Value;
		emitter << YAML::Key << "StartScene" << project->StartScene << YAML::Value;

		emitter << YAML::Key << "ScriptingModules" << YAML::BeginSeq;
		for (const std::filesystem::path& modulePath : project->ScriptingModules)
			emitter << YAML::Value << std::filesystem::relative(modulePath, project->Location).generic_string();
		emitter << YAML::EndSeq;

		emitter << YAML::EndMap;
		emitter << YAML::EndMap;

		std::ofstream output(path);
		if (!output.is_open())
		{
			FLARE_CORE_ERROR("Failed to write project file {0}", path.generic_string());
			return;
		}

		output << emitter.c_str();
	}

	void ProjectSerializer::Deserialize(const Ref<Project>& project, const std::filesystem::path& path)
	{
		std::ifstream input(path);

		if (!input.is_open())
		{
			FLARE_CORE_ERROR("Failed to read project file {0}", path.generic_string());
			return;
		}

		YAML::Node node = YAML::Load(input);

		if (YAML::Node projectNode = node["Project"])
		{
			if (YAML::Node nameNode = projectNode["Name"])
				project->Name = nameNode.as<std::string>();
			if (YAML::Node startSceneNode = projectNode["StartScene"])
				project->StartScene = startSceneNode.as<AssetHandle>();
			if (YAML::Node scriptingModules = projectNode["ScriptingModules"])
			{
				for (YAML::Node modulePath : scriptingModules)
					project->ScriptingModules.push_back(project->Location / modulePath.as<std::string>());
			}
		}
		else
		{
			FLARE_CORE_ERROR("Failed to parse project file {0}", path.generic_string());
		}
	}
}
