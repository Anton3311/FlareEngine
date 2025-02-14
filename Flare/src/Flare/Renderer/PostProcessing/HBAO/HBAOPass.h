#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/Renderer/ShaderConstantBuffer.h"
#include "Flare/Renderer/ShaderDescriptorBuffer.h"

namespace Flare
{
	class ComputeShader;
	class Material;
	class SSAO;

	class HBAOPass : public RenderGraphPass
	{
	public:
		HBAOPass(Ref<SSAO> parameters,
			RenderGraphTextureId linearDepth,
			RenderGraphTextureId outputTexture,
			uint32_t subPassIndex,
			float jitterAngle);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		uint32_t m_SubPassIndex = 0;
		float m_JitterAngle;

		Ref<ComputeShader> m_Shader = nullptr;
		RenderGraphTextureId m_LinearDepth;
		RenderGraphTextureId m_OutputTexture;

		ShaderConstantBuffer m_ConstantBuffer;
		ShaderDescriptorBuffer m_DescriptorBuffer;

		Ref<SSAO> m_Parameters;
	};

	class HBAOCombineDeinterleavedTexturesPass : public RenderGraphPass
	{
	public:
		using TextureIdsArray = std::array<RenderGraphTextureId, 4>;

		HBAOCombineDeinterleavedTexturesPass(const TextureIdsArray& textures);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		TextureIdsArray m_Textures;
		Ref<Material> m_Material = nullptr;
	};
}
