#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/RenderTargetsPool.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class FLARE_API Viewport
	{
	public:
		Viewport();

		inline glm::ivec2 GetPosition() const { return m_Position; }
		inline glm::ivec2 GetSize() const { return m_Size; }

		inline float GetAspectRatio() const { return (float)m_Size.x / (float)m_Size.y; }

		void Resize(glm::ivec2 position, glm::ivec2 size);
	public:
		RenderData FrameData;
		Ref<FrameBuffer> RenderTarget;
		RenderTargetsPool RTPool;
	private:
		bool m_IsDirty;

		glm::ivec2 m_Position;
		glm::ivec2 m_Size;
	};
}