#include "Sandbox.h"

#include <FlareCore/Log.h>

#include <Flare/Core/Time.h>

#include <Flare/Scene/Components.h>
#include <Flare/Scene/Transform.h>
#include <Flare/Scene/Prefab.h>

#include <Flare/Input/InputManager.h>

#include <FlareECS/World.h>
#include <FlareECS/System/System.h>

#include <FlareECS/Commands/CommandBuffer.h>

#include <iostream>
#include <random>

namespace Sandbox
{
	FLARE_IMPL_COMPONENT(RotatingQuadData);
	FLARE_IMPL_COMPONENT(SomeComponent);
	FLARE_IMPL_COMPONENT(TestComponent);
}
