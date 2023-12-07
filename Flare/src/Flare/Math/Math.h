#pragma once

#include "FlareCore/Core.h"

#include <glm/glm.hpp>

namespace Flare::Math
{
	FLARE_API bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

	struct Plane
	{
		inline float ValueAt(const glm::vec3& point) const
		{
			return glm::dot(point, Normal) + Offset;
		}

		inline bool IntersectsSegment(const glm::vec3& segmentStart, const glm::vec3& segmentEnd) const
		{
			float startValue = ValueAt(segmentStart);
			float endValue = ValueAt(segmentEnd);
			return glm::sign(startValue) != glm::sign(endValue);
		}

		inline static Plane TroughPoint(glm::vec3 point, glm::vec3 planeNormal)
		{
			return { planeNormal, -glm::dot(point, planeNormal)};
		}

		glm::vec3 Normal;
		float Offset;
	};

	struct Ray
	{
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

	inline float IntersectPlane(glm::vec3 planeNormal, glm::vec3 rayDirection, glm::vec3 rayOrigin)
	{
		return -glm::dot(planeNormal, rayOrigin) / glm::dot(rayDirection, planeNormal);
	}

	inline float IntersectPlane(const Plane& plane, const Ray& ray)
	{
		return (-plane.Offset - glm::dot(plane.Normal, ray.Origin)) / glm::dot(plane.Normal, ray.Direction);
	}

	struct AABB
	{
		constexpr AABB()
			: Min(0.0f), Max(0.0f) {}

		constexpr AABB(glm::vec3 min, glm::vec3 max)
			: Min(min), Max(max) {}

		inline void GetCorners(glm::vec3* corners) const
		{
			corners[0] = Min;
			corners[1] = glm::vec3(Min.x, Min.y, Max.z);
			corners[2] = glm::vec3(Max.x, Min.y, Min.z);
			corners[3] = glm::vec3(Max.x, Min.y, Max.z);

			corners[4] = glm::vec3(Min.x, Max.y, Min.z);
			corners[5] = glm::vec3(Min.x, Max.y, Max.z);
			corners[6] = glm::vec3(Max.x, Max.y, Min.z);
			corners[7] = Max;
		}

		inline glm::vec3 GetCenter() const { return (Max + Min) / 2.0f; }
		inline glm::vec3 GetSize() const { return Max - Min; }

		glm::vec3 Min;
		glm::vec3 Max;
	};
}