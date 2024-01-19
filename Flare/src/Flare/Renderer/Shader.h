#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/ShaderMetadata.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <optional>

namespace Flare
{
	using ShaderProperties = std::vector<ShaderProperty>;
	// Indices of frame buffer attachments to which the shader writes
	using ShaderOutputs = std::vector<uint32_t>;

	class FLARE_API Shader : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		Shader()
			: Asset(AssetType::Shader) {}

		virtual ~Shader() = default;

		virtual void Load() = 0;
		virtual bool IsLoaded() = 0;

		virtual void Bind() = 0;

		virtual const ShaderProperties& GetProperties() const = 0;
		virtual const ShaderOutputs& GetOutputs() const = 0;
		virtual ShaderFeatures GetFeatures() const = 0;
		virtual std::optional<uint32_t> GetPropertyIndex(std::string_view name) const = 0;

		virtual void SetInt(const std::string& name, int value) = 0;
		virtual void SetFloat(const std::string& name, float value) = 0;
		virtual void SetFloat2(const std::string& name, glm::vec2 value) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
		virtual void SetIntArray(const std::string& name, const int* values, uint32_t count) = 0;
		virtual void SetMatrix4(const std::string& name, const glm::mat4& matrix) = 0;
	public:
		static Ref<Shader> Create();
	};
}