#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	class ComputeShader;
	class DescriptorSet;
	class DescriptorSetPool;
	class Sampler;
	class Texture;
	class FLARE_API ShaderDescriptorBuffer
	{
	public:
		struct TextureDescriptor
		{
			Ref<const Texture> Texture = nullptr;
			Ref<const Sampler> Sampler = nullptr;
		};

		~ShaderDescriptorBuffer();

		void SetShader(Ref<ComputeShader> shader);

		void SetTexture(size_t propertyIndex, Ref<const Texture> texture);
		void SetTexture(size_t propertyIndex, Ref<const Texture> texture, Ref<const Sampler> sampler);

		inline void MarkAsUsedInRendering() { m_CurrentSetIsInUse = true; }
		inline bool IsDirty() const { return m_IsDirty; }

		inline Ref<DescriptorSet> GetDescriptorSet() const { return m_DescriptorSet; }

		void UpdateDescriptorSet();
		void Reset();
	private:
		size_t PropertyIndexToTextureDescriptorIndex(size_t propertyIndex) const;
	private:
		bool m_CurrentSetIsInUse = false;
		bool m_IsDirty = false;

		Ref<const ShaderMetadata> m_Metadata = nullptr;

		Ref<DescriptorSet> m_DescriptorSet = nullptr;
		Ref<DescriptorSetPool> m_DescriptorPool = nullptr;

		std::vector<TextureDescriptor> m_TextureDescriptors;
	};
}
