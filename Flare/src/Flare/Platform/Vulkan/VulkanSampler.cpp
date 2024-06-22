#include "VulkanSampler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanSampler::VulkanSampler(const SamplerSpecifications& specifications)
		: m_Specifications(specifications)
	{
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		switch (m_Specifications.WrapMode)
		{
		case TextureWrap::Clamp:
			addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		case TextureWrap::Repeat:
			addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.borderColor = {};
		samplerInfo.compareEnable = m_Specifications.ComparisonEnabled;
		samplerInfo.compareOp = DepthComparisonFunctionToVulkanCompareOp(m_Specifications.ComparisonFunction);
		samplerInfo.flags = 0;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.mipLodBias = 0.0f;

		switch (m_Specifications.Filter)
		{
		case TextureFiltering::Closest:
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		case TextureFiltering::Linear:
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		switch (m_Specifications.Filter)
		{
		case TextureFiltering::Closest:
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			break;
		case TextureFiltering::Linear:
			samplerInfo.minFilter = VK_FILTER_LINEAR;
			samplerInfo.magFilter = VK_FILTER_LINEAR;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		VK_CHECK_RESULT(vkCreateSampler(VulkanContext::GetInstance().GetDevice(), &samplerInfo, nullptr, &m_Sampler));
	}
	
	VulkanSampler::~VulkanSampler()
	{
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());
		vkDestroySampler(VulkanContext::GetInstance().GetDevice(), m_Sampler, nullptr);
	}

	const SamplerSpecifications& Flare::VulkanSampler::GetSpecifications() const
	{
		return m_Specifications;
	}
}
