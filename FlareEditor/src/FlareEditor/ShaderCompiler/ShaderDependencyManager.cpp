#include "PCH.h"
#include "ShaderDependencyManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Project/Project.h"
#include "Flare/Serialization/Serialization.h"

#include "Flare/Renderer/ShaderMetadata.h"

#include "FlareEditor/AssetManager/EditorShaderCache.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	static ShaderDependencyManager* s_Instance = nullptr;

	ShaderDependencyManager::ShaderDependencyManager()
	{
		FLARE_CORE_ASSERT(s_Instance == nullptr);
		s_Instance = this;

		Deserialize();
	}

	ShaderDependencyManager::~ShaderDependencyManager()
	{
		Serialize();
		s_Instance = nullptr;
	}

	bool ShaderDependencyManager::IsShaderUpToDate(AssetHandle shaderHandle) const
	{
		FLARE_PROFILE_FUNCTION();
		auto it = m_DependencyInfo.find(shaderHandle);
		FLARE_CORE_ASSERT(it != m_DependencyInfo.end());

		Ref<EditorAssetManager> assetManager = EditorAssetManager::GetInstance();
		FLARE_CORE_ASSERT(assetManager->IsAssetHandleValid(shaderHandle));

		const ShaderDependencyInfo& info = it->second;
		const AssetMetadata* shaderAssetMetadata = assetManager->GetAssetMetadata(shaderHandle);
		FLARE_CORE_ASSERT(shaderAssetMetadata);

		ShaderStageType allShaderStages[] = { ShaderStageType::Vertex, ShaderStageType::Pixel, ShaderStageType::Compute };
		auto& cacheManager = EditorShaderCache::GetInstance();

		std::filesystem::file_time_type minWriteTime = std::filesystem::file_time_type::min();
		for (ShaderStageType shaderStage : allShaderStages)
		{
			if (cacheManager.HasCache(shaderHandle, shaderStage))
			{
				std::filesystem::path cachePath = cacheManager.GetCacheFilePath(shaderHandle, shaderStage);
				auto writeTime = std::filesystem::last_write_time(cachePath);

				if (minWriteTime == std::filesystem::file_time_type::min() || writeTime < minWriteTime)
				{
					minWriteTime = writeTime;
				}
			}
		}

		FLARE_CORE_ASSERT(minWriteTime != std::filesystem::file_time_type::min());

		for (AssetHandle dependency : info.Dependencies)
		{
			const AssetMetadata* metadata = assetManager->GetAssetMetadata(dependency);
			FLARE_CORE_ASSERT(metadata);

			std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(metadata->Path);
			if (lastWriteTime > minWriteTime)
			{
				return false;
			}
		}

		return true;
	}

	void ShaderDependencyManager::SetShaderDependencies(AssetHandle shaderHandle, std::vector<AssetHandle>&& dependencies)
	{
		m_DependencyInfo[shaderHandle].ShaderHandle = shaderHandle;
		m_DependencyInfo[shaderHandle].Dependencies = std::move(dependencies);

		m_NeedsSerializing = true;
	}

	ShaderDependencyManager& ShaderDependencyManager::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance);
		return *s_Instance;
	}

	void ShaderDependencyManager::Serialize()
	{
		FLARE_PROFILE_FUNCTION();

		YAML::Emitter emitter;

		emitter << YAML::BeginSeq;

		for (const auto& [shaderHandle, info] : m_DependencyInfo)
		{
			emitter << YAML::BeginMap;
			emitter << YAML::Key << "ShaderHandle" << YAML::Value << (UUID)shaderHandle;
			emitter << YAML::Key << "Dependencies" << YAML::Value << YAML::BeginSeq;

			for (AssetHandle dependencyHandle : info.Dependencies)
			{
				emitter << (UUID)dependencyHandle;
			}

			emitter << YAML::EndSeq;
			emitter << YAML::EndMap;
		}

		emitter << YAML::EndSeq; // Entries

		std::filesystem::path outputPath = GetSerializationLocation();
		std::ofstream outputStream(outputPath);
		outputStream << emitter.c_str();

		m_NeedsSerializing = false;
	}

	void ShaderDependencyManager::Deserialize()
	{
		FLARE_PROFILE_FUNCTION();
		std::filesystem::path serializationLocation = GetSerializationLocation();
		if (!std::filesystem::exists(serializationLocation))
		{
			return;
		}

		try
		{
			YAML::Node node = YAML::LoadFile(serializationLocation.string());

			for (YAML::Node entry : node)
			{
				if (const YAML::Node shaderHandleNode = entry["ShaderHandle"])
				{
					AssetHandle shaderHandle = shaderHandleNode.as<AssetHandle>();

					std::vector<AssetHandle> dependencies;
					if (const YAML::Node dependenciesNode = entry["Dependencies"])
					{
						for (YAML::Node dependency : dependenciesNode)
						{
							dependencies.push_back(dependency.as<AssetHandle>());
						}

						m_DependencyInfo[shaderHandle].ShaderHandle = shaderHandle;
						m_DependencyInfo[shaderHandle].Dependencies = std::move(dependencies);
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			FLARE_CORE_ERROR(e.what());
		}
	}

	std::filesystem::path ShaderDependencyManager::GetSerializationLocation() const
	{
		return Project::GetActive()->Location / "Cache/ShaderDependencies.yaml";
	}
}
