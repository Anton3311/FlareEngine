#pragma once

#include <stdint.h>

#include "FlareScriptingCore/Bindings/ECS/ECS.h"

namespace Flare
{
	namespace Bindings
	{
		struct Entity
		{
		public:
			constexpr Entity()
				: m_Index(UINT32_MAX), m_Generation(UINT16_MAX), m_Dummy(UINT16_MAX) {}
		private:
			uint32_t m_Index;
			uint16_t m_Generation;
			uint16_t m_Dummy;
		};

		struct WorldBindings
		{
			using CreateEntityFunction = Entity(*)(ComponentId* components, size_t count);
			CreateEntityFunction CreateEntity;

			void* (*AddEntityComponent)(Entity entity, ComponentId component, const void* componentData, size_t componentDataSize);
			void(*RemoveEntityComponent)(Entity entity, ComponentId component);

			bool(*IsEntityAlive)(Entity entity);
			void(*DeleteEntity)(Entity entity);

			static WorldBindings Bindings;
		};

		class World
		{
		public:
			template<typename... Components>
			constexpr Entity CreateEntity()
			{
				ComponentId ids[sizeof...(Components)];

				size_t index = 0;
				([&]
				{
					ids[index] = Components::Id;
					index++;
				} (), ...);

				WorldBindings::Bindings.CreateEntity(ids, sizeof...(Components));
			}
		};
	}
}