#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	struct PipelineSpecifications
	{
		BufferLayout InputLayout;
		Ref<Shader> Shader;
	};

	class Pipeline
	{
	public:
		virtual const PipelineSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<Pipeline> Create(const PipelineSpecifications& specifications);
	};
}
