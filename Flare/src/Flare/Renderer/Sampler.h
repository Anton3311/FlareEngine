#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	struct SamplerSpecifications
	{
		TextureWrap WrapMode;
		TextureFiltering Filter;

		bool ComparisonEnabled = false;
		DepthComparisonFunction ComparisonFunction = DepthComparisonFunction::Never;
	};

	class Sampler
	{
	public:
		virtual ~Sampler() = default;

		virtual const SamplerSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<Sampler> Create(const SamplerSpecifications& specifications);
	};
}
