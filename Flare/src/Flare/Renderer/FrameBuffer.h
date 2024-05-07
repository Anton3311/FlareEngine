#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/Texture.h"

#include <glm/glm.hpp>

namespace Flare
{
	enum class FrameBufferTextureFormat
	{
		RGB8,
		RGBA8,
		R11G11B10,

		RedInteger,
		RF32,

		Depth24Stencil8,

		Depth = Depth24Stencil8
	};

	constexpr bool IsDepthFormat(FrameBufferTextureFormat format)
	{
		switch (format)
		{
		case FrameBufferTextureFormat::RGB8:
		case FrameBufferTextureFormat::RGBA8:
		case FrameBufferTextureFormat::R11G11B10:
		case FrameBufferTextureFormat::RedInteger:
			return false;
		case FrameBufferTextureFormat::Depth24Stencil8:
			return true;
		}

		return false;
	}

	constexpr bool HasStencilCompomnent(FrameBufferTextureFormat format)
	{
		switch (format)
		{
		case FrameBufferTextureFormat::Depth24Stencil8:
			return true;
		}

		return false;
	}

	struct FrameBufferAttachmentSpecifications
	{
		FrameBufferTextureFormat Format;
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

		virtual void ClearAttachment(uint32_t index, const void* value) = 0;
		virtual void ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput) = 0;
		virtual void BindAttachmentTexture(uint32_t attachment, uint32_t slot = 0) = 0;

		virtual const FrameBufferSpecifications& GetSpecifications() const = 0;

		inline glm::uvec2 GetSize() const
		{
			const auto& specifications = GetSpecifications();
			return glm::uvec2(specifications.Width, specifications.Height);
		}
	public:
		static Ref<FrameBuffer> Create(const FrameBufferSpecifications& specifications);
	};
}