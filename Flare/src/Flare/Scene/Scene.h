#pragma once

#include "Flare/Core/Core.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/Asset.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/RenderData.h"

namespace Flare
{
	class SceneSerializer;
	class Scene : public Asset
	{
	public:
		Scene();
		~Scene();
	public:
		void OnBeforeRender(RenderData& renderData);
		void OnRender(const RenderData& renderData);

		void OnUpdateRuntime();
		void OnViewportResize(uint32_t width, uint32_t height);

		inline World& GetECSWorld() { return m_World; }
	private:
		World m_World;
		Ref<Shader> m_QuadShader;

		Query m_CameraDataUpdateQuery;
		Query m_SpritesQuery;
	private:
		static Ref<Scene> m_Active;

		friend SceneSerializer;
	};
}