#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include "Flare/Renderer/FrameBuffer.h"

#include <vector>

namespace Flare
{
	class FLARE_API RenderTargetsPool
	{
	public:
		RenderTargetsPool() = default;

		Ref<FrameBuffer> Get();
		void Release(const Ref<FrameBuffer>& renderTarget);
		void SetRenderTargetsSize(glm::ivec2 size);

		void SetSpecifications(const FrameBufferSpecifications& specs);
	private:
		FrameBufferSpecifications m_Specifications;
		std::vector<Ref<FrameBuffer>> m_Pool;
	};
}