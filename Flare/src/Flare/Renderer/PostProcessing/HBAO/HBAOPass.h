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
			const RenderGraphTextureId* outputTextures,
			const float* jitterAngle);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		float m_JitterAngles[4];

		Ref<ComputeShader> m_Shader = nullptr;
		RenderGraphTextureId m_LinearDepth;
		RenderGraphTextureId m_OutputTextures[4];

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
