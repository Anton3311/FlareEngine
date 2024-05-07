#pragma once

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	class ImGuiLayerOpenGL : public ImGuiLayer
	{
	public:
		void InitializeRenderer() override;
		void ShutdownRenderer() override;
		void InitializeFonts() override;

		void Begin() override;
		void End() override;

		void RenderCurrentWindow() override;
		void UpdateWindows() override;

		ImTextureID GetTextureId(const Ref<const Texture>& texture) override;
		ImTextureID GetFrameBufferAttachmentId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachment) override;
	};
}
