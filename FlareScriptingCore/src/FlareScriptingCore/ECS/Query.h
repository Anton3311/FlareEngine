#pragma once

#include "FlareECS/QueryId.h"
#include "FlareECS/ComponentGroup.h"

#include "FlareScriptingCore/Bindings.h"

namespace Flare::Scripting
{
	class Query
	{
	public:
		constexpr Query()
			: m_Id(INVALID_QUERY_ID) {}
	private:
		constexpr Query(QueryId id)
			: m_Id(id) {}
	public:
		constexpr QueryId GetId() const { return m_Id; }

		template<typename... T>
		static constexpr Query New()
		{
			FilteredComponentsGroup<T...> components;
			return Query(Bindings::Instance->CreateQuery(components.GetComponents().data(), components.GetComponents().size()));
		}
	private:
		QueryId m_Id;
	};
}