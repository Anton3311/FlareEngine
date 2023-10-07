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
		OpenGLShader(const std::filesystem::path& path);
		~OpenGLShader();
	public:
		virtual void Bind() override;

		virtual const ShaderParameters& GetParameters() const override;
		virtual std::optional<uint32_t> GetParameterIndex(std::string_view name) const override;
		
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
		struct ShaderProgram
		{
			std::string Source;
			uint32_t Type;

			ShaderProgram(std::string_view source, uint32_t type)
				: Source(source), Type(type) {}
		};

		std::vector<ShaderProgram> PreProcess(std::string_view source);
		void Compile(const std::filesystem::path& path, std::string_view source);
	private:
		uint32_t m_Id;
		ShaderParameters m_Parameters;
		std::vector<int32_t> m_UniformLocations;
		std::unordered_map<std::string_view, uint32_t> m_NameToIndex;
	};
}