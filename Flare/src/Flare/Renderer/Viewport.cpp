#include "Viewport.h"

namespace Flare
{
	Viewport::Viewport()
		: m_Position(0), m_Size(0)
	{
		FrameBufferSpecifications specs = FrameBufferSpecifications(0, 0, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Linear }
		});

		RTPool.SetSpecifications(specs);
	}

	void Viewport::Resize(glm::ivec2 position, glm::ivec2 size)
	{
		m_Position = position;
		m_Size = size;

		RTPool.OnViewportResize((glm::uvec2)m_Size);
	}
}
