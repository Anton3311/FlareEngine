#include "OpenGLGPUTimer.h"

#include <glad/glad.h>

namespace Flare
{
	OpenGLGPUTImer::OpenGLGPUTImer()
		: m_Id(0)
	{
		glCreateQueries(GL_TIME_ELAPSED, 1, &m_Id);
	}

	OpenGLGPUTImer::~OpenGLGPUTImer()
	{
		glDeleteQueries(1, &m_Id);
	}

	void OpenGLGPUTImer::Start()
	{
		glBeginQuery(GL_TIME_ELAPSED, m_Id);
	}

	void OpenGLGPUTImer::Stop()
	{
		glEndQuery(GL_TIME_ELAPSED);
	}

	std::optional<float> OpenGLGPUTImer::GetElapsedTime()
	{
		uint64_t time = 0;
		glGetQueryObjectui64v(m_Id, GL_QUERY_RESULT, &time);

		const float milliToNano = 1000000;
		return (float)time / milliToNano;
	}
}
