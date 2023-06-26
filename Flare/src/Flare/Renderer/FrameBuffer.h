#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Flare
{
	enum class FrameBufferTextureFormat
	{
		RGB8,
		RGBA8,
	};

	struct FrameBufferAttachmentSpecifications
	{
		const FrameBufferTextureFormat Format;
		const TextureWrap Wrap;
		const TextureFiltering Filtering;
	};

	struct FrameBufferSpecifications
	{
		FrameBufferSpecifications(const std::initializer_list<FrameBufferAttachmentSpecifications>& attachments)
			: Width(0), Height(0), Attachments(attachments) {}
		FrameBufferSpecifications(uint32_t width, uint32_t height, const std::initializer_list<FrameBufferAttachmentSpecifications>& attachments)
			: Width(width), Height(height), Attachments(attachments) {}

		uint32_t Width;
		uint32_t Height;
		std::vector<FrameBufferAttachmentSpecifications> Attachments;
	};

	class FrameBuffer
	{
	public:
		virtual ~FrameBuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual void* GetColorAttachmentRendererId(uint32_t attachmentIndex) = 0;

		virtual const FrameBufferSpecifications& GetSpecifications() const = 0;
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecifications& specifications);
	};
}