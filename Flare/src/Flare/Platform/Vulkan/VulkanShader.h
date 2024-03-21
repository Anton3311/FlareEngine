#pragma once

#include "Flare/Renderer/Shader.h"

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

namespace Flare
{
	FLARE_API VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type);

	class VulkanShader : public Shader
	{
	public:
		~VulkanShader();

		VkShaderModule GetModuleForStage(ShaderStageType stage) const;

		void Load() override;
		bool IsLoaded() override;
		void Bind() override;
		const ShaderProperties& GetProperties() const override;
		const ShaderOutputs& GetOutputs() const override;
		ShaderFeatures GetFeatures() const override;
		std::optional<uint32_t> GetPropertyIndex(std::string_view name) const override;
		void SetInt(const std::string& name, int value) override;
		void SetFloat(const std::string& name, float value) override;
		void SetFloat2(const std::string& name, glm::vec2 value) override;
		void SetFloat3(const std::string& name, const glm::vec3& value) override;
		void SetFloat4(const std::string& name, const glm::vec4& value) override;
		void SetIntArray(const std::string& name, const int* values, uint32_t count) override;
		void SetMatrix4(const std::string& name, const glm::mat4& matrix) override;
	private:
		struct ShaderStageModule
		{
			ShaderStageType Stage = ShaderStageType::Vertex;
			VkShaderModule Module = VK_NULL_HANDLE;
		};

		bool m_Valid = false;

		Ref<const ShaderMetadata> m_Metadata = nullptr;
		std::vector<ShaderStageModule> m_Modules;

		std::unordered_map<std::string_view, uint32_t> m_NameToIndex;
	};
}
