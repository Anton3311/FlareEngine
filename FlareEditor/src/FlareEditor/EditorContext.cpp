#include "EditorContext.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scripting/ScriptingEngine.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

namespace Flare
{
	EditorContext EditorContext::Instance{};

	void EditorContext::Initialize()
	{
		Instance.Gizmo = GizmoMode::Translate;
	}

	void EditorContext::Uninitialize()
	{
		Instance.SelectedEntity = Entity();
	}
}