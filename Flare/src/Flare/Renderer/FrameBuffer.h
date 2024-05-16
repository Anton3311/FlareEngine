#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Flare
{
	struct FrameBufferAttachmentSpecifications
	{
		TextureFormat Format;
		TextureWrap Wrap;
		TextureFiltering Filtering;
	};

	struct FrameBufferSpecifications
	{
		FrameBufferSpecifications()
			: Width(0), Height(0) {}

		FrameBufferSpecifications(const std::initializer_list<FrameBufferAttachmentSpecifications>& attachments)
			: Width(0), Height(0), Attachments(attachments) {}
		FrameBufferSpecifications(uint32_t width, uint32_t height, const std::initializer_list<FrameBufferAttachmentSpecifications>& attachments)
			: Width(width), Height(height), Attachments(attachments) {}

		uint32_t Width;
		uint32_t Height;
		std::vector<FrameBufferAttachmentSpecifications> Attachments;
	};

	class FLARE_API FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual uint32_t GetAttachmentsCount() const = 0;
		virtual uint32_t GetColorAttachmentsCount() const = 0;
		virtual std::optional<uint32_t> GetDepthAttachmentIndex() const = 0;

		virtual Ref<Texture> GetAttachment(uint32_t index) const = 0;
		virtual const FrameBufferSpecifications& GetSpecifications() const = 0;

		inline glm::uvec2 GetSize() const
		{
			const auto& specifications = GetSpecifications();
			return glm::uvec2(specifications.Width, specifications.Height);
		}
	public:
		static Ref<FrameBuffer> Create(Span<Ref<Texture>> attachmentTextures);
		static Ref<FrameBuffer> Create(const FrameBufferSpecifications& specifications);
	};
}