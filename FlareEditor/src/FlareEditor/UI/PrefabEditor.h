#pragma once

#include "Flare/Scene/Prefab.h"

#include "FlareECS/World.h"

#include "FlareEditor/UI/AssetEditor.h"
#include "FlareEditor/UI/ECS/EntitiesHierarchy.h"
#include "FlareEditor/UI/ECS/EntityProperties.h"

namespace Flare
{
	class PrefabEditor : public AssetEditor
	{
	public:
		PrefabEditor(ECSContext& context);
	protected:
		virtual void OnOpen(AssetHandle asset) override;
		virtual void OnClose() override;
		virtual void OnRenderImGui(bool& show) override;
	private:
		World m_PrefabWorld;
		EntitiesHierarchy m_Entities;
		EntityProperties m_Properties;
		Ref<Prefab> m_Prefab;

		Entity m_SelectedEntity;
	};
}