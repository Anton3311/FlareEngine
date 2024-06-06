#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderGraph/RenderGraphContext.h"
#include "Flare/Renderer/Texture.h"

#include <string>
#include <string_view>

namespace Flare
{
	class Texture;
	class CommandBuffer;

	class FLARE_API RenderGraphPassSpecifications
	{
	public:
		struct Input
		{
			Ref<Texture> InputTexture = nullptr;
			ImageLayout Layout = ImageLayout::Undefined;
		};

		struct OutputAttachment
		{
			Ref<Texture> AttachmentTexture = nullptr;
			uint32_t AttachmentIndex = 0;
			ImageLayout Layout = ImageLayout::Undefined;
		};

		void SetDebugName(std::string_view debugName);
		void AddInput(const Ref<Texture>& texture, ImageLayout layout = ImageLayout::ReadOnly);
		void AddOutput(const Ref<Texture>& texture, uint32_t attachmentIndex, ImageLayout layout = ImageLayout::AttachmentOutput);

		inline const std::vector<Input>& GetInputs() const { return m_Inputs; };
		inline const std::vector<OutputAttachment>& GetOutputs() const { return m_Outputs; }
		inline const std::string& GetDebugName() const { return m_DebugName; }
	private:
		std::string m_DebugName;
		std::vector<Input> m_Inputs;
		std::vector<OutputAttachment> m_Outputs;
	};

	class FLARE_API RenderGraphPass
	{
	public:
		virtual ~RenderGraphPass() = default;
		virtual void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) = 0;
	};
}
