#include "EditorApplication.h"

namespace Flare
{
	EditorApplication::EditorApplication()
	{
		Renderer2D::Initialize();

		PushLayer(CreateRef<EditorLayer>());
	}

	EditorApplication::~EditorApplication()
	{
		Renderer2D::Shutdown();
	}
}
