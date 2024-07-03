#pragma once

#include "FlareCore/Collections/Span.h"
#include "Flare/Renderer/DescriptorSet.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace Flare
{
	class VulkanDescriptorSetPool;

	class FLARE_API VulkanDescriptorSetLayout : public DescriptorSetLayout
	{
	public:
		VulkanDescriptorSetLayout(const Span<VkDescriptorSetLayoutBinding>& bindings);
		~VulkanDescriptorSetLayout();

		inline VkDescriptorSetLayout GetHandle() const { return m_Layout; };
		inline uint32_t GetImageBindingsCount() const { return m_ImageBindings; }
		inline uint32_t GetBufferBindingsCount() const { return m_BufferBindings; }
	private:
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		uint32_t m_ImageBindings = 0;
		uint32_t m_BufferBindings = 0;
	};

	class FLARE_API VulkanDescriptorSet : public DescriptorSet
	{
	public:
		VulkanDescriptorSet(VulkanDescriptorSetPool* pool, VkDescriptorSet set);
		~VulkanDescriptorSet();

		void WriteImage(Ref<const Texture> texture, uint32_t binding) override;
		void WriteImage(Ref<const Texture> texture, Ref<const Sampler> sampler, uint32_t binding) override;
		void WriteImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding) override;
		void WriteImages(Span<Ref<const Texture>> textures, uint32_t arrayOffset, uint32_t binding) override;

		void WriteStorageImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding) override;

		void WriteUniformBuffer(Ref<const UniformBuffer> buffer, uint32_t binding) override;
		void WriteStorageBuffer(Ref<const ShaderStorageBuffer> buffer, uint32_t binding) override;

		void FlushWrites() override;

		void SetDebugName(std::string_view name) override;
		const std::string& GetDebugName() const override;
	public:
		inline VkDescriptorSet GetHandle() const { return m_Set; }
	private:
		std::string m_DebugName;
		VkDescriptorSet m_Set = VK_NULL_HANDLE;

		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorImageInfo> m_Images;
		std::vector<VkDescriptorBufferInfo> m_Buffers;
	};

	class FLARE_API VulkanDescriptorSetPool : public DescriptorSetPool
	{
	public:
		VulkanDescriptorSetPool(size_t maxSets, const Span<VkDescriptorSetLayoutBinding>& bindings);
		~VulkanDescriptorSetPool();

		Ref<DescriptorSet> AllocateSet() override;
		void ReleaseSet(Ref<DescriptorSet> set) override;

		Ref<const DescriptorSetLayout> GetLayout() const override;
	private:
		Ref<VulkanDescriptorSetLayout> m_Layout = nullptr;
		VkDescriptorPool m_Pool = VK_NULL_HANDLE;

		size_t m_MaxSets = 0;
		size_t m_AllocatedSets = 0;
	};
}
