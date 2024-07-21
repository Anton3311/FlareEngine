#include "SceneSubmition.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	void SceneSubmition::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		OpaqueGeometrySubmitions.Clear();
		PointLights.clear();
		SpotLights.clear();
		DecalSubmitions.clear();

		DebugRendererSubmition.LineCount = 0;
		DebugRendererSubmition.RayCount = 0;
		DebugRendererSubmition.LineVertices.clear();
		DebugRendererSubmition.Rays.clear();
	}
}
