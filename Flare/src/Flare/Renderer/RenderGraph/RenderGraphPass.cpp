#include "RenderGraphPass.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	void RenderGraphPassSpecifications::AddInput(const Ref<Texture>& texture)
	{
		FLARE_CORE_ASSERT(texture != nullptr);
		m_Inputs.push_back(texture);
	}

	void RenderGraphPassSpecifications::AddOutput(const Ref<Texture>& texture, uint32_t attachmentIndex)
	{
		FLARE_CORE_ASSERT(texture != nullptr);
		auto& output = m_Outputs.emplace_back();
		output.AttachmentTexture = texture;
		output.AttachmentIndex = attachmentIndex;
	}
}
