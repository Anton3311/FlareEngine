#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	enum class BorderColor
	{
		FloatTransparentBlack,
		IntTransparentBlack,
		FloatOpaqueBlack,
		IntOpaqueBlack,
		FloatOpaqueWhite,
		IntOPaqueWhite,
	};

	struct SamplerSpecifications
	{
		TextureWrap WrapMode;
		TextureFiltering Filter;
		BorderColor BorderColor;

		bool ComparisonEnabled = false;
		DepthComparisonFunction ComparisonFunction = DepthComparisonFunction::Never;
	};

	class Sampler : public RefCounted<Sampler>
	{
	public:
		virtual ~Sampler() = default;

		virtual const SamplerSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<Sampler> Create(const SamplerSpecifications& specifications);
	};
}
