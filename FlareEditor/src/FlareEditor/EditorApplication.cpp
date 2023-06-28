#include "EditorApplication.h"

namespace Flare
{
	EditorApplication::EditorApplication()
	{
		PushLayer(CreateRef<EditorLayer>());
	}

	EditorApplication::~EditorApplication()
	{
	}
}
