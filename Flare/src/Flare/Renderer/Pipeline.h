#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	struct PipelineInputLayoutElement
	{
	public:
		PipelineInputLayoutElement(uint32_t binding, uint32_t location, ShaderDataType type)
			: Binding(binding), Location(location), Type(type) {}

	public:
		uint32_t Binding = 0;
		uint32_t Location = 0;
		ShaderDataType Type = ShaderDataType::Float;

		size_t Offset = 0;
	};

	struct PipelineInputLayout
	{
	public:
		PipelineInputLayout() {}
		PipelineInputLayout(const std::initializer_list<PipelineInputLayoutElement>& elements)
			: Elements(elements)
		{
			for (auto& element : Elements)
			{
				element.Offset = Stride;
				Stride += ShaderDataTypeSize(element.Type);
			}
		}
	public:
		size_t Stride = 0;
		std::vector<PipelineInputLayoutElement> Elements;
	};

	struct PipelineSpecifications
	{
		PipelineInputLayout InputLayout;
		Ref<Shader> Shader;

		CullingMode Culling = CullingMode::Back;
		BlendMode Blending = BlendMode::Opaque;
		bool DepthTest = true;
		bool DepthWrite = true;
		DepthComparisonFunction DepthFunction = DepthComparisonFunction::Less;
	};

	class Pipeline
	{
	public:
		virtual const PipelineSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<Pipeline> Create(const PipelineSpecifications& specifications);
	};
}
