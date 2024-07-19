#pragma once

#include "FlareCore/Core.h"
#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	struct ComputeShaderMetadata
	{
		std::string Name;
		glm::uvec3 LocalGroupSize = glm::uvec3(1u);
		ShaderPushConstantsRange PushConstantsRange;
		std::vector<ShaderProperty> Properties;
	};

	class FLARE_API ComputeShader : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		ComputeShader();
		virtual ~ComputeShader();

		virtual Ref<const ComputeShaderMetadata> GetMetadata() const = 0;
		virtual void Load() = 0;
		virtual bool IsLoaded() const = 0;
	public:
		static Ref<ComputeShader> Create();
	};
}
