#pragma once

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/VertexArray.h"

#include "FlareECS/Entity/Entity.h"

#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"

namespace Flare
{
	class SceneViewportWindow : public ViewportWindow
	{
	public:
		SceneViewportWindow(EditorCamera& camera);

		virtual void OnRenderViewport() override;
		virtual void OnViewportChanged() override;
		virtual void OnRenderImGui() override;
		virtual void OnEvent(Event& event) override;
	protected:
		virtual void CreateFrameBuffer() override;
		virtual void OnClear() override;
	private:
		Entity GetEntityUnderCursor() const;
	private:
		EditorCamera& m_Camera;
		Ref<Shader> m_SelectionOutlineShader;
		Ref<Shader> m_GridShader;
		Ref<FrameBuffer> m_ScreenBuffer;
	};
}