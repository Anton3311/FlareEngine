#include "Shader.h"

#include "Flare/Platform/OpenGL/OpenGLShader.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	Ref<Shader> Shader::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLShader>();
		}

		return nullptr;
	}
}
