#include "VulkanDescriptorSet.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Flare/Platform/Vulkan/VulkanShaderStorageBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"
#include "Flare/Platform/Vulkan/VulkanSampler.h"

namespace Flare
{
	VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(const Span<VkDescriptorSetLayoutBinding>& bindings)
	{
		for (const auto& binding : bindings)
		{
			switch (binding.descriptorType)
			{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				m_ImageBindings++;
				break;
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				m_BufferBindings++;
				break;
			}
		}

		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.flags = 0;
		info.bindingCount = (uint32_t)bindings.GetSize();
		info.pBindings = bindings.GetData();

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_Layout));
	}

	VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout(VulkanContext::GetInstance().GetDevice(), m_Layout, nullptr);
	}



	VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSetPool* pool, VkDescriptorSet set)
		: m_Set(set)
	{
		Ref<const VulkanDescriptorSetLayout> layout = As<const VulkanDescriptorSetLayout>(pool->GetLayout());
		m_Buffers.reserve(layout->GetBufferBindingsCount());
		m_Images.reserve(layout->GetImageBindingsCount());
	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
	}

	void VulkanDescriptorSet::WriteImage(Ref<const Texture> texture, uint32_t binding)
	{
		WriteImages(Span(&texture, 1), 0, binding);
	}

	void VulkanDescriptorSet::WriteImage(Ref<const Texture> texture, Ref<const Sampler> sampler, uint32_t binding)
	{
		FLARE_CORE_ASSERT(texture && sampler);

		auto vulkanTexture = As<const VulkanTexture>(texture);
		auto& image = m_Images.emplace_back();
		image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image.imageView = vulkanTexture->GetImageViewHandle();
		image.sampler = As<const VulkanSampler>(sampler)->GetHandle();

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &image;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Images.size() < m_Images.capacity());
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachmentIndex < frameBuffer->GetAttachmentsCount());

		auto vulkanFrameBuffer = As<const VulkanFrameBuffer>(frameBuffer);
		auto& image = m_Images.emplace_back();
		image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image.imageView = vulkanFrameBuffer->GetAttachmentImageView(attachmentIndex);
		image.sampler = vulkanFrameBuffer->GetDefaultAttachmentSampler(attachmentIndex);

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &image;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteImages(Span<Ref<const Texture>> textures, uint32_t arrayOffset, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Images.size() < m_Images.capacity());
		size_t firstBindingIndex = m_Images.size();
		for (size_t i = 0; i < textures.GetSize(); i++)
		{
			auto vulkanTexture = As<const VulkanTexture>(textures[i]);
			auto& image = m_Images.emplace_back();
			image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image.imageView = vulkanTexture->GetImageViewHandle();
			image.sampler = vulkanTexture->GetDefaultSampler();

			FLARE_CORE_ASSERT(image.imageView);
			FLARE_CORE_ASSERT(image.sampler);
		}

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = (uint32_t)textures.GetSize();
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = binding;
		write.dstArrayElement = (uint32_t)arrayOffset;
		write.dstSet = m_Set;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &m_Images[firstBindingIndex];
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteStorageImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Images.size() < m_Images.capacity());
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachmentIndex < frameBuffer->GetAttachmentsCount());

		auto vulkanFrameBuffer = As<const VulkanFrameBuffer>(frameBuffer);
		auto& image = m_Images.emplace_back();
		image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		image.imageView = vulkanFrameBuffer->GetAttachmentImageView(attachmentIndex);
		image.sampler = vulkanFrameBuffer->GetDefaultAttachmentSampler(attachmentIndex);

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = nullptr;
		write.pImageInfo = &image;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteUniformBuffer(Ref<const UniformBuffer> buffer, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Buffers.size() < m_Buffers.capacity());
		auto& bufferWrite = m_Buffers.emplace_back();
		bufferWrite.buffer = As<const VulkanUniformBuffer>(buffer)->GetBufferHandle();
		bufferWrite.offset = 0;
		bufferWrite.range = buffer->GetSize();

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = &bufferWrite;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteStorageBuffer(Ref<const ShaderStorageBuffer> buffer, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Buffers.size() < m_Buffers.capacity());
		auto& bufferWrite = m_Buffers.emplace_back();
		bufferWrite.buffer = As<const VulkanShaderStorageBuffer>(buffer)->GetBufferHandle();
		bufferWrite.offset = 0;
		bufferWrite.range = buffer->GetSize();

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = &bufferWrite;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::FlushWrites()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Writes.size() == 0)
			return;

		vkUpdateDescriptorSets(VulkanContext::GetInstance().GetDevice(), (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);
		m_Writes.clear();
		m_Images.clear();
		m_Buffers.clear();
	}

	void VulkanDescriptorSet::SetDebugName(std::string_view name)
	{
		m_DebugName = name;
		VK_CHECK_RESULT(VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)m_Set, m_DebugName.c_str()));
	}

	const std::string& VulkanDescriptorSet::GetDebugName() const
	{
		return m_DebugName;
	}



	VulkanDescriptorSetPool::VulkanDescriptorSetPool(size_t maxSets, const Span<VkDescriptorSetLayoutBinding>& bindings)
		: m_MaxSets(maxSets)
	{
		std::vector<VkDescriptorPoolSize> sizes(bindings.GetSize());
		for (size_t i = 0; i < bindings.GetSize(); i++)
		{
			sizes[i].descriptorCount = bindings[i].descriptorCount;
			sizes[i].type = bindings[i].descriptorType;
		}

		VkDescriptorPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		info.maxSets = (uint32_t)maxSets;
		info.poolSizeCount = (uint32_t)sizes.size();
		info.pPoolSizes = sizes.data();
		info.pNext = nullptr;
		info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_Pool));

		m_Layout = CreateRef<VulkanDescriptorSetLayout>(bindings);
	}

	VulkanDescriptorSetPool::~VulkanDescriptorSetPool()
	{
		FLARE_CORE_ASSERT(m_AllocatedSets == 0);
		vkDestroyDescriptorPool(VulkanContext::GetInstance().GetDevice(), m_Pool, nullptr);
	}

	Ref<DescriptorSet> VulkanDescriptorSetPool::AllocateSet()
	{
		return AllocateSet(m_Layout);
	}

	void VulkanDescriptorSetPool::ReleaseSet(Ref<DescriptorSet> set)
	{
		FLARE_CORE_ASSERT(m_AllocatedSets > 0);

		VkDescriptorSet setHandles[] = { As<VulkanDescriptorSet>(set)->GetHandle() };
		VK_CHECK_RESULT(vkFreeDescriptorSets(VulkanContext::GetInstance().GetDevice(), m_Pool, 1, setHandles));

		m_AllocatedSets--;
	}

	Ref<DescriptorSet> VulkanDescriptorSetPool::AllocateSet(Ref<const DescriptorSetLayout> layout)
	{
		FLARE_CORE_ASSERT(m_AllocatedSets < m_MaxSets, "Out of descriptor pool memory");

		VkDescriptorSetLayout layoutHandle = As<const VulkanDescriptorSetLayout>(layout)->GetHandle();
		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = m_Pool;
		info.descriptorSetCount = 1;
		info.pNext = nullptr;
		info.pSetLayouts = &layoutHandle;

		m_AllocatedSets++;

		VkDescriptorSet set = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(VulkanContext::GetInstance().GetDevice(), &info, &set));
		return CreateRef<VulkanDescriptorSet>(this, set);
	}

	Ref<const DescriptorSetLayout> VulkanDescriptorSetPool::GetLayout() const
	{
		return m_Layout;
	}
}
