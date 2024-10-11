#include "PCH.h"

#include "ShaderDescriptorBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanComputeShader.h"

namespace Flare
{
	ShaderDescriptorBuffer::~ShaderDescriptorBuffer()
	{
		Reset();
	}

	void ShaderDescriptorBuffer::SetShader(Ref<ComputeShader> shader)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(shader);

		Reset();

		m_Metadata = shader->GetMetadata();
		m_DescriptorPool = shader.As<VulkanComputeShader>()->GetSetPool();

		m_TextureDescriptors.resize(m_Metadata->DescriptorSetUsage[3].PropertyCount);
	}

	void ShaderDescriptorBuffer::SetTexture(size_t propertyIndex, Ref<const Texture> texture)
	{
		TextureDescriptor& descriptor = m_TextureDescriptors[PropertyIndexToTextureDescriptorIndex(propertyIndex)];
		descriptor.Texture = texture;
		descriptor.Sampler = nullptr;

		m_IsDirty = true;
	}

	void ShaderDescriptorBuffer::SetTexture(size_t propertyIndex, Ref<const Texture> texture, Ref<const Sampler> sampler)
	{
		TextureDescriptor& descriptor = m_TextureDescriptors[PropertyIndexToTextureDescriptorIndex(propertyIndex)];
		descriptor.Texture = texture;
		descriptor.Sampler = sampler;

		m_IsDirty = true;
	}

	void ShaderDescriptorBuffer::UpdateDescriptorSet()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_TextureDescriptors.size() == 0)
			return;

		if (!m_IsDirty)
			return;

		// Current descriptor is used in rendering and cannot be updated
		if (m_CurrentSetIsInUse && m_DescriptorSet != nullptr)
		{
			VulkanContext::GetInstance().EnqueueDescriptorRelease(m_DescriptorSet, m_DescriptorPool);
		}

		if (m_CurrentSetIsInUse || m_DescriptorSet == nullptr)
		{
			m_DescriptorSet = m_DescriptorPool->AllocateSet();
		}

		const std::vector<ShaderDescriptorProperty>& descriptorProperties = m_Metadata->DescriptorProperties;
		const ShaderDescriptorSetUsage& setUsage = m_Metadata->DescriptorSetUsage[3]; // TODO: Don't hardcode

		// Texture index 0 maps to setUage.FirstPropertyInSet
		for (size_t textureIndex = 0; textureIndex < m_TextureDescriptors.size(); textureIndex++)
		{
			const TextureDescriptor& textureDescriptor = m_TextureDescriptors[textureIndex];
			const ShaderDescriptorProperty& descriptorProperty = descriptorProperties[setUsage.FirstPropertyInSet + (uint32_t)textureIndex];

			switch (descriptorProperty.Type)
			{
			case ShaderDescriptorType::UniformBuffer:
			case ShaderDescriptorType::StorageBuffer:
				FLARE_CORE_ASSERT(false);
				break;
			case ShaderDescriptorType::StorageImage:
				m_DescriptorSet->WriteStorageImage(textureDescriptor.Texture, descriptorProperty.Binding);
				break;
			case ShaderDescriptorType::SampledImage:
				if (textureDescriptor.Texture)
					m_DescriptorSet->WriteImage(textureDescriptor.Texture, descriptorProperty.Binding);
				else
					m_DescriptorSet->WriteImage(Renderer::GetWhiteTexture(), descriptorProperty.Binding);
				break;
			}
		}

		m_DescriptorSet->FlushWrites();
	}

	void ShaderDescriptorBuffer::Reset()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_DescriptorSet && m_CurrentSetIsInUse)
		{
			VulkanContext::GetInstance().EnqueueDescriptorRelease(m_DescriptorSet, m_DescriptorPool);
		}

		m_DescriptorSet = nullptr;
		m_DescriptorPool = nullptr;

		m_Metadata = nullptr;
		m_TextureDescriptors.clear();

		m_IsDirty = false;
		m_CurrentSetIsInUse = false;
	}

	size_t ShaderDescriptorBuffer::PropertyIndexToTextureDescriptorIndex(size_t propertyIndex) const
	{
		FLARE_CORE_ASSERT(m_Metadata);

		// TODO: Don't hard code set 3
		const ShaderDescriptorSetUsage& setUsage = m_Metadata->DescriptorSetUsage[3];

		size_t rangeStart = (size_t)setUsage.FirstPropertyInSet;
		size_t rangeEnd = (size_t)(setUsage.FirstPropertyInSet + setUsage.PropertyCount);

		FLARE_CORE_ASSERT(propertyIndex >= rangeStart && propertyIndex < rangeEnd);

		return propertyIndex - rangeStart;
	}
}
