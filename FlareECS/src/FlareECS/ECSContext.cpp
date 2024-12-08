#include "ECSContext.h"

namespace Flare
{
	static ECSContext* s_GlobalContext = nullptr;

	ECSContext::ECSContext()
		: Components(), Archetypes(Components), Queries(Archetypes)
	{
		FLARE_CORE_VERIFY(s_GlobalContext == nullptr);
		s_GlobalContext = this;
	}

	ECSContext::~ECSContext()
	{
		if (s_GlobalContext == this)
			s_GlobalContext = nullptr;
	}

	void ECSContext::Clear()
	{
		Queries.Clear();
		Archetypes.Clear();
		Components.Clear();
		SystemsRegistry.Clear();
	}

	ECSContext& ECSContext::GetGlobal()
	{
		FLARE_CORE_VERIFY(s_GlobalContext != nullptr);
		return *s_GlobalContext;
	}
}
