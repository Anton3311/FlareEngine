#include "OpenGLGraphicsContext.h"

#include "Flare.h"

namespace Flare
{
	OpenGLGraphicsContext::OpenGLGraphicsContext(GLFWwindow* windowHandle)
		: m_Window(windowHandle)
	{

	}

	void OpenGLGraphicsContext::Initialize()
	{
		glfwMakeContextCurrent(m_Window);

		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		if (!status)
			FLARE_CORE_CRITICAL("Failed to initialize GLAD");
	}

	void OpenGLGraphicsContext::Present()
	{
		glfwSwapBuffers(m_Window);
	}
}
