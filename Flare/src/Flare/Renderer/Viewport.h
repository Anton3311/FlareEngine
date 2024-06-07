#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class Texture;
	class FLARE_API Viewport
	{
	public:
		inline glm::ivec2 GetPosition() const { return m_Position; }
		inline glm::ivec2 GetSize() const { return m_Size; }

		inline float GetAspectRatio() const { return (float)m_Size.x / (float)m_Size.y; }

		void Resize(glm::ivec2 position, glm::ivec2 size);
	public:
		bool PostProcessingEnabled = true;
		bool ShadowMappingEnabled = true;

		RenderData FrameData;
		Ref<FrameBuffer> RenderTarget = nullptr;

		RenderGraph Graph;

		uint32_t ColorAttachmentIndex = UINT32_MAX;
		uint32_t NormalsAttachmentIndex = UINT32_MAX;
		uint32_t DepthAttachmentIndex = UINT32_MAX;

		Ref<Texture> ColorTexture = nullptr;
		Ref<Texture> NormalsTexture = nullptr;
		Ref<Texture> DepthTexture = nullptr;
	private:
		glm::ivec2 m_Position = glm::ivec2(0, 0);
		glm::ivec2 m_Size = glm::ivec2(0, 0);
	};
}