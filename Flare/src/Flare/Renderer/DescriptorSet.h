#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include <string>
#include <string_view>

namespace Flare
{
	class GPUBuffer;
	class Sampler;
	class Texture;

	class FLARE_API DescriptorSet : public RefCounted<DescriptorSet>
	{
	public:
		virtual ~DescriptorSet();

		virtual void WriteImage(Ref<const Texture> texture, uint32_t binding) = 0;
		virtual void WriteImage(Ref<const Texture> texture, Ref<const Sampler> sampler, uint32_t binding) = 0;
		virtual void WriteImages(Span<Ref<const Texture>> textures, uint32_t arrayOffset, uint32_t binding) = 0;

		virtual void WriteStorageImage(Ref<const Texture> texture, uint32_t binding) = 0;

		virtual void WriteUniformBuffer(Ref<const GPUBuffer> buffer, uint32_t binding) = 0;
		virtual void WriteStorageBuffer(Ref<const GPUBuffer> buffer, uint32_t binding) = 0;

		virtual void FlushWrites() = 0;

		virtual void SetDebugName(std::string_view name) = 0;
		virtual const std::string& GetDebugName() const = 0;
	};

	class FLARE_API DescriptorSetLayout : public RefCounted<DescriptorSetLayout>
	{
	public:
		virtual ~DescriptorSetLayout();
	};

	class FLARE_API DescriptorSetPool : public RefCounted<DescriptorSetPool>
	{
	public:
		virtual ~DescriptorSetPool();

		virtual Ref<DescriptorSet> AllocateSet() = 0;
		virtual void ReleaseSet(Ref<DescriptorSet> set) = 0;

		virtual Ref<const DescriptorSetLayout> GetLayout() const = 0;
	};
}
