#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/ComputeShader.h"

namespace Flare
{
	struct ComputePipelineSpecifications
	{
		Ref<ComputeShader> Shader;
	};

	class ComputePipeline
	{
	public:
		virtual const ComputePipelineSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<ComputePipeline> Create(const ComputePipelineSpecifications& specifications);
	};
}
