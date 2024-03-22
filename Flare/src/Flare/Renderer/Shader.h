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
	class FLARE_API Shader : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		Shader()
			: Asset(AssetType::Shader) {}

		virtual ~Shader() = default;

		virtual void Load() = 0;
		virtual bool IsLoaded() const = 0;

		virtual void Bind() = 0;

		virtual const ShaderProperties& GetProperties() const = 0;
		virtual const ShaderOutputs& GetOutputs() const = 0;
		virtual ShaderFeatures GetFeatures() const = 0;
		virtual std::optional<uint32_t> GetPropertyIndex(std::string_view name) const = 0;
	public:
		static Ref<Shader> Create();
	};
}