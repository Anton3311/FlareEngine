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
		VulkanDescriptorSet(VulkanDescriptorSetPool* pool, VkDescriptorPool subPool, VkDescriptorSet set);
		~VulkanDescriptorSet();

		void WriteImage(Ref<const Texture> texture, uint32_t binding) override;
		void WriteImage(Ref<const Texture> texture, Ref<const Sampler> sampler, uint32_t binding) override;
		void WriteImages(Span<Ref<const Texture>> textures, uint32_t arrayOffset, uint32_t binding) override;

		void WriteStorageImage(Ref<const Texture> texture, uint32_t binding) override;

		void WriteUniformBuffer(Ref<const GPUBuffer> buffer, uint32_t binding) override;
		void WriteStorageBuffer(Ref<const GPUBuffer> buffer, uint32_t binding) override;

		void FlushWrites() override;

		void SetDebugName(std::string_view name) override;
		const std::string& GetDebugName() const override;

		inline const VulkanDescriptorSetPool* GetOwnerPool() const { return m_OwnerPool; }
		inline const VkDescriptorPool GetSubPoolHandle() const { return m_SubPoolHandle; }
		inline VkDescriptorSet GetHandle() const { return m_Set; }
	private:
		void ResetAllocation();
	private:
		std::string m_DebugName;

		VkDescriptorSet m_Set = VK_NULL_HANDLE;
		VulkanDescriptorSetPool* m_OwnerPool = nullptr;
		VkDescriptorPool m_SubPoolHandle = nullptr;

		std::vector<VkWriteDescriptorSet> m_Writes;
		std::vector<VkDescriptorImageInfo> m_Images;
		std::vector<VkDescriptorBufferInfo> m_Buffers;

		friend class VulkanDescriptorSetPool;
	};

	class FLARE_API VulkanDescriptorSetPool : public DescriptorSetPool
	{
	public:
		struct PoolEntry
		{
			VkDescriptorPool Pool = VK_NULL_HANDLE;
			size_t MaxSets = 0;
			size_t AllocatedSets = 0;
		};

		VulkanDescriptorSetPool(const Span<VkDescriptorSetLayoutBinding>& bindings);
		~VulkanDescriptorSetPool();

		Ref<DescriptorSet> AllocateSet() override;
		void ReleaseSet(Ref<DescriptorSet> set) override;

		Ref<DescriptorSet> AllocateSet(Ref<const DescriptorSetLayout> layout);

		Ref<const DescriptorSetLayout> GetLayout() const override;
	private:
		void ReleaseSubPool(PoolEntry& entry);
		void AllocateSubPool();
	private:
		Ref<VulkanDescriptorSetLayout> m_Layout = nullptr;

		std::vector<VkDescriptorPoolSize> m_PoolSizes;
		std::vector<PoolEntry> m_SubPools;
	};
}
