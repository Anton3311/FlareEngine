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
		virtual bool IsLoaded() const override;
		virtual void Bind() override;

		virtual Ref<const ShaderMetadata> GetMetadata() const override;
		virtual const ShaderProperties& GetProperties() const override;
		virtual const ShaderOutputs& GetOutputs() const override;
		virtual ShaderFeatures GetFeatures() const override;
		virtual std::optional<uint32_t> GetPropertyIndex(std::string_view name) const override;

		inline const std::vector<int32_t> GetUniformLocations() const { return m_UniformLocations; }
		inline uint32_t GetId() const { return m_Id; }
	private:
		uint32_t m_Id;
		bool m_IsValid;

		Ref<const ShaderMetadata> m_Metadata;

		std::vector<int32_t> m_UniformLocations;
		std::unordered_map<std::string_view, uint32_t> m_NameToIndex;
	};
}