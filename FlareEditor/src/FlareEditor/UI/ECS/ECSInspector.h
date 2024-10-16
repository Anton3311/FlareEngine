#pragma once

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Entity/Archetypes.h"

namespace Flare
{
	struct QueryData;
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
		void RenderQuery(const QueryData& query);
	private:
		bool m_Shown;
	};
}