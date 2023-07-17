#pragma once

#ifdef FLARE_BUILD_SHARED
	#define FLARE_API extern "C" __declspec(dllexport)
	#define FLARE_API_CLASS __declspec(dllexport)
#else
	#define FLARE_API
	#define FLARE_API_CLASS
#endif
