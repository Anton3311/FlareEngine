#include "VulkanDescriptorSet.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Flare/Platform/Vulkan/VulkanShaderStorageBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

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



	VulkanDescriptorSet::VulkanDescriptorSet(VkDescriptorPool pool, const Ref<const VulkanDescriptorSetLayout>& layout)
	{
		VkDescriptorSetLayout layoutHandle = layout->GetHandle();

		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = pool;
		info.descriptorSetCount = 1;
		info.pSetLayouts = &layoutHandle;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(VulkanContext::GetInstance().GetDevice(), &info, &m_Set));
	}

	VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSetPool* pool, VkDescriptorSet set)
		: m_Pool(pool), m_Set(set)
	{
		m_Buffers.reserve(pool->GetLayout()->GetBufferBindingsCount());
		m_Images.reserve(pool->GetLayout()->GetImageBindingsCount());
	}

	VulkanDescriptorSet::~VulkanDescriptorSet()
	{
	}

	void VulkanDescriptorSet::WriteUniformBuffer(const Ref<const UniformBuffer>& uniformBuffer, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Buffers.size() < m_Buffers.capacity());
		auto& buffer = m_Buffers.emplace_back();
		buffer.buffer = As<const VulkanUniformBuffer>(uniformBuffer)->GetBufferHandle();
		buffer.offset = 0;
		buffer.range = uniformBuffer->GetSize();

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = &buffer;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteStorageBuffer(const Ref<const ShaderStorageBuffer>& storageBuffer, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Buffers.size() < m_Buffers.capacity());
		auto& buffer = m_Buffers.emplace_back();
		buffer.buffer = As<const VulkanShaderStorageBuffer>(storageBuffer)->GetBufferHandle();
		buffer.offset = 0;
		buffer.range = storageBuffer->GetSize();

		auto& write = m_Writes.emplace_back();
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.dstBinding = binding;
		write.dstArrayElement = 0;
		write.dstSet = m_Set;
		write.pBufferInfo = &buffer;
		write.pImageInfo = nullptr;
		write.pTexelBufferView = nullptr;
	}

	void VulkanDescriptorSet::WriteTexture(const Ref<const Texture>& texture, uint32_t binding)
	{
		WriteTextures(Span<Ref<const Texture>>((Ref<const Texture>)texture), 0, binding);
	}

	void VulkanDescriptorSet::WriteFrameBufferAttachment(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachmentIndex, uint32_t binding)
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

	void VulkanDescriptorSet::WriteStorageImage(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachmentIndex, uint32_t binding)
	{
		FLARE_CORE_ASSERT(m_Images.size() < m_Images.capacity());
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachmentIndex < frameBuffer->GetAttachmentsCount());

		auto vulkanFrameBuffer = As<const VulkanFrameBuffer>(frameBuffer);
		auto& image = m_Images.emplace_back();
		image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		image.imageView = vulkanFrameBuffer->GetAttachmentImageView(attachmentIndex);
		image.sampler = VK_NULL_HANDLE;

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

	void VulkanDescriptorSet::WriteTextures(const Span<Ref<const Texture>>& textures, size_t arrayOffset, uint32_t binding)
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

	void VulkanDescriptorSet::FlushWrites()
	{
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



	VulkanDescriptorSetPool::VulkanDescriptorSetPool(size_t maxSets, const Span<VkDescriptorSetLayoutBinding>& bindings)
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
		vkDestroyDescriptorPool(VulkanContext::GetInstance().GetDevice(), m_Pool, nullptr);
	}

	Ref<VulkanDescriptorSet> VulkanDescriptorSetPool::AllocateSet()
	{
		VkDescriptorSetLayout layout = m_Layout->GetHandle();
		VkDescriptorSetAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		info.descriptorPool = m_Pool;
		info.descriptorSetCount = 1;
		info.pNext = nullptr;
		info.pSetLayouts = &layout;

		VkDescriptorSet set = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(VulkanContext::GetInstance().GetDevice(), &info, &set));
		return CreateRef<VulkanDescriptorSet>(this, set);
	}

	void VulkanDescriptorSetPool::ReleaseSet(const Ref<VulkanDescriptorSet>& set)
	{
		VkDescriptorSet setHandles[] = { set->GetHandle() };
		VK_CHECK_RESULT(vkFreeDescriptorSets(VulkanContext::GetInstance().GetDevice(), m_Pool, 1, setHandles));
	}
}
