#pragma once

#include "Flare/Renderer/Shader.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

namespace Flare
{
	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader();
		~OpenGLShader();
	public:
		virtual void Load() override;
		virtual bool IsLoaded() override;
		virtual void Bind() override;

		virtual const ShaderProperties& GetProperties() const override;
		virtual const ShaderOutputs& GetOutputs() const override;
		virtual ShaderFeatures GetFeatures() const override;
		virtual std::optional<uint32_t> GetPropertyIndex(std::string_view name) const override;
		
		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetFloat2(const std::string& name, glm::vec2 value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetIntArray(const std::string& name, const int* values, uint32_t count) override;
		virtual void SetMatrix4(const std::string& name, const glm::mat4& matrix) override;

		void SetInt(uint32_t index, int32_t value);
		void SetInt2(uint32_t index, glm::ivec2 value);
		void SetInt3(uint32_t index, const glm::ivec3& value);
		void SetInt4(uint32_t index, const glm::ivec4& value);

		void SetFloat(uint32_t index, float value);
		void SetFloat2(uint32_t index, glm::vec2 value);
		void SetFloat3(uint32_t index, const glm::vec3& value);
		void SetFloat4(uint32_t index, const glm::vec4& value);
		void SetIntArray(uint32_t index, const int* values, uint32_t count);
		void SetMatrix4(uint32_t index, const glm::mat4& matrix);
	private:
		uint32_t m_Id;
		bool m_IsValid;
		ShaderProperties m_Properties;
		ShaderOutputs m_Outputs;
		ShaderFeatures m_Features;

		std::vector<int32_t> m_UniformLocations;
		std::unordered_map<std::string_view, uint32_t> m_NameToIndex;
	};
}