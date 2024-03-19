#pragma once

#include "Flare/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Flare
{
	class OpenGLGraphicsContext : public GraphicsContext
	{
	public:
		OpenGLGraphicsContext(GLFWwindow* windowHandle);
	public:
		virtual void Initialize() override;
		virtual void Release() override;
		virtual void BeginFrame() override;
		virtual void Present() override;
		virtual void WaitForDevice() override;
	private:
		GLFWwindow* m_Window;
	};
}