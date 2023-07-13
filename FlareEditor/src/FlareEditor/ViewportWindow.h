#pragma once

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace Flare
{
	class ViewportWindow
	{
	public:
		ViewportWindow(std::string_view name, bool useEditorCamera = false);
	public:
		void OnRenderImGui();

		virtual void OnRenderViewport();
		void SetViewProjection(const glm::mat4& projection) { m_RenderData.Camera.ProjectionMatrix = projection; }
	protected:
		virtual void OnViewportResize() {}
	protected:
		std::string m_Name;
		Ref<FrameBuffer> m_FrameBuffer;
		RenderData m_RenderData;
	};
}