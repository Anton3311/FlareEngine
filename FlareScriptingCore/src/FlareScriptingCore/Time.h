#pragma once

namespace Flare::Scripting
{
	struct TimeData
	{
		float DeltaTime;

		static TimeData Data;
	};

	class Time
	{
	public:
		static constexpr float GetDeltaTime() { return TimeData::Data.DeltaTime; }
	};
}