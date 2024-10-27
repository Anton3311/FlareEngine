#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

#include <glm/glm.hpp>
#include <stdint.h>

namespace Flare
{
	class Texture;
	class CommandBuffer;

	enum class ResourceAccess
	{
		None = 0,
		Read = 1,
		Write = 2,

		ReadWrite = Read | Write,
	};

	FLARE_IMPL_ENUM_BITFIELD(ResourceAccess);

	enum class RenderGraphPassType
	{
		Graphics,
		Compute,
		Other,
	};

	enum class AttachmentClearValueType : uint8_t
	{
		Color = 0,
		Depth = 1,
	};

	struct AttachmentClearValue
	{
		AttachmentClearValue() = default;
		AttachmentClearValue(const glm::vec4& clearColor)
			: Type(AttachmentClearValueType::Color), Color(clearColor) {}
		AttachmentClearValue(float clearDepth)
			: Type(AttachmentClearValueType::Depth), Depth(clearDepth) {}

		AttachmentClearValueType Type = AttachmentClearValueType::Color;
		union
		{
			glm::vec4 Color = glm::vec4(0.0f);
			float Depth;
		};
	};

	class FLARE_API RenderGraphPassSpecifications
	{
	public:
		struct Input
		{
			RenderGraphTextureId InputTexture;
			ImageLayout Layout = ImageLayout::Undefined;
		};

		struct GeneralTextureResource
		{
			ResourceAccess Access = ResourceAccess::None;
			RenderGraphTextureId TextureId;
		};

		struct OutputAttachment
		{
			RenderGraphTextureId AttachmentTexture;
			ImageLayout Layout = ImageLayout::Undefined;

			TextureSubresource Subresource = TextureSubresource::FULL_VIEW;

			std::optional<AttachmentClearValue> ClearValue;
		};

		void SetType(RenderGraphPassType type) { m_Type = type; }

		void SetDebugName(std::string_view debugName);
		void AddInput(RenderGraphTextureId textureId, ImageLayout layout = ImageLayout::ReadOnly);
		void AddOutput(RenderGraphTextureId textureId, ImageLayout layout = ImageLayout::AttachmentOutput);
		
		void AddResource(RenderGraphTextureId textureId, ResourceAccess access);
		void AddOutput(RenderGraphTextureId textureId, const glm::vec4& clearColor, ImageLayout layout = ImageLayout::AttachmentOutput);
		void AddOutput(RenderGraphTextureId textureId, float depthClearValue, ImageLayout layout = ImageLayout::AttachmentOutput);

		void AddSubresourceOutput(RenderGraphTextureId textureId, std::optional<glm::vec4> clearValue, const TextureSubresource& subresource);
		void AddDepthSubresourceOutput(RenderGraphTextureId textureId, std::optional<float> clearValue, const TextureSubresource& subresource);

		inline const std::vector<Input>& GetInputs() const { return m_Inputs; };
		inline const std::vector<OutputAttachment>& GetOutputs() const { return m_Outputs; }
		inline const std::vector<GeneralTextureResource>& GetGeneralTextureResources() const { return m_GeneralTextureResources; }

		inline const std::string& GetDebugName() const { return m_DebugName; }
		inline glm::vec4 GetDebugColor() const { return m_DebugColor; }
		inline RenderGraphPassType GetType() const { return m_Type; }

		inline bool HasOutputClearValues() const { return m_HasOutputClearValues; }
	private:
		RenderGraphPassType m_Type = RenderGraphPassType::Graphics;
		std::string m_DebugName;
		glm::vec4 m_DebugColor = glm::vec4(1.0f);
		std::vector<Input> m_Inputs;
		std::vector<OutputAttachment> m_Outputs;
		std::vector<GeneralTextureResource> m_GeneralTextureResources;

		bool m_HasOutputClearValues = false;
	};

}
