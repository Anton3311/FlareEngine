#pragma once

#include "Flare/Core/Application.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "FlareEditor/EditorLayer.h"

namespace Flare
{
	class EditorApplication : public Application
	{
	public:
		EditorApplication();
		~EditorApplication();
	};
}