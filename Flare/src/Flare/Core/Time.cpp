#include "Time.h"

#include "FlarePlatform/Platform.h"

namespace Flare
{
	struct TimeData
	{
		float DeltaTime;
		float PreviousFrameTime;
	};

	TimeData s_Data;

	float Time::GetDeltaTime()
	{
		return s_Data.DeltaTime;
	}

	void Time::UpdateDeltaTime()
	{
		float currentTime = Platform::GetTime();
		s_Data.DeltaTime = currentTime - s_Data.PreviousFrameTime;
		s_Data.PreviousFrameTime = currentTime;
	}
}
