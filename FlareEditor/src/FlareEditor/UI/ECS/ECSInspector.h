#pragma once

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Entity/Archetype.h"

namespace Flare
{
	class ECSInspector
	{
	public:
		void OnImGuiRender();

		static void Show();
		static ECSInspector& GetInstance();
	private:
		void RenderEntityInfo(Entity entity);
		void RenderArchetypeInfo(ArchetypeId archetype);
		void RenderSystem(uint32_t systemIndex);
	private:
		bool m_Shown;
	};
}