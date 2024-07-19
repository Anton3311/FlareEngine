#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include <string>
#include <string_view>

namespace Flare
{
	class Texture;
	class Sampler;
	class FrameBuffer;
	class UniformBuffer;
	class ShaderStorageBuffer;

	class FLARE_API DescriptorSet
	{
	public:
		virtual ~DescriptorSet();

		virtual void WriteImage(Ref<const Texture> texture, uint32_t binding) = 0;
		virtual void WriteImage(Ref<const Texture> texture, Ref<const Sampler> sampler, uint32_t binding) = 0;
		virtual void WriteImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding) = 0;
		virtual void WriteImages(Span<Ref<const Texture>> textures, uint32_t arrayOffset, uint32_t binding) = 0;

		virtual void WriteStorageImage(Ref<const FrameBuffer> frameBuffer, uint32_t attachmentIndex, uint32_t binding) = 0;

		virtual void WriteUniformBuffer(Ref<const UniformBuffer> buffer, uint32_t binding) = 0;
		virtual void WriteStorageBuffer(Ref<const ShaderStorageBuffer> buffer, uint32_t binding) = 0;

		virtual void FlushWrites() = 0;

		virtual void SetDebugName(std::string_view name) = 0;
		virtual const std::string& GetDebugName() const = 0;
	};

	class FLARE_API DescriptorSetLayout
	{
	public:
		virtual ~DescriptorSetLayout();
	};

	class FLARE_API DescriptorSetPool
	{
	public:
		virtual ~DescriptorSetPool();

		virtual Ref<DescriptorSet> AllocateSet() = 0;
		virtual void ReleaseSet(Ref<DescriptorSet> set) = 0;

		virtual Ref<const DescriptorSetLayout> GetLayout() const = 0;
	};
}
