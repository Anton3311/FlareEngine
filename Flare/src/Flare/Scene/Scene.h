#pragma once

#include "Flare/Core/Core.h"
#include "Flare/AssetManager/Asset.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/RenderData.h"

#include "Flare/Scene/SceneRenderer.h"

#include "FlareECS.h"

namespace Flare
{
	class SceneSerializer;
	class Scene : public Asset
	{
	public:
		Scene(bool registerComponents = true);
		~Scene();

		void CopyFrom(const Ref<Scene>& scene);

		void Initialize();
		void InitializeRuntime();
	public:
		void OnRuntimeStart();
		void OnRuntimeEnd();

		void OnBeforeRender(RenderData& renderData);
		void OnRender(const RenderData& renderData);

		void OnUpdateRuntime();
		void OnViewportResize(uint32_t width, uint32_t height);

		inline World& GetECSWorld() { return m_World; }
	private:
		World m_World;
		Ref<Shader> m_QuadShader;

		SystemGroupId m_2DRenderingGroup;
		SystemGroupId m_ScriptingUpdateGroup;
		SystemGroupId m_OnRuntimeStartGroup;
		SystemGroupId m_OnRuntimeEndGroup;

		Query m_CameraDataUpdateQuery;
	private:
		friend SceneSerializer;
	};
}