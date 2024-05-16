#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <stdint.h>
#include <chrono>

#ifdef FLARE_RELEASE
    #define FLARE_PROFILING_ENABLED
#endif

#include <Tracy.hpp>

#ifdef FLARE_PROFILING_ENABLED
	#define FLARE_PROFILE_BEGIN_FRAME(name) FrameMarkStart(name)
	#define FLARE_PROFILE_END_FRAME(name) FrameMarkEnd(name)

	#define FLARE_PROFILE_SCOPE(name) ZoneScopedN(name)
	#define FLARE_PROFILE_FUNCTION() FLARE_PROFILE_SCOPE(__FUNCSIG__)
#else
    #define FLARE_PROFILE_BEGIN_FRAME(name)
    #define FLARE_PROFILE_END_FRAME(name)
    #define FLARE_PROFILE_SCOPE(name)
    #define FLARE_PROFILE_FUNCTION()
#endif