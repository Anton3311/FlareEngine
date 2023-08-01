#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Scene/Scene.h"

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	enum class EditorMode
	{
		Edit,
		Play,
	};

	enum class GizmoMode
	{
		None,
		Translate,
		Rotate,
		Scale,
	};

	struct EditorContext
	{
	public:
		static void Initialize();
		static void Uninitialize();

	public:
		Entity SelectedEntity;
		EditorMode Mode;
		GizmoMode Gizmo;
	public:
		static EditorContext Instance;
	};
}