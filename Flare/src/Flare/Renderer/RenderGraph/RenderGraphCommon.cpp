#include "RenderGraphCommon.h"

namespace Flare
{
	void CompiledRenderGraph::Reset()
	{
		LayoutTransitions.clear();
		ExternalResourceFinalTransitions = LayoutTransitionsRange(0);
	}
}
