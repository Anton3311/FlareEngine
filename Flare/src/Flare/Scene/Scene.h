#pragma once

#include "Flare/Core/Core.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/CameraData.h"

namespace Flare
{
	class Scene
	{
	public:
		Scene();
		~Scene();
	public:
		void OnUpdateRuntime();
		void OnViewportResize(uint32_t width, uint32_t height);

		inline World& GetECSWorld() { return m_World; }
	private:
		uint32_t m_ViewportWidth = 0;
		uint32_t m_ViewportHeight = 0;

		World m_World;
		Ref<Shader> m_QuadShader;

		QueryId m_CameraDataUpdateQuery;
		CameraData m_CameraData;
	private:
		static Ref<Scene> m_Active;
	};
}