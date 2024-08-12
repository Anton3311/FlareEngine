#include "ECSContext.h"

namespace Flare
{
	ECSContext::ECSContext()
		: Archetypes(Components), Queries(Archetypes)
	{
	}
}
