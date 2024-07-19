#include "RenderGraphPass.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	void RenderGraphPassSpecifications::SetDebugName(std::string_view debugName)
	{
		m_DebugName = debugName;
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
