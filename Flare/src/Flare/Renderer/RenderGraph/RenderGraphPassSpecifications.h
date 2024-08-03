#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

#include <glm/glm.hpp>
#include <stdint.h>

namespace Flare
{
	class Texture;
	class CommandBuffer;

	enum class RenderGraphPassType
	{
		Graphics,
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

		struct OutputAttachment
		{
			RenderGraphTextureId AttachmentTexture;
			uint32_t AttachmentIndex = 0;
			ImageLayout Layout = ImageLayout::Undefined;
			std::optional<AttachmentClearValue> ClearValue;
		};

		void SetType(RenderGraphPassType type) { m_Type = type; }

		void SetDebugName(std::string_view debugName);
		void AddInput(RenderGraphTextureId textureId, ImageLayout layout = ImageLayout::ReadOnly);
		void AddOutput(RenderGraphTextureId textureId, uint32_t attachmentIndex, ImageLayout layout = ImageLayout::AttachmentOutput);

		void AddOutput(RenderGraphTextureId textureId,
			uint32_t attachmentIndex,
			const glm::vec4& clearColor,
			ImageLayout layout = ImageLayout::AttachmentOutput);

		void AddOutput(RenderGraphTextureId textureId,
			uint32_t attachmentIndex,
			float depthClearValue,
			ImageLayout layout = ImageLayout::AttachmentOutput);

		inline const std::vector<Input>& GetInputs() const { return m_Inputs; };
		inline const std::vector<OutputAttachment>& GetOutputs() const { return m_Outputs; }
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

		bool m_HasOutputClearValues = false;
	};

}
