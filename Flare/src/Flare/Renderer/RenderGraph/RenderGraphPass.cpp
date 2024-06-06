#include "RenderGraphPass.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	void RenderGraphPassSpecifications::SetDebugName(std::string_view debugName)
	{
		m_DebugName = debugName;
	}

	void RenderGraphPassSpecifications::AddInput(const Ref<Texture>& texture, ImageLayout layout)
	{
		FLARE_CORE_ASSERT(texture != nullptr);
		Input& input = m_Inputs.emplace_back();
		input.InputTexture = texture;
		input.Layout = layout;
	}

	void RenderGraphPassSpecifications::AddOutput(const Ref<Texture>& texture, uint32_t attachmentIndex, ImageLayout layout)
	{
		FLARE_CORE_ASSERT(texture != nullptr);
		auto& output = m_Outputs.emplace_back();
		output.AttachmentTexture = texture;
		output.AttachmentIndex = attachmentIndex;
		output.Layout = layout;
	}
}
