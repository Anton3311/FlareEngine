#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	struct ViewportRect
	{
		glm::ivec2 Position = glm::ivec2(0);
		glm::ivec2 Size = glm::ivec2(0);
	};

	class FLARE_API Viewport
	{
	public:
		Viewport();

		const ViewportRect& GetRect() const { return m_Rect; }
		glm::ivec2 GetSize() const { return m_Rect.Size; }

		float GetAspectRatio() const { return (float)m_Rect.Size.x / (float)m_Rect.Size.y; }

		void Resize(const ViewportRect& newRect);
	public:
		RenderData FrameData;
	private:
		bool m_IsDirty;
		ViewportRect m_Rect;
	};
}