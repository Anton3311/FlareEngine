#include "PCH.h"

#include "SceneSubmition.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	void SceneSubmition::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		BatchedGeometry.Clear();

		PointLights.clear();
		SpotLights.clear();
		ShadowCastingSpotLights.clear();
		DecalSubmitions.clear();

		Renderer2DSubmition.Reset();
		DebugRendererSubmition.Reset();
	}
}
