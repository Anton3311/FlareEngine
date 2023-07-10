#pragma once

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"

#include <string>
#include <string_view>

namespace Flare
{
	class ViewportWindow
	{
	public:
		ViewportWindow(std::string_view name, bool useEditorCamera = false);
	public:
		void OnRenderViewport();
		void OnRenderImGui();
	private:
		std::string m_Name;
		Ref<FrameBuffer> m_FrameBuffer;
		RenderData m_RenderData;
	};
}