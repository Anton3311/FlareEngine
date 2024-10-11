#include "PCH.h"

#include "RenderGraphPassSpecifications.h"

namespace Flare
{
	void RenderGraphPassSpecifications::SetDebugName(std::string_view debugName)
	{
		m_DebugName = debugName;

		size_t hash = std::hash<std::string>()(m_DebugName);
		uint8_t r = hash & 0xff;
		uint8_t g = (hash >> 8) & 0xff;
		uint8_t b = (hash >> 16) & 0xff;

		m_DebugColor.r = glm::clamp((float)r / 255.0f, 0.0f, 1.0f);
		m_DebugColor.g = glm::clamp((float)g / 255.0f, 0.0f, 1.0f);
		m_DebugColor.b = glm::clamp((float)b / 255.0f, 0.0f, 1.0f);
		m_DebugColor.a = 1.0f;
	}

	void RenderGraphPassSpecifications::AddInput(RenderGraphTextureId textureId, ImageLayout layout)
	{
		Input& input = m_Inputs.emplace_back();
		input.InputTexture = textureId;
		input.Layout = layout;
	}

	void RenderGraphPassSpecifications::AddOutput(RenderGraphTextureId textureId, uint32_t attachmentIndex, ImageLayout layout)
	{
		auto& output = m_Outputs.emplace_back();
		output.AttachmentTexture = textureId;
		output.AttachmentIndex = attachmentIndex;
		output.Layout = layout;
	}

	void RenderGraphPassSpecifications::AddResource(RenderGraphTextureId textureId, ResourceAccess access)
	{
		FLARE_CORE_ASSERT(access != ResourceAccess::None);

		auto& resource = m_GeneralTextureResources.emplace_back();
		resource.Access = access;
		resource.TextureId = textureId;
	}

	void RenderGraphPassSpecifications::AddOutput(RenderGraphTextureId textureId,
		uint32_t attachmentIndex,
		const glm::vec4& clearColor,
		ImageLayout layout)
	{
		auto& output = m_Outputs.emplace_back();
		output.AttachmentTexture = textureId;
		output.AttachmentIndex = attachmentIndex;
		output.Layout = layout;
		output.ClearValue = AttachmentClearValue(clearColor);

		m_HasOutputClearValues = true;
	}

	void RenderGraphPassSpecifications::AddOutput(RenderGraphTextureId textureId,
		uint32_t attachmentIndex,
		float depthClearValue,
		ImageLayout layout)
	{
		auto& output = m_Outputs.emplace_back();
		output.AttachmentTexture = textureId;
		output.AttachmentIndex = attachmentIndex;
		output.Layout = layout;
		output.ClearValue = AttachmentClearValue(depthClearValue);

		m_HasOutputClearValues = true;
	}
}
