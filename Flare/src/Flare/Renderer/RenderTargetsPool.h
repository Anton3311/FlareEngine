#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include "FlareCore/Collections/Span.h"
#include "Flare/Renderer/FrameBuffer.h"

#include <vector>
#include <unordered_map>

template<>
struct std::hash<Flare::Span<Flare::FrameBufferTextureFormat>>
{
	size_t operator()(const Flare::Span<Flare::FrameBufferTextureFormat>& formats) const
	{
		FLARE_CORE_ASSERT(!formats.IsEmpty());

		using UnderlyinfFormatType = std::underlying_type_t<Flare::FrameBufferTextureFormat>;
		auto hash = std::hash<UnderlyinfFormatType>();

		size_t result = hash((UnderlyinfFormatType)formats[0]);
		for (size_t i = 1; i < formats.GetSize(); i++)
		{
			Flare::CombineHashes(result, (UnderlyinfFormatType)formats[i]);
		}

		return result;
	}
};

namespace Flare
{
	class FLARE_API RenderTargetsPool
	{
	public:
		RenderTargetsPool() = default;

		Ref<FrameBuffer> Get();
		Ref<FrameBuffer> GetFullscreen(const Span<FrameBufferTextureFormat>& formats);

		void ReturnFullscreen(Span<FrameBufferTextureFormat> formats, Ref<FrameBuffer> renderTarget);

		void Release(const Ref<FrameBuffer>& renderTarget);
		void SetRenderTargetsSize(glm::ivec2 size);

		void SetSpecifications(const FrameBufferSpecifications& specs);
		void OnViewportResize(glm::uvec2 newSize);

		void Clear();
	private:
		struct FullscreenRenderTargetEntry
		{
			std::vector<FrameBufferTextureFormat> Formats;
			FrameBufferSpecifications Specifications;
			std::vector<Ref<FrameBuffer>> Pool;
		};

		glm::uvec2 m_ViewportSize = { 0, 0 };

		FrameBufferSpecifications m_Specifications;
		std::vector<Ref<FrameBuffer>> m_Pool;

		std::unordered_map<Span<FrameBufferTextureFormat>, FullscreenRenderTargetEntry> m_FullscreenTargets;
	};
}