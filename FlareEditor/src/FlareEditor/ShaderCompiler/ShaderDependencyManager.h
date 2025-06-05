#pragma once

#include "Flare/AssetManager/Asset.h"

#include <unordered_map>
#include <filesystem>

namespace Flare
{
	struct ShaderDependencyInfo
	{
		AssetHandle ShaderHandle;
		std::vector<AssetHandle> Dependencies;
	};

	class ShaderDependencyManager
	{
	public:
		ShaderDependencyManager();
		~ShaderDependencyManager();

		inline bool NeedsSerializing() const { return m_NeedsSerializing; }
		inline bool ContainsShader(AssetHandle handle) const { return m_DependencyInfo.contains(handle); }

		// Checks whether none of the dependency files were written after any of the shader cache files
		bool IsShaderUpToDate(AssetHandle shaderHandle) const;
		void SetShaderDependencies(AssetHandle shaderHandle, std::vector<AssetHandle>&& dependencies);

		void Serialize();
		void Deserialize();

		static ShaderDependencyManager& GetInstance();
	private:
		std::filesystem::path GetSerializationLocation() const;
	private:
		std::unordered_map<AssetHandle, ShaderDependencyInfo> m_DependencyInfo;
		bool m_NeedsSerializing = false;
	};
}
